/* RollEngine's SCORES-type roll handling. */
#include "rollengine.h"



void RollEngine::DoScores()
{
	if (!strcasecmp(roll->expression[0].c_str(), "D&D") || !strcasecmp(roll->expression[0].c_str(), "DND"))
	{
		ScoresDND();
	}
	else if (!strcasecmp(roll->expression[0].c_str(), "NH") || !strcasecmp(roll->expression[0].c_str(), "NEWHORIZONS") || !strcasecmp(roll->expression[0].c_str(), "HSCORES"))
	{
		ScoresNH();
	}
	else
	{
		results->Clear();
		results->AddError("Unknown system for SCORES: " + roll->expression[0]);
		throw new RollException;
	}
}



void RollEngine::ScoresDND()
{
	if (roll->expression.size() < 2)
	{
		results->Clear();
		results->AddError("A method between 1 and 7 must be specified when generating D&D Ability Scores.");
		throw new RollException;
	}

	/* Extract the method to be used to generate the character scores,
	 * including generating the way this is to be displayed to the user. */
	double method = round(ReadExpression(roll->expression[1]));

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 2; i < roll->expression.size(); i++)
		message += " " + roll->expression[i];

	/* Add the title line above the results of the roll. */
	results->AddMsg("<D&D Ability Scores" + For() + " [Method: " + Str(method, roll->expression[1]) + "]>" + message);

	if (method == 1)
		ScoresDND1();

	else if (method == 2)
		ScoresDND2();

	else if (method == 3)
		ScoresDND3();

	else if (method == 4)
		ScoresDND4();

	else if (method == 5)
		ScoresDND5();

	else if (method == 6)
		ScoresDND6();

	else if (method == 7)
		ScoresDND7();

	else
	{
		results->Clear();
		results->AddError("Unrecognised method for D&D SCORES: " + Str(method, roll->expression[1]));
		throw new RollException;
	}
}



void RollEngine::ScoresDND1()
{
	std::string resultline = "<";

	/* Handle strength. */
	double result = RollTheBones(3, 6);
	resultline += "Str: " + Str(result);
	if (result == 18)
	{
		resultline += "/";
		result = RollTheBones(1, 100);
		if (result == 100)
		{
			resultline += "00";
		}
		else
		{
			convstream << std::setfill('0') << std::setw(2);
			resultline += Str(result);
			convstream << std::setw(0);
		}
	}
	resultline += " ";

	/* Handle the rest. */
	resultline += "Dex: " + Str(RollTheBones(3, 6)) + " ";
	resultline += "Con: " + Str(RollTheBones(3, 6)) + " ";
	resultline += "Int: " + Str(RollTheBones(3, 6)) + " ";
	resultline += "Wis: " + Str(RollTheBones(3, 6)) + " ";
	resultline += "Cha: " + Str(RollTheBones(3, 6));

	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND2()
{
	double resultone;
	double resulttwo;
	double result;
	std::string resultline = "<";

	/* Handle strength. */
	resultone = RollTheBones(3, 6);	resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Str: " + Str(result);
	if (result == 18)
	{
		resultline += "/";
		result = RollTheBones(1, 100);
		if (resultone == 100)
		{
			resultline += "00";
		}
		else
		{
			convstream << std::setfill('0') << std::setw(2);
			resultline += Str(result);
			convstream << std::setw(0);
		}
	}
	resultline += " ";

	/* Handle dexterity. */
	resultone = RollTheBones(3, 6); resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Dex: " + Str(result) + " ";

	/* Handle constitution. */
	resultone = RollTheBones(3, 6); resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Con: " + Str(result) + " ";

	/* Handle intelligence. */
	resultone = RollTheBones(3, 6); resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Int: " + Str(result) + " ";

	/* Handle wisdom. */
	resultone = RollTheBones(3, 6); resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Wis: " + Str(result) + " ";

	/* Handle charisma. */
	resultone = RollTheBones(3, 6); resulttwo = RollTheBones(3, 6);
	result = (resultone > resulttwo) ? resultone : resulttwo;
	resultline += "Cha: " + Str(result);

	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND3()
{
	std::string resultline = "<";
	for (unsigned int i = 0; i < 6; i++)
	{
		resultline += Str(RollTheBones(3, 6));
		if (i != 5)
			resultline += " ";
	}
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND4()
{
	std::string resultline = "<";
	for (unsigned int i = 0; i < 12; i++)
	{
		resultline += Str(RollTheBones(3, 6));
		if (i != 11)
			resultline += " ";
	}
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND5()
{
	double dice[4];
	std::string resultline = "<";
	for (unsigned int i = 0; i < 6; i++) 
	{
		unsigned int lowest = 0;
		for (unsigned int die = 0; die < 4; die++)
		{
			dice[die] = RollTheBones(1, 6);
			if (dice[die] < dice[lowest])
				lowest = die;
		}

		double total = 0;
		for (unsigned int die = 0; die < 4; die++)
			if (die != lowest) total += dice[die];
		
		resultline += Str(total);
		if (i != 5)
			resultline += " ";
	}
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND6()
{
	std::string resultline = "<Add the following dice to attributes, each starting at 8:";
	for (unsigned int i = 0; i < 7; i++)
		resultline += " " + Str(RollTheBones(1, 6));
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresDND7()
{
	std::string resultline = "<Assign three dice to each attribute:";
	for (unsigned int i = 0; i < 18; i++)
		resultline += " " + Str(RollTheBones(1, 6));
	resultline += ">";
	results->AddMsg(resultline);
}



void RollEngine::ScoresNH()
{

	/* The rest of the parameters are an optional message to be displayed
	 * with the roll; put them together for such. */
	std::string message;
	for (size_t i = 2; i < roll->expression.size(); i++)
	{
		message += " " + roll->expression[i];
	}

	/* Add the title line above the results of the roll. */
	results->AddMsg("<New Horizons Ability Scores" + For() + ">" + message);
	std::string resultline = "<";

	/* Add most powers. */
	resultline += "Physical Power: " + Str(RollTheBones(3, 10) + 10) + " "; 
	resultline += "Psychic Power: " + Str(RollTheBones(3, 10)) + " "; 
	resultline += "Magic Power: " + Str(RollTheBones(3, 10)) + " ";
	
	/* Add alteration power. */
	double result = RollTheBones(3, 10) - 20; if (result < 0) result = 0;
	resultline += "Alteration Power: " + Str(result) + " ";

	/* Add most resistances. */
	resultline += "Physical Resistance: " + Str(RollTheBones(3, 10)) + " ";
	resultline += "Psychic Resistance: " + Str(RollTheBones(3, 10)) + " ";
	resultline += "Magic Resistance: " + Str(RollTheBones(3, 10)) + " ";

	/* Add alteration resistance. */
	result = RollTheBones(3, 10) - 20; if (result < -5) result = -5;
	resultline += "Alteration Resistance: " + Str(result);

	resultline += ">";
	results->AddMsg(resultline);
}
