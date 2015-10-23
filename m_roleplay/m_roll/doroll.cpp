/* RollEngine's ROLL-type roll handling. */
#include "rollengine.h"



void RollEngine::DoRoll()
{
	std::vector<const char*> expr;
	expr.resize(roll->expression.size());
	for (size_t i = 0; i < roll->expression.size(); i++)
		expr[i] = roll->expression[i].c_str();

	if (!strcasecmp(expr[0], "craps"))
		RollCraps();

	else if (expr.size() >= 2 && !strcasecmp(expr[0], "the") && !strcasecmp(expr[1], "dice"))
		RollCraps();

	else if (!strcasecmp(expr[0], "dtwenty") || !strcasecmp(expr[0], "dt"))
		RollD20();

	else if (!strcasecmp(expr[0], "exalted") || !strcasecmp(expr[0], "exalt") || !strcasecmp(expr[0], "exal") || !strcasecmp(expr[0], "ex"))
		RollExalted(1);

	else if (!strcasecmp(expr[0], "exalted2") || !strcasecmp(expr[0], "exalt2") || !strcasecmp(expr[0], "exal2") || !strcasecmp(expr[0], "ex2"))
		RollExalted(0);

	else if (!strcasecmp(expr[0], "newhorizons") || !strcasecmp(expr[0], "nh") || !strcasecmp(expr[0], "horizons") || !strcasecmp(expr[0], "hz"))
		RollNewHorizons();

	else if (!strcasecmp(expr[0], "rtd"))
		RollRTD();

	else if (!strcasecmp(expr[0], "shadowrun") || !strcasecmp(expr[0], "shadow") || !strcasecmp(expr[0], "shad"))
		RollShadowrun();

	else if (!strcasecmp(expr[0], "wod"))
		RollWOD();
		
	else if (!strcasecmp(expr[0], "rwod"))
		RollRWOD();

	else if (!strcasecmp(expr[0], "nwod"))
		RollNWOD();

	else if (!strcasecmp(expr[0], "nwodc"))
		RollNWODChance();

	else if (!strcasecmp(expr[0], "init"))
		RollDND2EInit();

	else if (!strcasecmp(expr[0], "attack") || !strcasecmp(expr[0], "hit") || !strcasecmp(expr[0], "check") || !strcasecmp(expr[0], "save"))
		RollDNDAlias();

	else if (!strcasecmp(expr[0], "barrel"))
		RollEEBarrel();

	else if ((expr.size() >= 3 && !strcasecmp(expr[0], "down") && !strcasecmp(expr[1], "the") && !strcasecmp(expr[2], "stairs")) || !strcasecmp(expr[0], "stairs"))
		RollEEDownTheStairs();

	else if ((expr.size() >= 3 && !strcasecmp(expr[0], "in") && !strcasecmp(expr[1], "the") && !strcasecmp(expr[2], "hay")) || !strcasecmp(expr[0], "hay"))
		RollEEInTheHay();

	else if (!strcasecmp(expr[0], "joint") || !strcasecmp(expr[0], "cigar"))
		RollEEJoint();

	else if (!strcasecmp(expr[0], "fuzzfactor"))
		RollEEJointFuzzFactor();

	else if (!strcasecmp(expr[0], "over"))
		RollEEOver();

	else if (!strcasecmp(expr[0], "rick"))
		RollEERick();

	else if (expr.size() >= 2 && (!strcasecmp(expr[0], "your") || !strcasecmp(expr[0], "yo")) && (!strcasecmp(expr[1], "mom") || !strcasecmp(expr[1], "mum") || !strcasecmp(expr[1], "mother") || !strcasecmp(expr[1], "momma")))
		RollEEYourMom();

	else if (expr.size() >= 2 && (!strcasecmp(expr[0], "your") || !strcasecmp(expr[0], "yo")) && (!strcasecmp(expr[1], "dad") || !strcasecmp(expr[1], "father") || !strcasecmp(expr[1], "dad")))
		RollEEYourDad();

	else if (strchr(expr[0], '[') && expr[0][strlen(expr[0])-1] == ']')
		RollRepeatedExpression();

	else
		RollExpression();
}



