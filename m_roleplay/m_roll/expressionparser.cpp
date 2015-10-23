/* RollEngine's expression parser. */
#include "rollengine.h"


void ExpressionParser::Parse(const char *expression_string)
{
	/* Reset the expression. */
	string = expression_string;
	ParsePosition = string;
	ParseStackDepth = 0;
	ExpressionLength = 0;

	/* Reset the current token to END, which as a previous token indicates
	 * the start when examined for contextual information. */
	CurrentToken.token.type = END;

	/* Read the first token and start parsing. This returns once the end is
	 * reached. */
	ReadToken();
	Level1();

	/* If we're not at the end of the string, we've 'junk' after the
	 * expression. */
	if (CurrentToken.token.type != END)
	{
		if (CurrentToken.token.type == CPAREN)
			ThrowParseError("Unmatched closing parenthesis:");
		else
			ThrowParseError("Invalid token for this position in expression:");
	}
}


void ExpressionParser::Level1()
{
	StackDepthCounter sdc(this);

	Level2();
	while (CurrentToken.token.type == ADD || CurrentToken.token.type == SUBTRACT)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		Level2();
		AddToken(OurToken);
	}
}


void ExpressionParser::Level2()
{
	StackDepthCounter sdc(this);

	Level3();
	while (CurrentToken.token.type == MULTIPLY || CurrentToken.token.type == DIVIDE || CurrentToken.token.type == MODULO)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		Level3();
		AddToken(OurToken);
	}
}


void ExpressionParser::Level3()
{
	StackDepthCounter sdc(this);

	Level4();
	while (CurrentToken.token.type == EXPONENT)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		Level4();
		AddToken(OurToken);
	}
}


void ExpressionParser::Level4()
{
	StackDepthCounter sdc(this);

	if (CurrentToken.token.type == UMINUS)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		Level4();  /* Allow for multiple unary minuses. */
		AddToken(OurToken);
	}
	else
		Level5();
}


void ExpressionParser::Level5()
{
	Level6();
	while (CurrentToken.token.type == DICE)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		Level6();
		AddToken(OurToken);
	}
}


void ExpressionParser::Level6()
{
	StackDepthCounter sdc(this);

	/* Handle numbers. */
	if (CurrentToken.token.type == NUMBER)
	{
		AddToken(CurrentToken);
		ReadToken();
		return;
	}

	/* Handle parenthesis. */
	if (CurrentToken.token.type == OPAREN)
	{
		ReadToken();
		if (CurrentToken.token.type == CPAREN)
			ThrowParseError("Missing expression in parenthesis:");
	
		Level1();

		if (CurrentToken.token.type != CPAREN)
			ThrowParseError("Missing closing parenthesis:");
		
		ReadToken();
		return;
	}

	/* Handle functions. */
	if (CurrentToken.token.type == FUNCTION)
	{
		ExpToken OurToken = CurrentToken;
		ReadToken();
		if (CurrentToken.token.type == CPAREN)
			ThrowParseError("Missing parameter for function:");
	
		Level1();

		if (CurrentToken.token.type != CPAREN)
			ThrowParseError("Missing closing parenthesis:");
		
		AddToken(OurToken);
		ReadToken();
		return;
	}

	/* Reaching end of string here means we were looking for a value for
	 * an operator above, but the string is missing one. */
	if (CurrentToken.token.type == END)
		ThrowParseError("End of expression when a number or equivalent was expected:");

	/* Otherwise, this is a generic "wanted an number/similar, but didn't
	 * get one" scenario. */
	ThrowParseError("Expected number or equivalent:");
}


void ExpressionParser::AddToken(ExpToken& token)
{
	if (ExpressionLength >= MAX_EXP_TOKENS)
	{
		ThrowParseError("Maximum expression tokens exceeded (expression too long or complex):");
	}

	Expression[ExpressionLength] = token;
	ExpressionLength++;
}


