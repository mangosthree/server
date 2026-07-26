#include "botpch.h"
#include "../../playerbot.h"
#include "RememberTaxiAction.h"
#include "../values/LastMovementValue.h"

using namespace ai;

bool RememberTaxiAction::Execute(Event event)
{
    WorldPacket p(event.getPacket());
    p.rpos(0);

    switch (p.GetOpcode())
    {
    case CMSG_ACTIVATETAXI:
        {
            LastMovement& movement = context->GetValue<LastMovement&>("last taxi")->Get();
            movement.taxiNodes.clear();
            movement.taxiNodes.resize(2);
            try
            {
                p >> movement.taxiMaster >> movement.taxiNodes[0] >> movement.taxiNodes[1];
            }
            catch(ByteBufferException&)
            {
                // Packet read failure that would cause server crash.
                return false;
            }
            return true;
        }
    case CMSG_ACTIVATETAXIEXPRESS:
        {
            ObjectGuid guid;
            uint32 node_count;
            try
            {
                p >> guid >> node_count;

                // Store into "last taxi" (not "last movement"): TaxiAction reads
                // "last taxi", and "last movement" is overwritten by every normal
                // move, which would wipe a remembered multi-hop taxi path before
                // the bot follows it. The single-hop CMSG_ACTIVATETAXI case above
                // already uses "last taxi"; keep the express path consistent.
                LastMovement& movement = context->GetValue<LastMovement&>("last taxi")->Get();
                movement.taxiNodes.clear();
                for (uint32 i = 0; i < node_count; ++i)
                {
                    uint32 node;
                    p >> node;
                    movement.taxiNodes.push_back(node);
                }

                return true;
            }
            catch(ByteBufferException&)
            {
                // Packet read failure that would cause server crash.
                return false;
            }
        }
    }

    return false;
}