void RollEngine::RollCraps()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Add first line. */
	results->AddMsg("<Results of Craps" + For() + ">" + message);

	/* Perform the first roll. */
	double sum, point;
	unsigned int roll_count = 1;
	sum = RollTheBones(2, 6);

	/* Show it, continuing if it isn't an instant win/lose. */
	if (sum == 2)
		results->AddMsg("<Rolled a 2, snake eyes, you lose>");
	else if (sum == 3) 
		results->AddMsg("<Rolled a 3, craps, you lose>");
	else if (sum == 12) 
		results->AddMsg("<Rolled a 12, box cars, you lose>");
	else if (sum == 7) 
		results->AddMsg("<Rolled a 7, a natural 7, you win>");
	else if (sum == 11) 
		results->AddMsg("<Rolled an 11, a natural 11, you win>");
	else
	{
		/* The results of this first roll are now the point. */
		point = sum;
		results->AddMsg("<Rolled a " + Str(sum) + ", the point is " + Str(sum) + ">");

		/* Keep rolling up to a limit until they win or lose. */
		std::string point_str = Str(point);
		while (roll_count < 10)
		{
			sum = RollTheBones(2, 6);
			roll_count++;

			if (sum == point)
			{
				results->AddMsg("<Rolled a " + Str(sum) + ", the point was " + point_str + ", matched point and won>");
				break;
			}
			else if (sum == 7)
			{
				results->AddMsg("<Rolled a 7, the point was " + point_str + ", sevened-out and lost>");
				break;
			}
			else
				results->AddMsg("<Rolled a " + Str(sum) + ", the point was " + point_str + ">");
		}
	}
}



void RollEngine::RollD20()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: D20 rolls require an additional parameter specifying the modifier to the roll, with an optional parameter for a target value.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 3; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the modifier. */
	double mod = round(ReadExpression(roll->expression[1]));

	/* Get the target value, if one is provided. */
	double diff = 0;
	if (roll->expression.size() >= 3)
	{
		diff = round(ReadExpression(roll->expression[2]));
	}

	/* Perform the roll. */
	double result = RollTheBones(1, 20);

	/* Check for success. */
	bool success = false;
	if (diff != 0 && result + mod >= diff)
		success = true;

	/* Output the results. */
	if (diff != 0)
	{
		std::string line = "<D20 check" + For() + " [Modifier: " + Str(mod, roll->expression[1]);
		line += ", Diff: " + Str(diff, roll->expression[2]) + "]>" + message;
		results->AddMsg(line);
		line = "<Roll: " + Str(result);
		line += ", Total: " + Str(result + mod) + " - ";
		line += (success ? "Success" : "Failure");
		line += ">";
		results->AddMsg(line);
	}
	else
	{
		results->AddMsg("<D20 check" + For() + " [Modifier: " + Str(mod, roll->expression[1]) + "]>" + message);
		std::string line = "<Roll: " + Str(result);
		line += ", Total: " + Str(result + mod) + ">";
		results->AddMsg(line);
	}
}



void RollEngine::RollExalted(bool double_successes)
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: Exalted rolls require an additional parameter specifying the number of dice to roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 2; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: Exalted roll specified zero or negative dice to roll, and will have a single die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: Exalted roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Add the descriptive line and start the result line. */
	results->AddMsg("<Exalted roll" + For() + " [Dice: " + Str(count, roll->expression[1]) + "]>" + message);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double result = RollTheBones(1, 10);
		resultline += Str(result);

		if (result >= 7)
			successes++;
		if (result == 10 && double_successes)
			successes++;

		if (i != count -1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	results->AddMsg("<Successes: " + Str(successes) + ">");
}



