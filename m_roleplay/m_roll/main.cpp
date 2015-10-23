/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2009 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 *
 * Written for InspIRCd by Namegduf (J Beshir) and Taros
 * Copyright © 2009-2012 John Beshir and Taros
 * Module designed for Tel'Laerad <http://tellaerad.net>
 */

#include <queue>

#include "inspircd.h"
#include "modules.h"
#include "xline.h"

#include "rollengine.h"
#include "rollcallback.h"

/* $ModDesc: Provides the /ROLL and /SCORES commands
 * which allow for making rolls and generating character
 * scores over IRC.
 */

class ModuleRoll;
class RollThread;
class UserRoll;
class UserRollResults;

class RollRestrict;
class CommandRoll;
class CommandScores;
class CommandRollmsg;



/* UserRoll class. */
/* A roll with associated information on the requesting user and the target. */
class UserRoll : public Roll
{
 public:
	/* The UUID of the user requesting the roll. */
	std::string source;

	/* The target of the roll; either - for none but the requester, a UUID
	 * for a user, or a channel name for a channel. */
	std::string target;
};



/* UserRollResults class. */
/* A roll result with associated information on the requesting user and the
 * target. */
class UserRollResults : public RollResults
{
 public:
	/* The UUID of the user who requested the roll. */
	std::string source;

	/* The target of the roll; either - for none but the requester, a UUID
	 * for a user, or a channel name for a channel. */
	std::string target;
};



/* Module class. */
class ModuleRoll : public Module
{
 private:
	
	CommandRoll *rollcommand;
	CommandScores *scorescommand;
	CommandRollmsg *rollmsgCommand;
	RollRestrict *rr;
	std::vector<std::pair<Module*, RollChanCallback*> > chan_cbs;

	/* Function called to display a set of results locally. */
	/* Only one or neither of targetuser or targetchan may be non-NULL. */
	void DisplayResults(User *user, User *targetuser, Channel *targetchan, const RollResults& results);

 public:
	
	RollThread *Roller;
	
	ModuleRoll();
	~ModuleRoll();
	virtual Version GetVersion();
	virtual void On005Numeric(std::string &output);
	virtual void OnRequest(Request &request);
	virtual void OnUnloadModule(Module* mod);
	virtual char* OnSaveState();
	virtual void OnRestoreState(const char* state);

	/* Send the results of a roll, locally and remotely. */
	void SendResults(User *user, User *targetuser, Channel *targetchan, const RollResults& results);

	/* Handle a remotely-received roll. */
	void RemoteResults(User *user, User *targetuser, Channel *targetchan, const RollResults& results);
};



/* Roll Thread Class */
/* Handles creating and communicating with a rolling thread. */
/* The main thread may only use public methods of this class to interact with
 * the rolling thread. The public methods must use mutexes to access private
 * variables of the class. The constructor and destroyer are both exceptions
 * to this, as the roll thread cannot be running while they are.
 * The rolling thread may not access anything other than private variables and
 * functions of this class. For variables with mutexes for access by the main
 * thread, it must use mutexes when accessing them. */
class RollThread : public SocketThread
{
 public:
	InspIRCd* ServerInstance;
	ModuleRoll* ModuleInstance;

	RollThread(InspIRCd* Instance, ModuleRoll* Me) : SocketThread(), ServerInstance(Instance), ModuleInstance(Me) { }	
	bool AddRoll(UserRoll* roll);
	UserRollResults* GetRollResults();
	virtual void OnNotify();

 private:

	/* The main thread creates a UserRoll instance, and adds it to the
	 * incoming queue, after which it must not access the instance. The roll
	 * thread runs the roll, then deletes it.
	 * The roll thread then creates a UserRollResults instance, and adds it
	 * to the outgoing queue, after which it must not access the instance.
	 * The main thread handles the results, then deletes them.
	 * In case of shutdown, the main thread must not access either queue
	 * after setting the exit flag of the roll thread. The roll thread will
	 * delete all remaining entries in both queues before exiting. */
	std::queue<UserRoll*> IncomingQueue; /* Mutexed by this->LockQueue() */
	std::queue<UserRollResults*> OutgoingQueue; /* Mutexed by OutgoingQueueMutex */
	Mutex OutgoingQueueMutex;

	RollEngine RE;

	virtual void Run();
};



