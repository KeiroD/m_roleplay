/* RollEngine basic roll and math functions. */
#include "rollengine.h"



/* This is where the magic happens; the dice roll function. For better or for
 * worse... time to roll the bones and see how lady luck dances. */
double RollEngine::RollTheBones(double count, double sides)
{
	/* Limit the maximum number of dice rolled to 0 to 10000. */
	if (count < 0)
	{
		if (IncWarningCount())
			results->AddError("Warning: Dice roll of " + Str(count) + "d" + Str(sides) + " specified a negative number of dice; 0d" + Str(sides) + " will be rolled instead.");
		
		count = 10000;
	}
	if (count > 10000)
	{
		if (IncWarningCount())
			results->AddError("Warning: Dice roll of " + Str(count) + "d" + Str(sides) + " exceeded the maximum number of dice, and was capped at the maximum of 10000d" + Str(sides) + ".");
		
		count = 10000;
	}

	/* Limit the sides rolled to 1 to 10000. */
	if (sides < 1)
	{
		if (IncWarningCount())
			results->AddError("Warning: Dice roll of " + Str(count) + "d" + Str(sides) + " specified zero or negative sides to a die; " + Str(count) + "d1 will be rolled instead.");
		
		sides = 1;
	}
	if (sides > 10000)
	{
		if (IncWarningCount())
			results->AddError("Warning: Dice roll of " + Str(count) + "d" + Str(sides) + " exceeded the maximum sides to a die, and was capped at the maximum of " + Str(count) + "d10000.");
		
		sides = 10000;
	}

	/* Round to integeral values; others are invalid for a dice roll. */
	if (round(count) != count || round(sides) != sides)
	{
		if (IncWarningCount())
		{
			double old_count = count;
			double old_sides = sides;
		
			count = round(count);
			sides = round(sides);

			results->AddError("Warning: Dice roll of " + Str(old_count) + "d" + Str(old_sides) + " contained non-integral values, and was rounded to " + Str(count) + "d" + Str(sides) + ".");
		}
		else
		{
			count = round(count);
			sides = round(sides);
		}
	}

	/* Checks complete, perform the roll! */
	double total = 0;
	for (unsigned int i = 0; i < count; i++)
		total += (double)Random((unsigned int)sides);
	
	/* Return the result. */
	return total;
}



/* Returns a random number between 1 and max. */
/* This is rather arcane, but it spreads bias where max is not a factor of
 * RAND_MAX throughout the range between 1 and max, rather than placing it
 * all at the upper end. */
unsigned int RollEngine::Random(unsigned int max)
{
	double divisor = RAND_MAX + 1.0;
	unsigned int result = (unsigned int)(1.0 + (double)rand_r(&seed) * ((double) max / divisor));
	return result;
}



/* Returns a random integer between 1 and the passed number. */
double RollEngine::ran(double max)
{
	return (double)Random((unsigned int) max);
}



double RollEngine::deg(double x)
{
	return x * 180/M_PI;
}



double RollEngine::fac(double x)
{
	x = round(x);

	double result = 1;
	for (unsigned int n = 1; n <= x; n++)
	{
		result *= n;

		/* Once we've reached infinity, there's no point continuing.
		 * it's not going to get any bigger. This also stops us from
		 * being told to calculate insane factorials. */
		if (result == std::numeric_limits<double>::infinity())
			return result;
	}
	return result;
}



double RollEngine::rad(double x)
{
	return x * M_PI/180;
}



double RollEngine::sqr(double x)
{
	return x*x;
}


/* RollEngine wrapper math functions. */
double RollEngine::abs(double x) { return fabs(x); }
double RollEngine::acos(double x) { return ::acos(x); }
double RollEngine::asin(double x) { return ::asin(x); }
double RollEngine::atan(double x) { return ::atan(x); }
double RollEngine::ceil(double x) { return ::ceil(x); }
double RollEngine::cos(double x) { return ::cos(x); }
double RollEngine::cosh(double x) { return ::cosh(x); }
double RollEngine::exp(double x) { return ::exp(x); }
double RollEngine::floor(double x) { return ::floor(x); }
double RollEngine::log(double x) { return ::log(x); }
double RollEngine::log10(double x) { return ::log10(x); }
double RollEngine::round(double x) { return ::round(x); }
double RollEngine::sin(double x) { return ::sin(x); }
double RollEngine::sinh(double x) { return ::sinh(x); }
double RollEngine::sqrt(double x) { return ::sqrt(x); }
double RollEngine::tan(double x) { return ::tan(x); }
double RollEngine::tanh(double x) { return ::tanh(x); }
double RollEngine::trunc(double x) { return ::trunc(x); }
