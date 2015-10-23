/* RollEngine main header file. */
#ifndef __ROLLENGINE_H__
#define __ROLLENGINE_H__

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "expressionparser.h"

/* Define string comparison function for Windows. This is ugly, but needed
 * because there's no portable way of doing this. */
#ifdef _WIN32
#define strcasecmp _stricmp
#endif


/* Roll types. These define how the expression is to be interpreted.
 * - CALC: To be interpreted as a mathematical expression, possibly including
 *   dice rolls. No presets or similar.
 * - ROLL: To be interpreted as a normal dice roll, either an expression as
 *   compatible with CALC, or one of the supported preset rolls.
 * - SCORES: To be interpreted as a request for a set of character scores to be
 *   randomly generated for the specified system and method. */
enum RollType { CALC, ROLL, SCORES };



/* Roll output types. These allow the engine to customise its output for
 * different uses, and to include extra information based on the situation in
 * its output. This extra information is required, for the rolls that have it.
 * - A PLAIN roll is a generic roll, and has no extra information.
 * - An IRC_SELF roll is a roll shown only to the requester on IRC, and expects
 *   the requester's nick.
 * - An IRC_CHAN roll is a roll sent to an IRC channel, and expects the
 *   requester's nick, and the target channel.
 * - An IRC_PM roll is a roll sent privately to another user on IRC, and expects
 *   the requester's nick and the target user.
 *   name. */
enum RollOutputType { PLAIN, IRC_SELF, IRC_CHAN, IRC_PM };



/* Result type. This specifies the type of a given result line, and thus how it
 * is meant to be displayed. In fitting with the roleplaying design of
 * RollEngine, results may be displayed or expressed in a number of ways,
 * depending on the output type. Each type may have multiple strings added to
 * the data vector per type added to the type vector, providing multiple pieces
 * of information for a single line or result effect.
 * - ERR: This is a message to be shown to the requester of the roll to tell
 *   them of an error in or an error performing the roll. This must be supported
 *   by all output types.
 * - MSG: This is a straightforward result message. This must be supported by
 *   all output types.
 * - ACTION: This is a message to be displayed as an action, or an emote, by the
 *   the rolling system. This is must be supported for all IRC output types.
 * - NPC: This is a message from an NPC. The data vector contains the message,
 *   and the name for the NPC. This must be supported for IRC_CHAN.
 * - NPCA: This is a message displayed as an action by an NPC. The data vector
 *   contains the action text, and the name for the NPC. This must be supported
 *   for IRC_CHAN.
 * - SCENE: This is a message displayed as a SCENE message. The data vector
 *   contains the message.
 * - KICK: This line is an instruction for the requester to be removed from the
 *   channel they are messaging. The data vector contains the reason for the
 *   removal. This must be supported for IRC_CHAN.
 * - SHUN: This line is an instruction for the requester to be shunned. The
 *   data vector contains the duration of the shun, and the reason. This must
 *   be supported for all IRC output types. */
enum RollResultType { ERR, MESSAGE, ACTION, NPC, NPCA, SCENE, KICK, SHUN };



/* Class declarations. */
class Roll;
class RollResults;
class RollException;
class ExpressionParser;
class StackDepthCounter;
class RollEngine;



/* Roll Class */
/* Stores information on a single roll to perform. RollEngine takes a const
 * reference to one as its task to perform. */
class Roll
{
 public:
	/* The type of roll; determines how expression is to be used. */
	RollType type;

	/* The output type; used to customise the results returned to the case
	 * RollEngine is being used in, and to tell RollEngine what extra
	 * information is available for it to include. */
	RollOutputType outputtype;

	/* The expression to be rolled. The first string must be present, and
	 * may be a mathematical equation, or some other instructions
	 * appropriate to the roll type. The second and subsequent provide
	 * extra parameters. */
	std::vector<std::string> expression;

	/* Extra information for the output type that RollEngine may use in
	 * generating messages. */
	std::vector<std::string> extra;
};



/* RollResults Class */
/* Stores the results for a roll. RollEngine takes a reference to one to fill
 * with results. */
class RollResults
{
 public:
	/* The types of each line of results. There is one entry here per result
	 * line. These are set by RollEngine and used to tell the caller how 
	 * to display its output. This should be iterated over, and the data
	 * for each line pulled out, in order. */
	std::list<RollResultType> types;