/* Handle channel mode +d for restricting dice rolls. */
class RollRestrict : public ModeHandler
{
 public:
	RollRestrict(ModuleRoll *me) : ModeHandler(me, "dice", 'd', PARAM_SETONLY, MODETYPE_CHANNEL) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding)
	{
		if (adding)
		{
			std::string cur_param = channel->GetModeParameter('d');
			if (cur_param == parameter)
			{
				// This is necessary to avoid the server reapplying the mode
				// on every remote join.
				return MODEACTION_DENY;
			}
			if (IS_LOCAL(source))
			{
				
				if (channel->GetPrefixValue(source) < HALFOP_VALUE)
				{
					source->WriteNumeric(482, "%s %s :You must be at least a half-operator to change modes on this channel.", source->nick.c_str(), channel->name.c_str());
					return MODEACTION_DENY;
				}
				if (parameter != "v" && parameter != "o" && parameter != "b")
				{
					source->WriteNumeric(403, "%s %s :Invalid roll restriction.", source->nick.c_str(), channel->name.c_str());
					return MODEACTION_DENY;
				}
			}

			channel->SetModeParam('d', parameter);
			return MODEACTION_ALLOW;
		}
		else
		{
			if (channel->IsModeSet('d'))
			{
				channel->SetModeParam('d', "");
				return MODEACTION_ALLOW;
			}
		}

		return MODEACTION_DENY;
	}
};



/* Function checking that a user has permission to send to their target(s). */
bool CanRoll(User *user, User *targetuser, Channel* targetchan)
{
	if (targetchan)
	{
		if(!targetchan->HasUser(user))
		{
			user->WriteServ("482 %s %s :You're not on that channel.", user->nick.c_str(), targetchan->name.c_str());
			return false;
		}

		if (targetchan->GetExtBanStatus(user, 'd') == MOD_RES_ALLOW)
		{
			// They're excepted, let 'em through regardless of other factors.
			return true;
		}

		if (targetchan->GetExtBanStatus(user, 'd') == MOD_RES_DENY)
		{
			// Matching extban, deny them.
			user->WriteServ("482 %s %s :You're banned from using ROLL and SCORES.", user->nick.c_str(), targetchan->name.c_str());
			return false;
		}

		if (targetchan->IsModeSet('d'))
		{
			std::string level = targetchan->GetModeParameter('d');
			if (level == "v" && targetchan->GetPrefixValue(user) < VOICE_VALUE)
			{
				user->WriteServ("482 %s %s :You're not voiced or a channel (half)operator, and ROLL and SCORES are limtied to voiced users and above in this channel.", user->nick.c_str(), targetchan->name.c_str());
				return false;
			}
			else if (level == "o" && targetchan->GetPrefixValue(user) < HALFOP_VALUE)
			{
				user->WriteServ("482 %s %s :You're not a channel (half)operator, and ROLL and SCORES are limited to channel operators in this channel.", user->nick.c_str(), targetchan->name.c_str());
				return false;
			}
			else if (level == "b")
			{
				user->WriteServ("482 %s %s :ROLL and SCORES are disabled on this channel.", user->nick.c_str(), targetchan->name.c_str());
				return false;
			}
		}
	}

	return true;
}



/* Handle /ROLL */
class CommandRoll : public Command
{
 private:
	ModuleRoll* ModuleInstance;

 public:
	CommandRoll (ModuleRoll* Me) : Command(Me, "ROLL", 1)
	{
		this->ModuleInstance = Me;
		syntax = "[target] <expression|preset roll> [preset roll parameters] [optional text]";
		TRANSLATE3(TR_NICK, TR_TEXT, TR_END);
	}

	CmdResult Handle (const std::vector<std::string>& params, User *user)
	{
		User *targetuser = NULL;
		Channel *targetchan = NULL;
		int rollstart = 1;

		/* If a target other than themselves is specified, look for
		 * it. */
		if (params.size() > 1 && params[0] != "-"
				&& params[0] != user->nick)
		{
			targetuser = ServerInstance->FindNickOnly(params[0]);
			if (targetuser == NULL)
			{
				targetchan = ServerInstance->FindChan(params[0]);
				if (targetchan == NULL)
				{
					user->WriteNumeric(401, "%s %s :No such nick/channel",user->nick.c_str(), params[0].c_str());
					return CMD_FAILURE;		
				}
			}
				
		}
		else if (params.size() == 1)
		{
			rollstart = 0;
		}
		
		/* Check we have permission to roll. */
		if (!CanRoll(user, targetuser, targetchan))
			return CMD_FAILURE;
		
		/* Check the user has permission if they're doing any restricted
		 * rolls. */
		if (!user->HasPrivPermission("roll/fuzzfactor") && params[rollstart] == "fuzzfactor")
		{
			user->WriteServ("481 %s :Permission Denied - ROLL FUZZFACTOR requires the roll/fuzzfactor priv.", user->nick.c_str());
			return CMD_FAILURE;
		}

		/* Create roll. */
		UserRoll *roll = new UserRoll;
		roll->type = ROLL;

		/* Set the roll expression. */
		for (size_t i = rollstart; i < params.size(); i++)
			roll->expression.push_back(params[i]);
		roll->source = user->uuid;
		roll->extra.push_back(user->nick);

		/* Set roll target. */
		if (targetchan)
		{
			roll->outputtype = IRC_CHAN;
			roll->target = targetchan->name;
			roll->extra.push_back(targetchan->name);
		}
		else if (targetuser)
		{
			roll->outputtype = IRC_PM;
			roll->target = targetuser->uuid;
			roll->extra.push_back(targetuser->nick);
		}
		else
		{
			roll->outputtype = IRC_SELF;
			roll->target = "-";
		}

		/* Add roll to queue. */
		bool added = ModuleInstance->Roller->AddRoll(roll);
		if (!added)
		{
			std::string errsource = "=Roll=!" + user->nick + "@" + "roll.fakeuser.invalid";
			user->Write(":%s NOTICE %s :%s", errsource.c_str(), user->nick.c_str(), "Error: Unable to add roll, because the rolling system is extremely busy. Please try again momentarily.");
		}

		return CMD_SUCCESS; 
	}