void RollEngine::RollNewHorizons()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: New Horizons rolls require one additional parameters specifying the number of dice to roll, with an optional parameter for the difficulty of the roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 3; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: New Horizons roll specified zero or negative dice to roll, and will roll one die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: New Horizons roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Get the difficulty of the roll. */
	double diff = 7;
	std::string diffstr = "7";
	if (roll->expression.size() >= 3)
	{
		diff = round(ReadExpression(roll->expression[2]));
		diffstr = Str(diff, roll->expression[2]);
	}


	/* Add the descriptive line and start the result line. */
	results->AddMsg("<New Horizons roll" + For() + " [Dice: " + Str(count, roll->expression[1]) + ", Diff: " + diffstr + "]>" + message);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double die = RollTheBones(1, 10);
		double result = die;

		while (die == 10)
		{
			die = RollTheBones(1, 10);
			result += die;
		}

		if (result == 1)
			successes--;
		else if (result >= diff)
			successes++;


		resultline += Str(result);
		if (i != count - 1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	if (successes > 0 )
		results->AddMsg("<Successes: " + Str(successes) + ">");
	else if (successes < 0 )
		results->AddMsg("<BOTCHED ROLL! Botches: " + Str(0-successes) + ">");
	else
		results->AddMsg("<Simple Failure>");

}



void RollEngine::RollRTD()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++) {
		if (i==1)
			message += ":";
		message += " " + roll->expression[i];
	}


	/* The Evil Switch Knoweth All things. */

	switch ((int)RollTheBones(1,6)) {
		case 1:
			results->AddMsg("<RTD roll" + For() + ": 1 - Horrifyingly Bad" + message + ">");
			break;
		case 2:
			results->AddMsg("<RTD roll" + For() + ": 2 - Failure" + message + ">");
			break;
		case 3:
			results->AddMsg("<RTD roll" + For() + ": 3 - Partial Success" + message + ">");
			break;
		case 4:
			results->AddMsg("<RTD roll" + For() + ": 4 - Success" + message + ">");
			break;
		case 5:
			results->AddMsg("<RTD roll" + For() + ": 5 - Perfect Success" + message + ">");
			break;
		default:
			results->AddMsg("<RTD roll" + For() + ": 6 - Horrifyingly Good" + message + ">");
	}

}



