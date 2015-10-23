/* RollEngine's main source file. */
#include "rollengine.h"

RollEngine::RollEngine()
{
	seed = time(NULL);
	expression = new ExpressionParser(this);

	/* Insert all our constants into the map. */
	constants.insert(std::pair<const char*, double>("pi", M_PI));
	constants.insert(std::pair<const char*, double>("i", 1));
	constants.insert(std::pair<const char*, double>("ii", 2));
	constants.insert(std::pair<const char*, double>("iii", 3));
	constants.insert(std::pair<const char*, double>("iv", 4));
	constants.insert(std::pair<const char*, double>("v", 5));
	constants.insert(std::pair<const char*, double>("vi", 6));
	constants.insert(std::pair<const char*, double>("vii", 7));
	constants.insert(std::pair<const char*, double>("viii", 8));
	constants.insert(std::pair<const char*, double>("ix", 9));
	constants.insert(std::pair<const char*, double>("x", 10));
	
	/* Insert all our functions into the function map. */
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("abs(", &RollEngine::abs));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("acos(", &RollEngine::acos));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("asin(", &RollEngine::asin));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("atan(", &RollEngine::atan));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("ceil(", &RollEngine::ceil));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("cos(", &RollEngine::cos));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("cosh(", &RollEngine::cosh));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("deg(", &RollEngine::deg));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("exp(", &RollEngine::exp));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("fac(", &RollEngine::fac));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("floor(", &RollEngine::floor));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("log(", &RollEngine::log));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("log10(", &RollEngine::log10));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("rad(", &RollEngine::rad));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("ran(", &RollEngine::ran));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("round(", &RollEngine::round));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("sin(", &RollEngine::sin));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("sinh(", &RollEngine::sinh));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("sqr(", &RollEngine::sqr));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("sqrt(", &RollEngine::sqrt));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("tan(", &RollEngine::tan));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("tanh(", &RollEngine::tanh));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("trunc(", &RollEngine::trunc));
	
	/* Insert alias function names into the function map. */
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("logten(", &RollEngine::log10));
	functions.insert(std::pair<const char*, double(RollEngine::*)(double)>("todeg(", &RollEngine::deg));

	/* Increase the output precision (maximum digits shown) when converting
	 * numbers to strings. */
	convstream.precision(10);

	/* Determine how distracted the fuzz are... */
	fuzzfactor = RollTheBones(1, 100);
}


void RollEngine::Run(const Roll& passed_roll, RollResults& passed_results)
{
	roll = &passed_roll;
	results = &passed_results;
	warning_count = 0;

	try {
		if (roll->type == CALC)
			RollExpression();
		else if (roll->type == ROLL)
			DoRoll();
		else if (roll->type == SCORES)
			DoScores();
	}
	catch (RollException* boom)
	{
		delete boom;
		/* Just quietly return, terminating handling of the roll.  */
	}
}


double RollEngine::ReadExpression(const std::string& expression_string)
{
	/* Handle special RPG expressions the parser cannot handle. */
	/* These must not be system-specific presets, must not produce more
	 * than one result, and must not require a special output format. */
	if (expression_string == "%")
	{
		/* "%" means 1d100. */
		return RollTheBones(1, 100);
	}
	else if (!strcasecmp(expression_string.c_str(), "d%HL") || !strcasecmp(expression_string.c_str(), "%HL"))
	{
		/* "%HL" and "d%HL" mean to perform a traditional percentile
		 * dice roll; two ten-sided, 0-9 dice for tens and ones, with
		 * two 0s indicating a result of 100. */
		/* Yes, this produces the same odds as and is outwardly
		 * indistinguishable from a 1d100. The old-time games know, but
		 * old dice superstitions die hard. */
		double tens = RollTheBones(1, 10) - 1;
		double ones = RollTheBones(1, 10) - 1;
		if (!tens && !ones)
			return 100;
		else
			return tens * 10 + ones;
	}

	/* Parse the expression normally, if it was not handled as a special
	 * case. */
	expression->Parse(expression_string.c_str());
	
	/* Return the results. */
	return expression->Eval();
}


bool RollEngine::IncWarningCount()
{
	if (warning_count < 3)
	{
		warning_count++;
		return true;
	}
	if (warning_count == 3)
	{
		results->AddError("Warning: Maximum warnings exceeded; further warnings were not shown.");
		warning_count++;
		return false;
	}

	return false;
}


const std::string& RollEngine::For()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM)
		forstring = " for " + roll->extra[0];
	else
		forstring = "";
	return forstring;
}


const std::string& RollEngine::Str(double number)
{
	convstream.str(""); convstream.clear();
	convstream << number; convstream >> convstring;
	return convstring;
}
const std::string& RollEngine::Str(double number, const std::string& input)
{
	convstream.str(""); convstream.clear();
	convstream << number; convstream >> convstring;
	if (convstring != input)
		convstring += " (" + input + ")";
	return convstring;
}