	/* Return LOCALONLY; we handle propagation ourselves. */
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters)
	{
		return ROUTE_LOCALONLY;
	}
};



/* Handle /SCORES */
class CommandScores : public Command
{
 private:
	ModuleRoll* ModuleInstance;

 public:
	CommandScores (ModuleRoll* Me) : Command(Me, "SCORES", 3)
	{
		this->ModuleInstance = Me;
		syntax = "<target> <system> [system-specific method or options] [optional text]";
		TRANSLATE3(TR_NICK, TR_TEXT, TR_END);
	}

	CmdResult Handle (const std::vector<std::string>& parameters, User *user)
	{
		User *targetuser = NULL;
		Channel *targetchan = NULL;

		/* If a target other than themselves is specified, look for
		 * it. */
		if (parameters[0] != "-" && parameters[0] != user->nick)
		{
			targetuser = ServerInstance->FindNickOnly(parameters[0]);
			if (targetuser == NULL)
			{
				targetchan = ServerInstance->FindChan(parameters[0]);
				if (targetchan == NULL)
				{
					user->WriteNumeric(401, "%s %s :No such nick/channel",user->nick.c_str(), parameters[0].c_str());
					return CMD_FAILURE;		
				}
			}
				
		}
		
		/* Check we have permission to roll. */
		if (!CanRoll(user, targetuser, targetchan))
			return CMD_FAILURE;

		/* Create roll. */
		UserRoll *roll = new UserRoll;
		roll->type = SCORES;

		/* Set the roll expression. */
		for (size_t i = 1; i < parameters.size(); i++)
			roll->expression.push_back(parameters[i]);
		roll->source = user->uuid;
		roll->extra.push_back(user->nick);

		/* Set roll target. */
		if (targetchan)
		{
			roll->outputtype = IRC_CHAN;
			roll->target = targetchan->name;
			roll->extra.push_back(targetchan->name);
		}
		else if (targetuser)
		{
			roll->outputtype = IRC_PM;
			roll->target = targetuser->uuid;
			roll->extra.push_back(targetuser->nick);
		}
		else
		{
			roll->outputtype = IRC_SELF;
			roll->target = "-";
		}

		/* Add roll to queue. */
		ModuleInstance->Roller->AddRoll(roll);

		return CMD_SUCCESS; 
	}

	/* Return LOCALONLY; we handle propagation ourselves. */
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters)
	{
		return ROUTE_LOCALONLY;
	}
};


/* Handle ROLLMSG messages for remote rolls. */
class CommandRollmsg : public Command
{
private:
	ModuleRoll *ModuleInstance;

	/* Stores partially-received rolls from remote servers, which will be
	 * displayed as a single block once received completely. */
	std::map<std::string, RollResults> incomingrolls;

public:
	CommandRollmsg(ModuleRoll* module) : Command(module, "ROLLMSG", 4, 7)
	{
		this->ModuleInstance = module;
		this->flags_needed = FLAG_SERVERONLY;
	}

	CmdResult Handle(const std::vector<std::string>& parameters, User *unusedUser)
	{
		/* Handle the various stages of a remote roll. Note that they
		 * MUST be sent all at once; this just allows receipt to be
		 * divided. */
		if (parameters[0] == "STARTEND")
		{
			User *user = NULL;
			User *targetuser = NULL;
			Channel *targetchan = NULL;

			if (parameters.size() < 6)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has too few parameters and is malformed; ignoring it.");	
				return CMD_FAILURE;
			}

			if (incomingrolls.find(parameters[1]) != incomingrolls.end())
			{
				ServerInstance->Logs->Log("m_roleplay", DEBUG, "ROLLMSG STARTEND from a server which already had a roll in progress. This should only happen if the server split midroll and reconnected. Forgetting about previous roll and continuing.");
				incomingrolls.erase(parameters[1]);
			}

			user = ServerInstance->FindUUID(parameters[2]);
			if (!user)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND source user (%s) unknown; ignoring.", parameters[2].c_str());	
				return CMD_FAILURE;
			}

