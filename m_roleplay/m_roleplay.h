#ifndef _ROLEPLAY_H
#define _ROLEPLAY_H

#include <string>

/* This header provides for modules to hook into the roleplay module. */

// Callback from m_roleplay to the modules that have asked to receive notification.
class RoleplayCallback
{
public:
	/** Handles a Roleplay event.
	 * @param u User that is doing the roleplaying.
	 * @param c Channel this is happening on.
	 * @param rpnick The name the user is using, fully decorated as necessary.
	 * @param text The string sent.
	 */
	virtual void OnRoleplay(User* u, Channel* c, const std::string& rpnick, const std::string& text) = 0;

	virtual ~RoleplayCallback() { }
};

class RegisterRPCallbackRequest : public Request
{
private:
	RoleplayCallback* rpc;
public:
	RegisterRPCallbackRequest(Module* origin, Module* target, RoleplayCallback* what) : Request(origin, target, "ROLEPLAYCALLBACK"), rpc(what) { }

	RoleplayCallback* GetCallback() { return rpc; }
};

#endif