double ExpressionParser::Eval() {
	EvalPosition = 0;
	for (size_t i = 0; i < ExpressionLength; ++i)
	{
		if (Expression[i].token.type == NUMBER)
		{
			EvalStack[EvalPosition] = Expression[i].number.value;
			EvalPosition++;
		}

		else if (Expression[i].token.type == ADD)
		{
			EvalStack[EvalPosition-2] += EvalStack[EvalPosition-1];
			EvalPosition--;
		}

		else if (Expression[i].token.type == SUBTRACT)
		{
			EvalStack[EvalPosition-2] -= EvalStack[EvalPosition-1];
			EvalPosition--;
		}

		else if (Expression[i].token.type == MULTIPLY)
		{
			EvalStack[EvalPosition-2] *= EvalStack[EvalPosition-1];
			EvalPosition--;
		}

		else if (Expression[i].token.type == DIVIDE)
		{
			EvalStack[EvalPosition-2] /= EvalStack[EvalPosition-1];
			EvalPosition--;
		}

		else if (Expression[i].token.type == MODULO)
		{
			EvalStack[EvalPosition-2] = (int)EvalStack[EvalPosition-2] % (int)EvalStack[EvalPosition-1];
			EvalPosition--;
		}

		else if (Expression[i].token.type == EXPONENT)
		{
			EvalStack[EvalPosition-2] = pow(EvalStack[EvalPosition-2], EvalStack[EvalPosition-1]);
			EvalPosition--;
		}

		else if (Expression[i].token.type == UMINUS)
			EvalStack[EvalPosition-1] = -EvalStack[EvalPosition-1];

		else if (Expression[i].token.type == DICE)
		{
			EvalStack[EvalPosition-2] = engine->RollTheBones(EvalStack[EvalPosition-2], EvalStack[EvalPosition-1]);
			EvalPosition--;
		}

		else if (Expression[i].token.type == FUNCTION)
		{
			EvalStack[EvalPosition-1] = (engine->*Expression[i].function.pointer)(EvalStack[EvalPosition-1]);
		}
	}

	/* Return any zero result as positive zero, never negative. */
	if (EvalStack[0] == 0)
		return 0;
	
	/* Return the result. */
	return EvalStack[0];
}