			targetuser = ServerInstance->FindUUID(parameters[3]);
			if (!targetuser)
			{
				targetchan = ServerInstance->FindChan(parameters[3]);
				if (!targetchan)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND target (%s) unknown; ignoring.", parameters[3].c_str());
					return CMD_FAILURE;
				}
			}

			RollResults results;
			
			if (parameters[4] == "M")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(MESSAGE);
				results.data.push_back(parameters[5]);
			}
			else if (parameters[4] == "A")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(ACTION);
				results.data.push_back(parameters[5]);
			}
			else if (parameters[4] == "N")
			{
				if (parameters.size() != 7)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(NPC);
				results.data.push_back(parameters[5]);
				results.data.push_back(parameters[6]);
			}
			else if (parameters[4] == "NA")
			{
				if (parameters.size() != 7)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(NPCA);
				results.data.push_back(parameters[5]);
				results.data.push_back(parameters[6]);
			}
			else if (parameters[4] == "S")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG STARTEND has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(SCENE);
				results.data.push_back(parameters[5]);
			}
			else
			{
				ServerInstance->Logs->Log("m_roleplay", DEBUG, "ROLLSTARTEND roll type (%s) unknown; ignoring.", parameters[4].c_str());	
				return CMD_FAILURE;
			}
			
			/* Handle the complete roll. */
			ModuleInstance->RemoteResults(user, targetuser, targetchan, results);
		}
		
		else if (parameters[0] == "START")
		{
			if (parameters.size() < 4)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has too few parameters and is malformed; ignoring it.");
				return CMD_FAILURE;
			}

			if (incomingrolls.find(parameters[1]) != incomingrolls.end())
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START from a server which already had a roll in progress. This should only happen if the server split midroll and reconnected. Forgetting about previous roll and continuing.");
				incomingrolls.erase(parameters[1]);
			}

			RollResults results;

			if (parameters[2] == "M")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;

				}
				results.types.push_back(MESSAGE);
				results.data.push_back(parameters[3]);
			}
			else if (parameters[2] == "A")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(ACTION);
				results.data.push_back(parameters[3]);
			}
			else if (parameters[2] == "N")
			{
				if (parameters.size() != 5)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(NPC);
				results.data.push_back(parameters[3]);
				results.data.push_back(parameters[4]);
			}
			else if (parameters[2] == "NA")
			{
				if (parameters.size() != 5)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(NPCA);
				results.data.push_back(parameters[3]);
				results.data.push_back(parameters[4]);
			}
			else if (parameters[2] == "S")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results.types.push_back(SCENE);
				results.data.push_back(parameters[3]);
			}
			else
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG START roll type (%s) unknown; ignoring.", parameters[2].c_str());
				return CMD_FAILURE;
			}
			
			incomingrolls.insert(std::pair<std::string, RollResults>(parameters[1], results));
		}
		
		else if (parameters[0] == "MIDDLE")
		{
			if (parameters.size() < 4)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has too few parameters and is malformed; ignoring it.");
				return CMD_FAILURE;
			}

			std::map<std::string, RollResults>::iterator results = incomingrolls.find(parameters[1]);
			if (results == incomingrolls.end())
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE from a server with no roll in progress. Discarding it.");
				return CMD_FAILURE;
			}

			if (parameters[2] == "M")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(MESSAGE);
				results->second.data.push_back(parameters[3]);
			}
			else if (parameters[2] == "A")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(ACTION);
				results->second.data.push_back(parameters[3]);
			}
			else if (parameters[2] == "N")
			{
				if (parameters.size() != 5)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(NPC);
				results->second.data.push_back(parameters[3]);
				results->second.data.push_back(parameters[4]);
			}
			else if (parameters[2] == "NA")
			{
				if (parameters.size() != 5)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(NPCA);
				results->second.data.push_back(parameters[3]);
				results->second.data.push_back(parameters[4]);
			}
			else if (parameters[2] == "S")
			{
				if (parameters.size() != 4)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(SCENE);
				results->second.data.push_back(parameters[3]);
			}
			else
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG MIDDLE roll type (%s) unknown; ignoring.", parameters[3].c_str());	
				return CMD_FAILURE;
			}
		}
		
		else if (parameters[0] == "END")
		{
			User *user = NULL;
			User *targetuser = NULL;
			Channel *targetchan = NULL;

			if (parameters.size() < 6)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has too few parameters and is malformed; ignoring it.");
				return CMD_FAILURE;
			}

			std::map<std::string, RollResults>::iterator results = incomingrolls.find(parameters[1]);
			if (results == incomingrolls.end())
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END from a server with no roll in progress. Discarding it.");
				return CMD_FAILURE;
			}

			user = ServerInstance->FindUUID(parameters[2]);
			if (!user)
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END source user (%s) unknown; ignoring.", parameters[2].c_str());	
				return CMD_FAILURE;
			}

			targetuser = ServerInstance->FindUUID(parameters[3]);
			if (!targetuser)
			{
				targetchan = ServerInstance->FindChan(parameters[3]);
				if (!targetchan)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END target (%s) unknown; ignoring.", parameters[3].c_str());	
					return CMD_FAILURE;
				}
			}

			if (parameters[4] == "M")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(MESSAGE);
				results->second.data.push_back(parameters[5]);
			}
			else if (parameters[4] == "A")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(ACTION);
				results->second.data.push_back(parameters[5]);
			}
			else if (parameters[4] == "N")
			{
				if (parameters.size() != 7)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(NPC);
				results->second.data.push_back(parameters[5]);
				results->second.data.push_back(parameters[6]);
			}
			else if (parameters[4] == "NA")
			{
				if (parameters.size() != 7)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(NPCA);
				results->second.data.push_back(parameters[5]);
				results->second.data.push_back(parameters[6]);
			}
			else if (parameters[4] == "S")
			{
				if (parameters.size() != 6)
				{
					ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END has the wrong number of parameters and is malformed; ignoring it.");
					return CMD_FAILURE;
				}
				results->second.types.push_back(SCENE);
				results->second.data.push_back(parameters[5]);
			}
			else
			{
				ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG END roll type (%s) unknown; ignoring.", parameters[4].c_str());	
				return CMD_FAILURE;
			}

			/* Handle the complete roll. */			
			ModuleInstance->RemoteResults(user, targetuser, targetchan, results->second);
		
			/* And forget about it, now. */
			incomingrolls.erase(results);
		}
		else
		{
			ServerInstance->Logs->Log("m_roll", DEBUG, "ROLLMSG message type (%s) unknown; ignoring.", parameters[0].c_str());	
			return CMD_FAILURE;
		}

		return CMD_SUCCESS;
	}
	
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters)
	{
		return ROUTE_OPT_BCAST;
	}
};