void RollEngine::RollShadowrun()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 3)
	{
		results->Clear();
		results->AddError("Error: Shadowrun rolls require two additional parameters specifying the number of dice to roll, and the difficulty of the roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 3; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: Shadowrun roll specified zero or negative dice to roll, and will roll one die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: Shadowrun roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Get the difficulty of the roll. */
	double diff = round(ReadExpression(roll->expression[2]));


	/* Add the descriptive line and start the result line. */
	std::string descline = "<Shadowrun roll" + For() + " [Dice: " + Str(count, roll->expression[1]);
	descline += ", Diff: " + Str(diff, roll->expression[2]) + "]>" + message;
	results->AddMsg(descline);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double die = RollTheBones(1, 6);
		double result = die;

		while (die == 6)
		{
			if (result >= 120)
			{
				if (IncWarningCount())
					results->AddError("Warning: Shadowrun roll result exceeded maximum number of repeats, was capped at 120.");
				break;
			}
			die = RollTheBones(1, 6);
			result += die;
		}

		if (result >= diff)
			successes++;

		resultline += Str(result);
		if (i != count - 1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	results->AddMsg("<Successes: " + Str(successes) + ">");
}



void RollEngine::RollWOD()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: World of Darkness rolls require an additional parameters specifying the number of dice to roll, with an optional parameter for the difficulty of the roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 3; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: World of Darkness roll specified zero or negative dice to roll, and will roll one die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: World of Darkness roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Get the difficulty of the roll. */
	double diff = 6;
	std::string diffstr = "6";
	if (roll->expression.size() >= 3)
	{
		diff = round(ReadExpression(roll->expression[2]));
		diffstr = Str(diff, roll->expression[2]);
	}

	
	/* Add the descriptive line and start the result line. */
	results->AddMsg("<World of Darkness roll" + For() + " [Dice: " + Str(count, roll->expression[1]) + ", Diff: " + diffstr + "]>" + message);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	double rerolls = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double die = RollTheBones(1, 10);

		if (die == 10)
		{
			++rerolls;
		} else if (die == 1) {
			--successes;
		}

		if (die >= diff)
			successes++;

		resultline += Str(die);
		if (i != count - 1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	if (successes > 0 && rerolls > 0) {
		std::string totalline = "<Successes: " + Str(successes) + ", ";
		totalline += "Rerolls: " + Str(rerolls) + " (with a specialization)>";
		results->AddMsg(totalline);
	} else if (successes > 0 )
		results->AddMsg("<Successes: " + Str(successes) + ">");
	else if (successes < 0 )
		results->AddMsg("<BOTCHED ROLL! Botches: " + Str(0-successes) + ">");
	else
		results->AddMsg("<Simple Failure>");
}



void RollEngine::RollRWOD()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: Revised Edition World of Darkness rolls require an additional parameter specifying the number of dice to roll, with an optional parameter for the difficulty of the roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 3; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: Revised Edition World of Darkness roll specified zero or negative dice to roll, and will roll one die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: Revised Edition World of Darkness roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Get the difficulty of the roll. */
	double diff = 6;
	std::string diffstr = "6";
	if (roll->expression.size() >= 3)
	{
		diff = round(ReadExpression(roll->expression[2]));
		diffstr = Str(diff, roll->expression[2]);
	}


	/* Add the descriptive line and start the result line. */
	results->AddMsg("<Revised Edition World of Darkness roll" + For() + " [Dice: " + Str(count, roll->expression[1]) + ", Diff: " + diffstr + "]>" + message);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	double ones = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double die = RollTheBones(1, 10);
		resultline += Str(die);

		if (die >= diff)
			successes++;
		else if (die == 1)
			ones++;

		if (die == 10) {
			resultline += "(";
			while (die == 10) {
				die=RollTheBones(1, 10);
				resultline += Str(die);
				if (die == 10)
					resultline += ",";
			}
			resultline += ")";
		}

		if (i != count - 1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	if (successes > 0 )
		results->AddMsg("<Successes: " + Str(successes) + ">");
	else if (successes == 0 && ones > 0)
		results->AddMsg("<BOTCHED ROLL!");
	else
		results->AddMsg("<Simple Failure>");
}



void RollEngine::RollNWOD()
{
	/* Check we have the required number of parameters. */
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("Error: New World of Darkness rolls require you specify the number of dice in your dice pool to roll.");
		throw new RollException;
	}

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 2; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Get the number of rolls to make. */
	double count = round(ReadExpression(roll->expression[1]));
	if (count < 1)
	{
		results->AddError("Warning: New World of Darkness roll specified zero or negative dice to roll, and will roll one die instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: New World of Darkness roll specified " + Str(count) + " dice to roll, exceeding the maximum, and was capped at the maximum of 40 dice.");

		count = 40;
	}

	/* Add the descriptive line and start the result line. */
	std::string descline = "<New World of Darkness roll" + For() + " [Dice Pool: " + Str(count, roll->expression[1]);
	descline += "]>" + message;
	results->AddMsg(descline);
	std::string resultline = "<";

	/* Perform the roll. */
	double successes = 0;
	for (unsigned int i = 0; i < count; i++)
	{
		double die = RollTheBones(1, 10);

		if (die >= 6)
			successes++;
		resultline += Str(die);
		if (die == 10) {
			resultline += "(";
			while (die == 10) {
				die=RollTheBones(1, 10);
				resultline += Str(die);
				if (die == 10)
					resultline += ",";
			}
			resultline += ")";
		}
		if (i != count - 1)
			resultline += " ";
	}

	/* Finish the results. */
	resultline += ">";
	results->AddMsg(resultline);
	if (successes > 0 )
		results->AddMsg("<Successes: " + Str(successes) + ">");
	else
		results->AddMsg("<Failure>");

}



void RollEngine::RollNWODChance()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Add the descriptive line and start the result line. */
	results->AddMsg("<New World of Darkness chance die roll" + For() + ">" + message);

	/* Perform the roll. */
	double die = RollTheBones(1, 10);
	if (die == 10)
		results->AddMsg("<SUCCESS!>");
	else if (die == 1)
		results->AddMsg("<DRAMATIC FAILURE!>");
	else
		results->AddMsg("<Failure: " + Str(die) + ">");

}