void ExpressionParser::ReadToken()
{
	StackDepthCounter sdc(this);

	/* Remember the previous token, for interpretation based on context. */
	/* A prev_token of END indicates this is the starting token. */
	ExpTokenType prev_token = CurrentToken.token.type;

	/* Handle constants and functions. */
	if (isalnum(*ParsePosition))
	{
		/* Build the maximum possible length text token. */
		/* This stops at the first (. */
		size_t TextLength = 0;
		while ((isalnum(*ParsePosition) || *ParsePosition == '(') && TextLength < MAX_TEXT_TOKEN)
		{
			TextToken[TextLength] = tolower(*ParsePosition);
			ParsePosition++;
			TextLength++;
			if (TextToken[TextLength-1] == '(')
				break;
		}
		TextToken[TextLength] = '\0';

		/* If it ends in an (, try matching it as a function. */
		if (TextToken[TextLength-1] == '(')
		{
			if (engine->functions.find(TextToken) != engine->functions.end())
			{
				/* Handle implicit multiplication. */
				/* This must be checked AFTER we know it is a
				 * valid function, or other alphabetical
				 * operators can break. */
				if (prev_token == NUMBER || prev_token == CPAREN)
				{
					CurrentToken.token.type = MULTIPLY;
					ParsePosition -= TextLength;
					return;
				}
		
				CurrentToken.function.type = FUNCTION;
				CurrentToken.function.pointer = engine->functions[TextToken];
				return;
			}
			
			TextToken[TextLength-1] = '\0';
			ParsePosition--;
			TextLength--;
		}

		/* If that fails, try to match it as a constant, dropping
		 * letters off until we find a match. */
		while (TextLength > 0)
		{
			if (engine->constants.find(TextToken) != engine->constants.end())
			{
				/* Handle implicit multiplication. */
				/* This must be checked AFTER we know it is a
				 * valid function, or other alphabetical
				 * operators can break. */
				if (prev_token == NUMBER || prev_token == CPAREN)
				{
					CurrentToken.token.type = MULTIPLY;
					ParsePosition -= TextLength;
					return;
				}
		
				CurrentToken.number.type = NUMBER;
				CurrentToken.number.value = engine->constants[TextToken];
				break;
			}
			TextToken[TextLength-1] = '\0';
			ParsePosition--;
			TextLength--;
		}

		/* If we matched a constant or function, return. Otherwise,
		 * carry on and let other token types try to match it. */
		if (TextLength > 0)
			return;
	}

	/* Handle numbers. */
	if ((*ParsePosition >= 48 && *ParsePosition <= 57) || (*ParsePosition == '.' && *(ParsePosition+1) >= 48 && *(ParsePosition+1) <= 57))
	{ 
		/* Handle implicit multiplication. */
		if (prev_token == NUMBER || prev_token == CPAREN)
		{
			CurrentToken.token.type = MULTIPLY;
			return;
		}
		
		
		char* end;
		double number = strtod(ParsePosition, &end);
		CurrentToken.number.type = NUMBER;
		CurrentToken.number.value = number;
		ParsePosition = end;
		return;
	}

	/* Handle %-meaning-the-number-100 in the special context of a roll. */
	if (prev_token == DICE && *ParsePosition == '%')
	{
		CurrentToken.number.type = NUMBER;
		CurrentToken.number.value = 100;
		ParsePosition++;
		return;
	}

	/* Handle unary plus/minus. */
	if ((prev_token != NUMBER && prev_token != CPAREN) && *ParsePosition == '+')
	{
		/* Ignore unary plus and skip to the next value. */
		ParsePosition++;
		ReadToken();
		return;
	}
	if ((prev_token != NUMBER && prev_token != CPAREN) && *ParsePosition == '-')
	{
		CurrentToken.token.type = UMINUS;
		ParsePosition++;
		return;
	}

	/* Handle basic operators. */
	if (*ParsePosition == '+')
	{
		CurrentToken.token.type = ADD;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == '-')
	{
		CurrentToken.token.type = SUBTRACT;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == '*')
	{
		CurrentToken.token.type = MULTIPLY;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == '/')
	{
		CurrentToken.token.type = DIVIDE;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == '^')
	{
		CurrentToken.token.type = EXPONENT;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == '%')
	{
		CurrentToken.token.type = MODULO;
		ParsePosition++;
		return;
	}

	/* Handle dice operators. */
	if (*ParsePosition == 'd')
	{
		/* Handle implicit 1 in front of unnumbered dice rolls. */
		if (prev_token != NUMBER && prev_token != CPAREN)
		{
			CurrentToken.number.type = NUMBER;
			CurrentToken.number.value = 1;
			return;
		}
		
		CurrentToken.token.type = DICE;
		ParsePosition++;
		return;
	}

	/* Handle parenthesis. */
	if (*ParsePosition == '(')
	{
		/* Handle implicit multiplication. */
		if (prev_token == NUMBER || prev_token == CPAREN)
		{
			CurrentToken.token.type = MULTIPLY;
			return;
		}
		
		CurrentToken.token.type = OPAREN;
		ParsePosition++;
		return;
	}
	if (*ParsePosition == ')')
	{
		CurrentToken.token.type = CPAREN;
		ParsePosition++;
		return;
	}
	
	/* Handle end of string. */
	if (*ParsePosition == '\0')
	{
		CurrentToken.token.type = END;
		ParsePosition++;
		return;
	}

	/* If nothing is matched, throw an unrecognised token error. */
	ParsePosition++;
	ThrowParseError("Unrecognised token in expression:");
}


void ExpressionParser::ThrowParseError(const char* message)
{
	engine->results->Clear();
	engine->results->AddError("Error parsing expression.");
	engine->results->AddError(message);
	engine->results->AddError(string);

	/* When positioning this, it is important to note that the parse
	 * position points at the NEXT character to be read, not the current
	 * one being read, which is where we should put the error mark. */
	std::string error_marker;
	error_marker.reserve(ParsePosition - string);
	for (int i = 1; i < ParsePosition - string; i++)
		error_marker += '-';
	error_marker += '^';
	engine->results->AddError(error_marker);

	throw new RollException;
}
