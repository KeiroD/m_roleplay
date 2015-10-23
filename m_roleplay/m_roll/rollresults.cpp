/* RollResult member function source file. */
#include "rollengine.h"



void RollResults::AddError(const std::string& msg)
{
	types.push_back(ERR);
	data.push_back(msg);
}



void RollResults::AddMsg(const std::string& msg)
{
	types.push_back(MESSAGE);
	data.push_back(msg);
}



void RollResults::AddAction(const std::string& action)
{
	types.push_back(ACTION);
	data.push_back(action);
}



void RollResults::AddNPC(const std::string& npc, const std::string& msg)
{
	types.push_back(NPC);
	data.push_back(npc);
	data.push_back(msg);
}



void RollResults::AddNPCA(const std::string& npc, const std::string& action)
{
	types.push_back(NPCA);
	data.push_back(npc);
	data.push_back(action);
}



void RollResults::AddScene(const std::string& msg)
{
	types.push_back(SCENE);
	data.push_back(msg);
}



void RollResults::AddKick(const std::string& reason)
{
	types.push_back(KICK);
	data.push_back(reason);
}



void RollResults::AddShun(const std::string& reason, const int& duration)
{
	types.push_back(SHUN);
	data.push_back(reason);

	std::stringstream convstream;
	std::string durationstring;
	convstream << duration;
	convstream >> durationstring;
	data.push_back(durationstring);
}



void RollResults::Clear()
{
	types.clear();
	data.clear();
}
