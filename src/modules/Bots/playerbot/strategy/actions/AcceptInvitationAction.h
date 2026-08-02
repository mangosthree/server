#pragma once

#include "../Action.h"

namespace ai
{
    class AcceptInvitationAction : public Action {
    public:
        AcceptInvitationAction(PlayerbotAI* ai) : Action(ai, "accept invitation") {}

        virtual bool Execute(Event event)
        {
            Player* master = GetMaster();

            Group* grp = bot->GetGroupInvite();
            if (!grp)
            {
                return false;
            }

            Player* inviter = sObjectMgr.GetPlayer(grp->GetLeaderGuid());
            if (!inviter)
            {
                return false;
            }

            if (!ai->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, inviter))
            {
                WorldPacket data(SMSG_GROUP_DECLINE, 10);
                data << bot->GetName();
                inviter->GetSession()->SendPacket(&data);
                bot->UninviteFromGroup();
                return false;
            }

            // Cata 4.3.4 CMSG_GROUP_INVITE_RESPONSE: reads bit 'unk' then bit 'accepted'.
            WorldPacket p(CMSG_GROUP_INVITE_RESPONSE, 1);
            p.WriteBit(false); // unk (0 => no trailing uint32)
            p.WriteBit(true);  // accepted
            p.FlushBits();
            bot->GetSession()->HandleGroupInviteResponseOpcode(p);

            if (sRandomPlayerbotMgr.IsRandomBot(bot))
            {
                bot->GetPlayerbotAI()->SetMaster(inviter);
            }

            ai->ResetStrategies();
            ai->TellMaster("Hello");
            return true;
        }
    };

}
