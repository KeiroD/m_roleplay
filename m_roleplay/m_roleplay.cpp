/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2009 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "channels.h"
#include "modules.h"

#include "m_cap.h"
#include "m_roleplay.h"

/* $ModDesc: Provides NPC, NPCA, FSAY, FACTION, and SCENE commands for use
 * by Game Masters doing pen & paper RPGs via IRC. Forked off CyberBotX's 
 * m_rpg module, and based on Falerin's m_roleplay module for UnrealIRCd. */
/* $ModAuthor: John Robert Beshir (Namegduf) */
/* $ModAuthorMail: namegduf@tellaerad.net */
/* Tributes also go to aquanight for the callback system, in addition to those
 * to the two named above. */

/* Declaration of module class. */
class ModuleRP;



/* Definition of RP Room (Extended RP command access) mode handler. */
class RPRoom : public SimpleChannelModeHandler
{
 public:
	RPRoom(Module *module) : SimpleChannelModeHandler(module, "roleplay", 'E') { }
};



/* Declaration of roleplay command handlers. */
class cmd_npc : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_npc(ModuleRP *module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters);
};

class cmd_npca : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_npca(ModuleRP *module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters);
};

class cmd_scene : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_scene(ModuleRP *module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters);
};

class cmd_fsay : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_fsay(ModuleRP *module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters);
};

class cmd_faction : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_faction(ModuleRP *module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters);
};

class cmd_rpmsg : public Command
{
private:
	ModuleRP *ModuleInstance;
public:
	cmd_rpmsg(ModuleRP* module);
	CmdResult Handle(const std::vector<std::string>& parameters, User *user);
	
	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters)
	{
		return ROUTE_OPT_BCAST;
	}
};


class ModuleRP : public Module
{
private:
	RPRoom* cmode_E;
	cmd_npc *npc;
	cmd_npca *npca;
	cmd_scene *scene;
	cmd_fsay *fsay;
	cmd_faction *faction;
	cmd_rpmsg *rpmsg;

	std::vector<std::pair<Module*, RoleplayCallback*> > cbs;

public:
	std::string rphost;
	std::string scenenick;

	ModuleRP() : Module::Module()
	{
		npc = new cmd_npc(this);
		ServerInstance->AddCommand(npc);
		npca = new cmd_npca(this);
		ServerInstance->AddCommand(npca);
		scene = new cmd_scene(this);
		ServerInstance->AddCommand(scene);
		fsay = new cmd_fsay(this);
		ServerInstance->AddCommand(fsay);
		faction = new cmd_faction(this);
		ServerInstance->AddCommand(faction);
		rpmsg = new cmd_rpmsg(this);
		ServerInstance->AddCommand(rpmsg);
		

		cmode_E = new RPRoom(this);
		if (!ServerInstance->Modes->AddMode(cmode_E))
		{
			throw ModuleException("Could not add new modes!");
		}

		Implementation eventlist[] = { I_OnRehash, I_OnUnloadModule };
		ServerInstance->Modules->Attach(eventlist, this, 2);

		OnRehash(NULL);
	}

	virtual ~ModuleRP() 
	{
		delete(npc);
		delete(npca);
		delete(scene);
		delete(fsay);
		delete(faction);
		delete(rpmsg);

		ServerInstance->Modes->DelMode(cmode_E);
		delete cmode_E;
	}

	virtual void OnRehash(User* user)
	{
		ConfigReader Conf;
		rphost = Conf.ReadValue("roleplay", "fakehost", "npc.fakeuser.invalid", 0, false);
		scenenick = Conf.ReadValue("roleplay", "scenenick", "=Scene=", 0, false);
	}

	virtual void OnUnloadModule(Module* mod)
	{
		ServerInstance->Logs->Log("ROLEPLAY", DEBUG, "[m_roleplay] Removing all callbacks for module %s (module unloading)", mod->ModuleSourceFile.c_str());
		std::vector<std::pair<Module*, RoleplayCallback*> >::iterator i = cbs.begin();
		while (i != cbs.end())
		{
			if (i->first == mod)
				i = cbs.erase(i);
			else
				++i;
		}
	}

	virtual void OnRequest(Request& request)
	{
		ServerInstance->Logs->Log("ROLEPLAY", DEBUG, "[m_roleplay] Request received (ID = %s)", request.id);
		if (!strcmp(request.id, "ROLEPLAYCALLBACK"))
		{
			RegisterRPCallbackRequest* rpc = (RegisterRPCallbackRequest*)(&request);
			ServerInstance->Logs->Log("ROLEPLAY", DEBUG, "[m_roleplay] Registered callback for %s", (*(rpc->source)).ModuleSourceFile.c_str());
			cbs.push_back(std::make_pair(rpc->source, rpc->GetCallback()));
		}
	}

	virtual Version GetVersion()
	{
		return Version("Provides roleplaying commands to IRC channels.", VF_COMMON);
	}

