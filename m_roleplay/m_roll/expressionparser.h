/* RollEngine expression parser header file. */
#ifndef __EXPRESSIONPARSER_H__
#define __EXPRESSIONPARSER_H__

/* The defined limits used in parsing expressions. */
#define MAX_EXP_TOKENS 1000   /* Maximum tokens in one expression. */
#define MAX_NUM_STACK 1000    /* Maximum numbers at once in evaluation. */
#define MAX_STACK_DEPTH 10000 /* Maximum recursion depth. */
#define MAX_TEXT_TOKEN 32     /* Maximum length of a textual token. */


/* Declare expression token classes. Each token is one of these classes. */
class ExpTokenOperator;
class ExpTokenNumber;

class RollEngine;
class ExpressionParser;

/* Expression token types. These define the valid tokens that may comprise an
 * expression. These are read from the input expression into an RPN format
 * expression for evaluation. Some tokens are never inserted into the final
 * expression, providing only information used during parsing.
 * ADD: An addition operation ("+")
 * SUBTRACT: A subtraction operator ("-")
 * MULTIPLY: A multiplication operator ("*")
 * DIVIDE: A division operator ("/")
 * MODULO: A modulo operator ("%")
 * EXPONENT: An exponent operator ("^")
 * UMINUS: An unary minus. ("-")
 * DICE: A dice operator ("d")
 * NUMBER: A number, or a constant converted to a number in parsing.
 * FUNCTION: A function, taking a single double and returning a single double.
 * OPAREN: Opening parenthesis; used during parsing, should never be added to
 *         the parsed expression.
 * CPAREN: Closing parenthesis; used during parsing, should never be added to
 *         the parsed expression.
 * END: End of the expression; indicates the expression string has ended.
 *      Should never be added to the parsed expression. */
enum ExpTokenType
{
	ADD, SUBTRACT,
	MULTIPLY, DIVIDE, MODULO,
	EXPONENT,
	UMINUS,
	DICE,
	NUMBER, FUNCTION, OPAREN, CPAREN,
	END
};



/* Expression token classes. Each of these contains the 'type' item, allowing
 * the type of the token to be viewed without knowing which type it is ahead
 * of time. */
class ExpTokenSimple { public: ExpTokenType type; };
class ExpTokenNumber { public: ExpTokenType type; double value; };
typedef double(RollEngine::*RollEngineMathFunc)(double);
class ExpTokenFunction { public: ExpTokenType type; RollEngineMathFunc pointer; };



/* Expression token. This is a union that can contain any one of the
 * expression token classes. */
union ExpToken {
	ExpTokenSimple token;
	ExpTokenNumber number;
	ExpTokenFunction function;
};


/* Text token comparison functor, used for the text token maps. */
class TextTokenCompare : public std::binary_function<char const *, char const *, bool>
{
 public:
	bool operator() (char const* str1, char const* str2) const
	{
		return strcmp (str1, str2) < 0;
	}
};



#endif