ModuleRoll::ModuleRoll() : Module()
{
	Roller = new RollThread(ServerInstance, this);
	ServerInstance->Threads->Start(Roller);

	rollcommand = new CommandRoll(this);
	ServerInstance->AddCommand(rollcommand);
	scorescommand = new CommandScores(this);
	ServerInstance->AddCommand(scorescommand);
	rollmsgCommand = new CommandRollmsg(this);
	ServerInstance->AddCommand(rollmsgCommand);

	rr = new RollRestrict(this);
	if (!ServerInstance->Modes->AddMode(rr))
		throw ModuleException("Could not add new modes!");

	Implementation eventlist[] = { I_On005Numeric, I_OnUnloadModule };
	ServerInstance->Modules->Attach(eventlist, this, 2);
}



ModuleRoll::~ModuleRoll()
{
	delete(rollcommand);
	delete(scorescommand);
	delete(rollmsgCommand);

	Roller->state->FreeThread(Roller);
	ServerInstance->Modes->DelMode(rr);
	delete rr;
}



Version ModuleRoll::GetVersion()
{
	return Version("Provides roleplaying commands to IRC channels.", VF_COMMON);
}



void ModuleRoll::On005Numeric(std::string &output)
{
	ServerInstance->AddExtBanChar('d');
}



void ModuleRoll::OnRequest(Request &request)
{
	ServerInstance->Logs->Log("m_roll", DEBUG, "[m_roll] Request received (ID = %s)", request.id);
	if (!strcmp(request.id, "ROLLCHANCALLBACK"))
	{
		RegisterRollChanCallbackRequest* rpc = (RegisterRollChanCallbackRequest*)(&request);
		ServerInstance->Logs->Log("m_roll", DEBUG, "[m_roll] Registered channel callback for %s", (*(rpc->source)).ModuleSourceFile.c_str());
		chan_cbs.push_back(std::make_pair(rpc->source, rpc->GetCallback()));
	}
}



void ModuleRoll::OnUnloadModule(Module* mod)
{
	ServerInstance->Logs->Log("m_roll", DEBUG, "[m_roll] Removing all callbacks for module %s (module unloading)", mod->ModuleSourceFile.c_str());
	std::vector<std::pair<Module*, RollChanCallback*> >::iterator i = chan_cbs.begin();
	while (i != chan_cbs.end())
	{
		if (i->first == mod)
			i = chan_cbs.erase(i);
		else
			i++;
	}
}



char* ModuleRoll::OnSaveState()
{
	char *state;

	/* Calculate size of saved state. */
	size_t size = 9;
	for (chan_hash::iterator c = ServerInstance->chanlist->begin(); c != ServerInstance->chanlist->end(); c++)
	{
		if (c->second->GetModeParameter('d') != "")
			size += c->second->name.size() + 3;
	}

	/* Generate state. */
	state = new char[size];
	strcpy(state, "MROLL,1");
	size_t position = 8;
	for (chan_hash::iterator c = ServerInstance->chanlist->begin(); c != ServerInstance->chanlist->end(); c++)
	{
		if (c->second->GetModeParameter('d') != "")
		{
			strcpy(state + position, c->second->name.c_str());
			position += c->second->name.size();
			state[position] = ',';
			state[position + 1] = c->second->GetModeParameter('d')[0];
			state[position + 2] = ' ';
			position += 3;
		}
	}
	state[size-1] = '\0';

	/* Return state. */
	return state;
}