	/* The results data. Some types of result line may have multiple pieces
	 * of data, rather than one, associated with them, in order to provide
	 * additional information. */
	std::list<std::string> data;

	/* Functions to add lines to the results. */
	/* Will not check that these are appropriate for the output type. */
	void AddError(const std::string& msg);
	void AddMsg(const std::string& msg);
	void AddAction(const std::string& action);
	void AddNPC(const std::string& npc, const std::string& msg);
	void AddNPCA(const std::string& npc, const std::string& action);
	void AddScene(const std::string& msg);
	void AddKick(const std::string& reason);
	void AddShun(const std::string& reason, const int& duration);

	/* Function to clear the results. */
	/* Used before adding fatal error messages. */
	void Clear();
};



/* RollException is a do-nothing class that is simply thrown to terminate
 * processing of a roll on error. The function throwing it is responsible for
 * clearing the previous messages from the roll results object, and then adding
 * error messages for the fatal error, beforehand. This is caught by RollEngine
 * in Run(), which simply returns. */
class RollException {};



/* ExpressionParser contains the state of a single expression, and provides
 * expression parsing functionality. It provides functions to parse an
 * expression in infix (that is, standard mathematical) format from a string,
 * and then to evaluate the stored expression.
 *
 * RollEngine's expression parser is designed for efficiency. It may and
 * should be reused for new expressions after the result of a previous
 * expression is has been retrieved, and will never allocate/deallocate memory
 * on the heap outside of construction and destruction. */
class ExpressionParser
{
 friend class StackDepthCounter;

 public:
	/* Constructor. Initalises the parent RE. */
	ExpressionParser(RollEngine* RE) { engine = RE; }

	/* Take an infix expression in a string and parses it, storing the
	 * result inside the parser for evaluation.
	 * Throws RollException for errors in the expression. */
	void Parse(const char* expression);

	/* Evaluates the current expression stored in the parser. Must only be
	 * called after a successful Parse() call which did not trigger an
	 * exception. Returns the result of evaluation as a double. */
	double Eval();

 private:
	/* The parent RollEngine. This engine has error messages added on
	 * exception to its roll, its math functions used, and so forth, and is
	 * assumed to be the caller. */
	RollEngine* engine;

	/* Current parsed expression. */
	ExpToken Expression[MAX_EXP_TOKENS];
	size_t ExpressionLength;

	/* Current parsing state. */
	const char* string;
	const char* ParsePosition;
	ExpToken CurrentToken;
	size_t ParseStackDepth;
	char TextToken[MAX_TEXT_TOKEN];

	/* Current evaluation state. */
	double EvalStack[MAX_NUM_STACK];
	size_t EvalPosition;

	/* Parsing subfunctions. */
	/* These are called by Parse(). Each implements a level of precedence,
	 * calling down to the level below them before adding tokens on
	 * their level. */ 
	void Level1();  /* Addition and subtraction. */
	void Level2();  /* Multiplication and division. */
	void Level3();  /* Exponent operator. */
	void Level4();  /* Unary minus (plus is ignored). */
	void Level5();  /* The dice operator. */
	void Level6();  /* Numbers. */

	/* Adds a token to the currently parsed expression. */
	void AddToken(ExpToken& token);

	/* Reads the next token from the current string being parsed into
	 * CurrentToken. */
	void ReadToken();

	/* Generate an error message highlighting the position of the error
	 * in the expression, along with the passed message, set it, and
	 * throw a roll exception. */
	void ThrowParseError(const char* message);
};



/* Stack depth counter; every time this is instantiated, it increases the
 * recorded stack depth. When it's destroyed, it decrements it. If the stack
 * depth goes over limits, this throws an exception. An instance is created at
 * the start of all the functions that recurse in the expression parser; that
 * is, all the Level* functions and ReadToken. */
class StackDepthCounter
{
 public:
	StackDepthCounter(ExpressionParser* const parent)
	{
		EP = parent;
		EP->ParseStackDepth++;
		if (EP->ParseStackDepth > MAX_STACK_DEPTH)
			EP->ThrowParseError("Maximum recursion depth exceeded:");
	}
	~StackDepthCounter() { EP->ParseStackDepth--; }

 private:
	ExpressionParser* EP;
};



