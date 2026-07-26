#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotCommandServer.h"

INSTANTIATE_SINGLETON_1(PlayerbotCommandServer);

using namespace std;

void PlayerbotCommandServer::Start()
{
    // The remote command server was built on ACE sockets, which the core
    // removed (ACE removal #303/#304). It is optional and disabled by default
    // (AiPlayerbot.CommandServerPort = 0). Reimplement on the WorldNetwork
    // layer if remote bot control is ever needed.
    if (sPlayerbotAIConfig.commandServerPort)
    {
        sLog.outError("Playerbot command server (port %d) is unavailable: not "
            "ported off ACE yet - ignoring.", sPlayerbotAIConfig.commandServerPort);
    }
}