void ModuleRoll::OnRestoreState(const char* state)
{
	/* Check this is our state, and a valid version. */
	if (strncmp(state, "MROLL,", 6))
	{
		/* Invalid state, ignore it. */
		return;
	}
	if (state[8] == '\0')
	{
		/* Empty state, ignore it. */
	}

	/* It is! Assume validity and go! */
	/* Reload all our +d channels! */
	irc::spacesepstream ss(state+8);
	std::string channel;
	while (ss.GetToken(channel))
	{
		char mode[2] = {0,0};
		mode[0] = channel.c_str()[channel.size()-1];
		channel = channel.substr(0, channel.size()-2);
		Channel* chan = ServerInstance->FindChan(channel);
		if (chan != NULL)
		{
			std::vector<std::string> modelist;
			modelist.push_back(channel);
			modelist.push_back("+d");
			modelist.push_back(mode);
			ServerInstance->Modes->Process(modelist, ServerInstance->FakeClient, true);
		}
	}
}



void ModuleRoll::SendResults(User *user, User *targetuser, Channel *targetchan, const RollResults& results)
{	
	/* Display results locally. Will not execute kicks/shuns. */
	DisplayResults(user, targetuser, targetchan, results);

	/* Determine the target servers and target string for propagation. */
	std::string targetservers;
	std::string target;
	if (targetchan)
	{
		targetservers = "*";
		target = targetchan->name;
	}
	else if (targetuser && !IS_LOCAL(targetuser))
	{
		targetservers = targetuser->uuid.substr(0, 3);
		target = targetuser->uuid;
	}

	/* Propagate the roll results, if they require it. */
	if (targetservers != "")
	{
		/* Build a list of roll messages to be sent. */
		std::list<parameterlist> sendlines;
		std::list<std::string>::const_iterator line = results.data.begin();
		for (std::list<RollResultType>::const_iterator i = results.types.begin(); i != results.types.end(); i++)
		{
			parameterlist sendparams;

			/* Ignore non-propagating types... */
			if (*i == ERR || *i == KICK)
			{
				/* Skip one line. */
				line++;
				continue;
			}
			else if (*i == SHUN)
			{
				/* Skip two lines. */
				line++; line++;
				continue;
			}

			/* Handle propagation. */
			if (*i == MESSAGE)
			{
				sendparams.push_back("M");
			}
			else if (*i == ACTION)
			{
				sendparams.push_back("A");
			}
			else if (*i == NPC)
			{
				sendparams.push_back("N");
				sendparams.push_back(*(line++));
			}
			else if (*i == NPCA)
			{
				sendparams.push_back("NA");
				sendparams.push_back(*(line++));
			}
			else if (*i == SCENE)
			{
				sendparams.push_back("S");
			}

			sendparams.push_back(":" + *(line++));
			sendlines.push_back(sendparams);
		}
		
		/* Lines to send assembled, put our SID, the appropriate type,
		 * and the target server on thestart of each and send.
		 * This is, if we have anything to send. */
		if (!sendlines.empty())
		{
			for (std::list<parameterlist>::iterator i = sendlines.begin(); i != sendlines.end(); i++)
			{
				std::list<parameterlist>::iterator next = i;
				next++;
				if (next == sendlines.end())
				{
					i->insert(i->begin(), target);
					i->insert(i->begin(), user->uuid);
					i->insert(i->begin(), ServerInstance->Config->GetSID());
					if (i == sendlines.begin())
						i->insert(i->begin(), "STARTEND");
					else
						i->insert(i->begin(), "END");
				}
				else if (i == sendlines.begin())
				{
					i->insert(i->begin(), ServerInstance->Config->GetSID());
					i->insert(i->begin(), "START");
				}
				else
				{
					i->insert(i->begin(), ServerInstance->Config->GetSID());
					i->insert(i->begin(), "MIDDLE");
				}
			
				i->insert(i->begin(), "ROLLMSG");
				i->insert(i->begin(), targetservers);
			
				ServerInstance->PI->SendEncapsulatedData(*i);
			}
		}
	}

	/* Call callback hooks in other modules. */
	if (targetchan)
	{
		for (std::vector<std::pair<Module*, RollChanCallback*> >::iterator i = chan_cbs.begin(); i != chan_cbs.end(); ++i)
		{
			i->second->OnChanRollResults(user, targetchan, results);
		}
	}

	/* Process and propagate kicks and shuns. These always occur AFTER
	 * all other messages, and are special in that the IRCD handles their
	 * routing. */
	std::list<std::string>::const_iterator line = results.data.begin();
	for (std::list<RollResultType>::const_iterator i = results.types.begin(); i != results.types.end(); i++)
	{
		/* Ignore types displayed/propagated normally... */
		if (*i == ERR || *i == MESSAGE || *i == ACTION || *i == SCENE)
		{
			/* Skip one line. */
			line++;
			continue;
		}
		else if (*i == NPC || *i == NPCA)
		{
			/* Skip two lines. */
			line++; line++;
			continue;
		}

		if (*i == KICK)
		{
			/* We know the user is always local. */
			if (targetchan)
			{
				targetchan->KickUser(ServerInstance->FakeClient, user, (*line++).c_str());
			}
		}

		else if (*i == SHUN)
		{
			XLineFactory* ShunFactory = ServerInstance->XLines->GetFactory("SHUN");
			std::string mask = user->nick + "!" + user->ident + "@" + user->host;
			std::string reason = *line++;
			long duration = atol((*line++).c_str());

			if (!ShunFactory)
			{
				ServerInstance->SNO->WriteToSnoMask('x', "=Roll= would have added a timed shun for %s to expire on %s with the reason \"%s\", but could not find the SHUN XLine type. Is m_shun loaded?", user->nick.c_str(), ServerInstance->TimeString(duration + ServerInstance->Time()).c_str(), reason.c_str());
			}
			else
			{
				XLine* shun = ShunFactory->Generate(ServerInstance->Time(), duration, "=Roll=", reason.c_str(), mask.c_str());
				if (ServerInstance->XLines->AddLine(shun, NULL))
				{
					ServerInstance->SNO->WriteToSnoMask('x', "=Roll= added timed shun for %s, expires on %s: %s", mask.c_str(), ServerInstance->TimeString(duration + ServerInstance->Time()).c_str(), reason.c_str());
				}
			}
		}
	}
}