	/* Generic roleplay command handler; called by all local roleplay
	 * commands, with parameters setting behaviour for each. Handles
	 * validation and filtering of parameters, and then local handling and
	 * display. */
	/* This assumes the parameter vector has at least two entries, three if
	 * a nick is not specified. Ensure this before calling. */
	CmdResult ProcessRoleplayCommand(const std::vector<std::string>& parameters, User  *user, std::string nick, bool mark, bool action)
	{
		/* Locate the channel. */
		Channel *channel = ServerInstance->FindChan(parameters[0]);
		if (!channel)
		{
			user->WriteServ("401 %s %s :No such channel", 
			user->nick.c_str(), parameters[0].c_str());
			return CMD_FAILURE;
		}
		
		/* Check the user has the power to roleplay in this channel. */
		/* Opers with the "channels/roleplay" privilege always can. */
		if (!user->HasPrivPermission("channels/roleplay"))
		{
			/* If the channel is +E, the user must be voiced or
			 * higher. Otherwise, they must be a halfop or above. */
			if (channel->IsModeSet('E'))
			{
				if (channel->GetPrefixValue(user) < VOICE_VALUE)
				{
					user->WriteServ("482 %s %s :You're not voiced or a channel operator.", user->nick.c_str(), parameters[0].c_str());
					return CMD_FAILURE;
				}
			}
			else if (channel->GetPrefixValue(user) < HALFOP_VALUE)
			{
				user->WriteServ("482 %s %s :You're not a channel operator.", user->nick.c_str(), parameters[0].c_str());
				return CMD_FAILURE;
			}
		}
	
		/* Build message, skipping a parameter if nick is not set, and
		 * adding the ACTION wrapper if it is an action. Strip all
		 * other characters of value '1' from it. */
		std::string line = "";
		if(action) line = "\1ACTION ";
		for (unsigned int i = (nick == "" ? 2 : 1); i < parameters.size(); ++i)
		{
			for (size_t x = 0; parameters[i][x]; ++x)
			{
				if (parameters[i][x] != 1)
					line += parameters[i][x];
			}

			if (i != parameters.size() -1)
				line += " ";
		}
		if(action) line += "\1";
	
		/* Build the NPC nick, if not provided to us. */
		if(nick == "")
		{
			nick = parameters[1];
				
			/* Strip unwanted characters from the nick, including
			 * the underline control code and clear formatting code
			 * if it is to be marked. */
			std::string newnick = "";
			for (int x = 0; nick[x]; ++x) 
			{
				if (nick[x] < 32 && !(nick[x] == 2 || nick[x] == 3 || nick[x] == 22 || (!mark && nick[x] == 15) || (!mark && nick[x] == 31)))
					continue;
				
				if (nick[x] == '!') continue;
				newnick += nick[x];
			}
			nick = newnick;
	
			/* Create a temporary copy of the final nick and strip
			 * every control character out of the results. Quit with
			 * an error if there isn't at least one non-control
			 * character, and the nick thus has no visible
			 * contents. */
			newnick = "";
			int incolor = 0;
			for (int x = 0; nick[x]; ++x)
			{
				/* Account for the hidden digits in color
				 * codes. */
				if (nick[x] == 3) { incolor = 2; continue; }
				if (incolor) {
 					if(nick[x] > 47 && nick[x] < 58)
					{
						--incolor; 
						continue;
					}
					else incolor = 0;
				}
	
				/* Strip all control codes. */
				if (nick[x] < 32) continue;

				/* Next character! */
				newnick += nick[x];
			}
			if(newnick == "")
			{
				user->WriteServ("401 %s %s :No visible non-stripped characters in nick.", user->nick.c_str(), parameters[0].c_str());
				return CMD_FAILURE;
			}

			/* If the nick is to be marked, underline it, after
			 * other formatting codes that could affect this have
			 * been removed. */
			if(mark) nick = "\x1F" + nick + "\x1F";
		}

		/* Display roleplay message locally. */
		PrintRoleplay(user, channel, nick, line);
	
		/* Propagate roleplay message via ENCAP. */
		parameterlist sendparams;
		sendparams.push_back("*");
		sendparams.push_back("RPMSG");
		sendparams.push_back(user->uuid);
		sendparams.push_back(channel->name);
		sendparams.push_back(nick);
		sendparams.push_back(":" + line);
		ServerInstance->PI->SendEncapsulatedData(sendparams);

		/* Call OnRoleplay hooks in other modules. */
		RunRoleplayCallbacks(user, channel, nick, line);
	
		return CMD_SUCCESS;
	}
	
	/* Print the results of both local and remote roleplay commands. */
	void PrintRoleplay(User* user, Channel* channel, const std::string &nick, const std::string &line)
	{
		/* Create the source host... */
		std::string source = nick + "!" + user->nick + "@" + rphost;
		
		/* Send the message! */
		channel->WriteChannelWithServ(source.c_str(), "PRIVMSG %s :%s", channel->name.c_str(), line.c_str());
	}

	/* Calls OnRoleplay hooks in other modules. */
	void RunRoleplayCallbacks(User *source, Channel *channel, const std::string &nick, const std::string &line)
	{
		for (std::vector<std::pair<Module*, RoleplayCallback*> >::iterator i = cbs.begin(); i != cbs.end(); ++i)
		{
			i->second->OnRoleplay(source, channel, nick, line);
		}
	}
};