void RollEngine::RollDND2EInit()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];


	/* Add the descriptive line and start the result line. */
	std::string initrslt = "<D&D Initiative roll" + For();
	initrslt += ": " + Str(RollTheBones(1,10)) + "> " + message;
	results->AddMsg(initrslt);
}



void RollEngine::RollDNDAlias()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* The label to use is the expression in lowercase. */
	std::string label = roll->expression[0];
	std::transform(label.begin(), label.end(), label.begin(), tolower);

	/* Add result. */
	results->AddMsg("<D&D " + label + " roll" + For() + ": " + Str(RollTheBones(1, 20)) + ">" + message);
}



void RollEngine::RollEEBarrel()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddAction("roars and beats its chest");
		results->AddAction("throws a barrel at " + roll->extra[0]);
		if (roll->outputtype == IRC_CHAN)
			results->AddNPC("BotServ", "Do a barrel roll!");

		// Roll save vs barrel.
		double save = RollTheBones(1, 20);

		// Did they save?
		if (save > 5)
		{
			results->AddAction("roars angrily as the barrel bounces past " + roll->extra[0]);
			if (roll->outputtype == IRC_CHAN)
				results->AddNPC("BotServ", "You did it! I was worried for a moment.");
		}
		else
		{
			results->AddAction("roars in triumph as the barrel hits " + roll->extra[0]);
			results->AddError("Game over.");
		}
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEDownTheStairs()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		if (roll->outputtype == IRC_CHAN)
			results->AddKick("shoves you down the stairs");
		results->AddError("Have a nice trip! See you next fall!");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEInTheHay()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddAction("beats the \2hell\2 out of " + roll->extra[0]);
		results->AddMsg("I'm \2NOT\2 that kind of \2SERVICE\2 SICKO!");
		results->AddError("Try OperServ or something; he might be into that.");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEJoint()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		/* Decrement Fuzz Factor. */
		fuzzfactor--;

		/* Give them one. */
		results->AddAction("passes a 'cigarette' to " + roll->extra[0]);

		/* Check Fuzz Factor to find out if they get busted. */
		if (fuzzfactor > 0)
			results->AddError("If you get busted, I never saw you!");
		else
		{
			/* Oh dear. */
			results->AddAction("suddenly looks terrified, exclaims \"We never met!\", then jumps out a window");

			/* Sentencing! */
			double shuntime = RollTheBones(1, 600);
			if (roll->outputtype == IRC_CHAN)
			{
				results->AddNPCA("OperServ", "busts into the room with guns blazing");
				results->AddNPC("OperServ", "\002FREEZE!\002 " + roll->extra[0] + ", this is a raid!");

				std::string sentencing = "sentences " + roll->extra[0] + " to " + Str(floor(shuntime/60));
				sentencing += " minutes, " + Str(shuntime - floor(shuntime/60)*60) + " seconds in prison";
				results->AddNPCA("OperServ", sentencing);
			}
			results->AddShun("Say no to drugs!", shuntime);

			/* And Fuzz Factor resets for next time. */
			fuzzfactor = RollTheBones(1, 100);
		}
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEJointFuzzFactor()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		/* Check we have the required number of parameters. */
		if (roll->expression.size() < 2)
		{
			results->Clear();
			results->AddError("Error: Fuzz Factor setting requires an additional parameter giving what to set the Fuzz Factor to.");
			throw new RollException;
		}

		/* Get the value to set Fuzz Factor to. */
		fuzzfactor = round(ReadExpression(roll->expression[1]));
		if (fuzzfactor < 1)
		{
			results->AddError("Warning: Fuzz Factor specified zero or negative value, and was set to 1 instead.");

			fuzzfactor = 1;
		}

		/* Tell them it was done. */
		results->AddMsg("<Fuzz Factor set" + For() + " to " + Str(fuzzfactor, roll->expression[1]) + ">");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEOver()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddAction("kicks " + roll->extra[0] + " in the shin.");
		results->AddMsg("What am I, your dog?");
		results->AddError("I am not a dog, moron!");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEERick()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddAction("dances in, and begins to sing");
		results->AddMsg("Never gonna give you up, never gonna let you down...");
		results->AddMsg("Never gonna run around, and desert you! Never gonna make you cry...");
		results->AddMsg("Never gonna say goodbye! Never gonna tell a lie, and hurt you!");
		results->AddMsg("You just got roll ricked. *dances out, leaving " + roll->extra[0] + " a bill*");
		results->AddError("That'll be $750.");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEYourMom()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddMsg("No thanks " + roll->extra[0] + ". Oh, by the way, here's the $20 I owe YOUR mom for last night.");
		results->AddError("Yo Momma!");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