void ModuleRoll::RemoteResults(User *user, User *targetuser, Channel *targetchan, const RollResults& results)
{
	DisplayResults(user, targetuser, targetchan, results);

	/* Call callback hooks in other modules. */
	if (targetchan)
	{
		for (std::vector<std::pair<Module*, RollChanCallback*> >::iterator i = chan_cbs.begin(); i != chan_cbs.end(); ++i)
		{
			i->second->OnChanRollResults(user, targetchan, results);
		}
	}
}

void ModuleRoll::DisplayResults(User* user, User *targetuser, Channel* targetchan, const RollResults& results)
{
	/* Go over each result, and send it to the its targets on this
	 * server. */
	std::list<std::string>::const_iterator line = results.data.begin();
	for (std::list<RollResultType>::const_iterator i = results.types.begin(); i != results.types.end(); i++)
	{
		/* Ignore results not displayed here... */
		if (*i == KICK)
		{
			/* Skip one line. */
			line++;
		}
		else if (*i == SHUN)
		{
			/* Skip two lines. */
			line++; line++;
		}

		if (*i == ERR)
		{
			std::string source = "=Roll=!" + user->nick + "@" + "roll.fakeuser.invalid";
			user->Write(":%s NOTICE %s :%s", source.c_str(), user->nick.c_str(), line->c_str());
			++line;
		}

		else if (*i == MESSAGE)
		{
			std::string source = "=Roll=!" + user->nick + "@" + "roll.fakeuser.invalid";
			if (targetchan)
			{
				targetchan->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :%s", targetchan->name.c_str(), line->c_str());
			}
			else
				user->Write(":%s NOTICE %s :%s", source.c_str(), user->nick.c_str(), line->c_str());
			
			if (targetuser)
				targetuser->Write(":%s NOTICE %s :%s", source.c_str(), targetuser->nick.c_str(), line->c_str());

			++line;
		}
		
		else if (*i == ACTION)
		{
			std::string source = "=Roll=!" + user->nick + "@" + "roll.fakeuser.invalid";
			if (targetchan)
			{
				targetchan->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :\1ACTION %s.\1", targetchan->name.c_str(), line->c_str());
			}
			else
				user->Write(":%s NOTICE %s :*%s*", source.c_str(), user->nick.c_str(), line->c_str());
	
			if (targetuser)
				targetuser->Write(":%s NOTICE %s :*%s*", source.c_str(), targetuser->nick.c_str(), line->c_str());

			++line;
		}

		else if (*i == NPC)
		{
			std::string source = "\x1F" + (*line++) + "\xF!" + user->nick + "@" + "roll.fakeuser.invalid";
			if (targetchan)
			{
				targetchan->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :%s", targetchan->name.c_str(), line->c_str());
			}

			++line;
		}

		else if (*i == NPCA)
		{
			std::string source = "\x1F" + (*line++) + "\x1F!" + user->nick + "@" + "roll.fakeuser.invalid";
			if (targetchan)
			{
				targetchan->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :\1ACTION %s.\1", targetchan->name.c_str(), line->c_str());
			}

			++line;
		}
		
		else if (*i == SCENE)
		{
			std::string source = "=Scene=!" + user->nick + "@" + "roll.fakeuser.invalid";
			if (targetchan)
			{
				targetchan->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :%s", targetchan->name.c_str(), line->c_str());
			}

			++line;
		}
	}
}



