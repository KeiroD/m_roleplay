#ifndef _ROLLCALLBACK_H
#define _ROLLCALLBACK_H

#include "rollengine.h"

/* This header provides for modules to hook into the roll module. */

// Callback from m_roll to the modules that have asked to receive notification.
class RollChanCallback
{
public:
	/** Handles roll results being sent to a channel.
	 * This is called remotely on all servers, so long as the results
	 * contain at least some lines which are not ERROR, KICK, or SHUN. These
	 * lines will only ever be seen locally. All others will be seen
	 * globally, and in the correct order.
	 * @param u User that is doing the rolling.
	 * @param target Channel receiving the roll.
	 * @param results The results structure of the roll.
	 */
	virtual void OnChanRollResults(User* u, Channel* target, const RollResults& results) = 0;
};

class RegisterRollChanCallbackRequest : public Request
{
private:
	RollChanCallback* rpc;
public:
	RegisterRollChanCallbackRequest(Module* origin, Module* target, RollChanCallback* what) : Request(origin, target, "ROLLCHANCALLBACK"), rpc(what) { }

	RollChanCallback* GetCallback() { return rpc; }
};

#endif