/* RollEngine class. */
class RollEngine
{
 friend class ExpressionParser;

 public:
	/* Constructor; initialises the seed, spawns the expression parser,
	 * and initialise the constants. */
	RollEngine();

	/* Destructor; delete the expression parser. */
	~RollEngine() { delete expression; }

	/* Top-level function called to process a roll. */
	void Run(const Roll& roll, RollResults& results);

 private:
	
	/* The expression parser instance used by RollEngine. */
	ExpressionParser* expression;

	/* Variables kept between rolls. */
	unsigned int seed; /* Used as a running seed for ran(). */
	double fuzzfactor; /* Used for the "joint" easter egg. */

	/* Variables set for each roll. */
	const Roll* roll;
	RollResults* results;

	/* Variables used temporarily with different contents during processing
	 * a roll. */
	std::stringstream convstream;
	std::string convstring;
	std::string forstring;
	unsigned int warning_count;

	/* Handle ROLL-type rolls. */
	void DoRoll();
	void RollCraps();
	void RollD20();
	void RollExalted(bool double_successes);
	void RollNewHorizons();
	void RollRTD();
	void RollShadowrun();
	void RollWOD();
	void RollRWOD();
	void RollNWOD();
	void RollNWODChance();
	void RollDND2EInit();
	void RollDNDAlias();
	void RollEEBarrel();
	void RollEEDownTheStairs();
	void RollEEInTheHay();
	void RollEEJoint();
	void RollEEJointFuzzFactor();
	void RollEEOver();
	void RollEERick();
	void RollEEYourMom();
	void RollEEYourDad();
	void RollRepeatedExpression();
	void RollExpression();

	/* Handle SCORES-type rolls. */
	void DoScores();
	void ScoresDND();
	void ScoresDND1();
	void ScoresDND2();
	void ScoresDND3();
	void ScoresDND4();
	void ScoresDND5();
	void ScoresDND6();
	void ScoresDND7();
	void ScoresNH();

	/* Reads the given string as an expression, returning its numerical
	 * value. Used for basic rolls, and for numerical parameters. */
	double ReadExpression(const std::string& expression);

	/* Increment the warning count, and return true if another warning can
	 * be added, or false if the maximum number of warnings has been
	 * reached. */
	bool IncWarningCount();

	/* Return a "for <name>" string or similar byline if appropriate for the
	 * output type for the author to be indicated in such a manner, or
	 * an empty string otherwise. Uses forstring to avoid allocating a
	 * string when called, and must not be used twice in the same line as a
	 * result. */
	const std::string& For();

	/* Convert a number to a string. Uses convstring to avoid allocating a
	 * string when called, and it and the other Str() must not be used twice
	 * in the same line as a result. */
	const std::string& Str(double number);
	
	/* Convert a number corresponding to an evaluated input string, to a
	 * string, adding the original input in brackets if they aren't
	 * identical. Uses convstring to avoid allocating a string when called,
	 * and it and the other Str() must not be used twice in the same line
	 * as a result. */
	const std::string& Str(double number, const std::string& input);

	/* Roll functions; used to roll dice! */
	/* RollTheBones may add warnings to the results if numbers exceeding
	 * its limits are provided. */
	double RollTheBones(double count, double sides);
	unsigned int Random(unsigned int max);

	/* Basic math functions, each taking and returning a double. These are
	 * used in parsing expressions to implement support for the math
	 * functions of the same name, and outside of this where required. */
	/* See external helpop documentation for what each of these do. */
	double deg(double);
	double fac(double);
	double rad(double);
	double ran(double);
	double sqr(double);
	
	/* Basic math functions which are just wrappers around their C library
	 * equivalents. */
	/* See external helpop documentation for what each of these do. */
	double abs(double);
	double acos(double);
	double asin(double);
	double atan(double);
	double ceil(double);
	double cos(double);
	double cosh(double);
	double exp(double);
	double floor(double);
	double log(double);
	double log10(double);
	double round(double);
	double sin(double);
	double sinh(double);
	double sqrt(double);
	double tan(double);
	double tanh(double);
	double trunc(double);

	/* Constant map, to which supported constants are added. */
	std::map<const char*, double, TextTokenCompare> constants;

	/* Function map, containing mathematical functions. */
	std::map<const char*, double(RollEngine::*)(double), TextTokenCompare> functions;
};

#endif