/* Handle /NPC. This will be a message marked as an NPC. */
cmd_npc::cmd_npc(ModuleRP *module) : Command(module, "NPC", 3, 3)
{
	this->ModuleInstance = module;
	syntax = "[channel] [nick] [text]";
}
CmdResult cmd_npc::Handle(const std::vector<std::string>& parameters, User *user)
{
	return ModuleInstance->ProcessRoleplayCommand(parameters, user, "", true, false);
}
RouteDescriptor cmd_npc::GetRouting(User* user, const std::vector<std::string>& parameters)
{
	return ROUTE_LOCALONLY;
}



/* Handle /NPCA. This will be an action marked as an NPC. */
cmd_npca::cmd_npca(ModuleRP *module) : Command(module, "NPCA", 3, 3)
{
	this->ModuleInstance = module;
	syntax = "[channel] [nick] [text]";
}
CmdResult cmd_npca::Handle(const std::vector<std::string>& parameters, User *user)
{
	return ModuleInstance->ProcessRoleplayCommand(parameters, user, "", true, true);
}
RouteDescriptor cmd_npca::GetRouting(User* user, const std::vector<std::string>& parameters)
{
	return ROUTE_LOCALONLY;
}



/* Handle /SCENE. This has a fixed nick, and will never be marked. */
cmd_scene::cmd_scene(ModuleRP *module) : Command(module, "SCENE", 2, 2)
{
	this->ModuleInstance = module;
	syntax = "[channel] [text]";
}
CmdResult cmd_scene::Handle(const std::vector<std::string>& parameters, User *user)
{
	return ModuleInstance->ProcessRoleplayCommand(parameters, user, ModuleInstance->scenenick, false, false);
}
RouteDescriptor cmd_scene::GetRouting(User* user, const std::vector<std::string>& parameters)
{
	return ROUTE_LOCALONLY;
}



/* Handle /FSAY. This will be unmarked for opers. */
/* For non-opers, the behaviour is the same as /NPC. */
cmd_fsay::cmd_fsay(ModuleRP *module) : Command(module, "FSAY", 3, 3)
{
	this->ModuleInstance = module;
	syntax = "[channel] [nick] [text]";
}
CmdResult cmd_fsay::Handle(const std::vector<std::string>& parameters, User *user)
{
	return ModuleInstance->ProcessRoleplayCommand(parameters, user, "", user->HasPrivPermission("channels/roleplay") ? false : true, false);
}
RouteDescriptor cmd_fsay::GetRouting(User* user, const std::vector<std::string>& parameters)
{
	return ROUTE_LOCALONLY;
}



/* Handle /FACTION. This will be unmarked for opers. */
/* For non-opers, the behaviour is the same as /NPCA. */
cmd_faction::cmd_faction(ModuleRP *module) : Command(module, "FACTION", 3, 3)
{
	this->ModuleInstance = module;
	syntax = "[channel] [nick] [text]";
}
CmdResult cmd_faction::Handle(const std::vector<std::string>& parameters, User *user)
{
	return ModuleInstance->ProcessRoleplayCommand(parameters, user, "", user->HasPrivPermission("channels/roleplay") ? false : true, true);
}  
RouteDescriptor cmd_faction::GetRouting(User* user, const std::vector<std::string>& parameters)
{
	return ROUTE_LOCALONLY;
}




/* Handle remote roleplay command. */
cmd_rpmsg::cmd_rpmsg(ModuleRP* module) : Command(module, "RPMSG", 4)
{
	this->ModuleInstance = module;
	this->flags_needed = FLAG_SERVERONLY; // should not be called by users
}

CmdResult cmd_rpmsg::Handle(const std::vector<std::string>& parameters, User *user)
{
	/* Malformed ENCAP! Madness, sabotage, treachery! */
	if (parameters.size() != 4)
	{
		ServerInstance->Logs->Log("m_roleplay", DEBUG, "RPMSG has the wrong number of parameters and is malformed; ignoring it.");	
		return CMD_FAILURE;
	}

	User* source = ServerInstance->FindNick(parameters[0]);
	if (!source)
	{
		ServerInstance->Logs->Log("m_roleplay", DEBUG, "RPMSG source user (%s) unknown; ignoring.", (parameters[2]).c_str());	
		return CMD_FAILURE;
	}

	Channel* channel = ServerInstance->FindChan(parameters[1]);
	if (!channel)
	{
			ServerInstance->Logs->Log("m_roleplay", DEBUG, "RPMSG target channel (%s) unknown; ignoring.", (parameters[3]).c_str());	
			return CMD_FAILURE;
	}

	/* Print the roleplay message. */
	ModuleInstance->PrintRoleplay(source, channel, parameters[2], parameters[3]);

	/* Call OnRoleplay hooks in other modules. */
	ModuleInstance->RunRoleplayCallbacks(user, channel, parameters[2], parameters[3]);

	return CMD_SUCCESS;
}

MODULE_INIT(ModuleRP)