void RollEngine::RollEEYourDad()
{
	if (roll->outputtype == IRC_CHAN || roll->outputtype == IRC_PM || roll->outputtype == IRC_SELF)
	{
		results->AddMsg("No thanks " + roll->extra[0] + ". Your dad would be jealous.");
		results->AddScene("This explains those slurping noises coming from " + roll->extra[0] + "'s father's room last night!");
		results->AddError("Schwing!");
	}
	else
	{
		results->AddError("This easter egg is not available here.");
	}
}



/* We assume that the expression IS a valid repeated expression if this is
 * called. */
void RollEngine::RollRepeatedExpression()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Split the count expression and sub expression out. */
	const char* fullexpression = roll->expression[0].c_str();
	size_t position = strchr(fullexpression, '[') - fullexpression;
	std::string countexpression = roll->expression[0].substr(0, position);
	std::string subexpression = roll->expression[0].substr(position + 1, strlen(fullexpression) - position - 2);

	/* Get the count. */
	double count = round(ReadExpression(countexpression));
	if (count < 1)
	{
		results->AddError("Warning: Repeated roll specified zero or negative repeats, and will be performed once instead.");

		count = 1;
	}
	if (count > 40)
	{
		results->AddError("Warning: Repeated roll specified " + Str(count) + " repeats, exceeding the maximum, and was capped at the maximum of 40 repeats.");

		count = 40;
	}

	/* Add the descriptive line and start the result line. */
	results->AddMsg("<Repeated roll" + For() + " [Expression: " + subexpression + ", Repeats: " + Str(count, countexpression) + "]>" + message);
	std::string resultline = "<";

	/* Handle special RPG expressions the parser cannot handle. */
	/* These must not be system-specific presets, must not produce more
	 * than one result, and must not require a special output format. */
	if (subexpression == "%")
	{
		/* "%" means 1d100. */
		for (size_t i = 0; i < count; i++)
		{
			resultline += Str(RollTheBones(1, 100));
			if (i + 1 != count)
				resultline += " ";
		}
	}
	else if (!strcasecmp(subexpression.c_str(), "d%HL") || !strcasecmp(subexpression.c_str(), "%HL"))
	{
		/* "%HL" and "d%HL" mean to perform a traditional percentile
		 * dice roll; two ten-sided, 0-9 dice for tens and ones, with
		 * two 0s indicating a result of 100. */
		/* Yes, this produces the same odds as and is outwardly
		 * indistinguishable from a 1d100. The old-time gamers know,
		 * but old dice superstitions die hard. */
		for (size_t i = 0; i < count; i++)
		{
			double result;

			double tens = RollTheBones(1, 10) - 1;
			double ones = RollTheBones(1, 10) - 1;
			if (!tens && !ones)
				result = 100;
			else
				result = tens * 10 + ones;

			resultline += Str(result);
			if (i + 1 != count)
				resultline += " ";
		}
	}

	/* If it's not a special case, evaluate the given expression the given
	 * number of times, adding the results to the result line. */
	else
	{
		expression->Parse(subexpression.c_str());
		for (size_t i = 0; i < count; i++)
		{
			resultline += Str(expression->Eval());
			if (i + 1 != count)
				resultline += " ";
		}
	}

	/* Add the end of the result line, and add it. */
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::RollExpression()
{
	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 1; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Do the roll... */
	double result = ReadExpression(roll->expression[0]);

	/* Add result. */
	results->AddMsg("<Results" + For() + " [" + roll->expression[0] + "]: " + Str(result) + ">" + message);
}