/* Add a roll to the back of the roll thread's incoming queue. */
/* Returns whether the add was rejected due to the queue being full. */
/* Run by main thread. */
bool RollThread::AddRoll(UserRoll* roll)
{
	bool added = 0;
	this->LockQueue();
	if (IncomingQueue.size() < 50)
	{
		IncomingQueue.push(roll);
		added = 1;
	}
	this->UnlockQueueWakeup();
	if (added)
		ServerInstance->Logs->Log("m_roleplay", DEBUG, "Inserting roll from %s, target \"%s\", into incoming roll queue: %s", roll->source.c_str(), roll->target.c_str(), roll->expression[0].c_str());
	else
		ServerInstance->Logs->Log("m_roleplay", DEBUG, "NOT Inserting roll from %s, target \"%s\", into incoming roll queue, QUEUE FULL.", roll->source.c_str(), roll->target.c_str());

	return added;
}



/* Get the roll at the front of the thread's outgoing queue, and remove it
 * from the queue. */
/* Returns NULL if the queue is empty. */
/* Run by main thread. */
UserRollResults* RollThread::GetRollResults()
{
	UserRollResults *roll = NULL;

	OutgoingQueueMutex.Lock();
	if (!OutgoingQueue.empty())
	{
		roll = OutgoingQueue.front();
		OutgoingQueue.pop();
		ServerInstance->Logs->Log("m_roleplay", DEBUG, "Retrieving finished roll from %s, to %s, out of the outgoing roll queue.", roll->source.c_str(), roll->target.c_str());
	}
	OutgoingQueueMutex.Unlock();

	return roll;
}



/* Handles a new completed roll. */
/* Run by main thread. */
void RollThread::OnNotify()
{
	UserRollResults* results;
	while ((results = this->GetRollResults()))
	{
		User *user;
		User *targetuser = NULL;
		Channel *targetchan = NULL;

		user = ServerInstance->FindUUID(results->source);
		if (!user)
		{
			delete results;
			return;
		}

		/* If a target other than themselves is specified, look for
		 * it. */
		if (results->target != "-" && results->target != user->uuid)
		{
			targetuser = ServerInstance->FindUUID(results->target);
			if (targetuser == NULL)
			{
				targetchan = ServerInstance->FindChan(results->target);
				if (targetchan == NULL)
				{
					delete results;
					return;
				}
			}
		}
		
		ModuleInstance->SendResults(user, targetuser, targetchan, *results);
		delete results;
	}
}



/* Roll thread main function. */
void RollThread::Run()
{
	while (1)
	{
		UserRoll* roll = NULL;
		UserRollResults* results = NULL;

		/* Wait until we have a roll to do or are asked to quit. If we
		 * are asked to quit, continue immediately with no roll. */
		this->LockQueue();
		while (!roll && !this->GetExitFlag())
		{
			if (!IncomingQueue.empty())
			{
				roll = IncomingQueue.front();
				IncomingQueue.pop();
			}
			else
			{
				/* If there's no work, freeze the thread and
				 * wait for some. */
				this->WaitForQueue();
			}
		}
		this->UnlockQueue();

		/* If the above continued without a roll, it means we have been
		 * asked to quit. Do so. */
		if (!roll)
			break;

		/* We got a roll. Create a UserRollResults instance to store the
		 * results. */
		results = new UserRollResults;
		results->source = roll->source;
		results->target = roll->target;

		/* Roll it! */
		RE.Run(*roll, *results);

		/* Now done with this roll, delete it. */
		delete roll;

		/* Add the results to the output queue! */
		OutgoingQueueMutex.Lock();
		OutgoingQueue.push(results);
		OutgoingQueueMutex.Unlock();
		
		/* Finally, notify the main thread that there's a finished
		 * roll's results ready for displaying. */
		this->NotifyParent();
	}

	/* Delete all entries in the incoming queue. */
	this->LockQueue();
	while (!IncomingQueue.empty())
	{
		delete IncomingQueue.front();
		IncomingQueue.pop();
	}
	this->UnlockQueue();

	/* Delete all entries in the outgoing queue. */
	OutgoingQueueMutex.Lock();
	while (!OutgoingQueue.empty())
	{
		delete IncomingQueue.front();
		IncomingQueue.pop();
	}
	OutgoingQueueMutex.Unlock();
}



MODULE_INIT(ModuleRoll)
