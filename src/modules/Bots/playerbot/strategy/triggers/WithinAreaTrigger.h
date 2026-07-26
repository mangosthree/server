#pragma once
#include "../Trigger.h"
#include "../values/LastMovementValue.h"

namespace ai
{
    class WithinAreaTrigger : public Trigger {
    public:
        WithinAreaTrigger(PlayerbotAI* ai) : Trigger(ai, "within area trigger") {}

        virtual bool IsActive()
        {


            LastMovement& movement = context->GetValue<LastMovement&>("last area trigger")->Get();
            if (!movement.lastAreaTrigger)
            {
                return false;
            }

            AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(movement.lastAreaTrigger);
            if (!atEntry)
            {
                return false;
            }

            AreaTrigger const* at = sObjectMgr.GetAreaTrigger(movement.lastAreaTrigger);
            if (!at)
            {
                return false;
            }

            return IsPointInAreaTriggerZone(atEntry, bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), 0.5f);
        }

    private:
        bool IsPointInAreaTriggerZone(AreaTriggerEntry const* atEntry, uint32 mapid, float x, float y, float z, float delta)
        {
            if (mapid != atEntry->MapId)
            {
                return false;
            }

            if (atEntry->Radius > 0)
            {
                // if we have radius check it
                float dist2 = (x - atEntry->PosX) * (x - atEntry->PosX) + (y - atEntry->PosY) * (y - atEntry->PosY) + (z - atEntry->PosZ) * (z - atEntry->PosZ);
                if (dist2 > (atEntry->Radius + delta) * (atEntry->Radius + delta))
                {
                    return false;
                }
            }
            else
            {
                // we have only extent

                // rotate the players position instead of rotating the whole cube, that way we can make a simplified
                // is-in-cube check and we have to calculate only one point instead of 4

                // 2PI = 360, keep in mind that ingame orientation is counter-clockwise
                double rotation = 2 * M_PI - atEntry->Box_yaw;
                double sinVal = sin(rotation);
                double cosVal = cos(rotation);

                float playerBoxDistX = x - atEntry->PosX;
                float playerBoxDistY = y - atEntry->PosY;

                float rotPlayerX = float(atEntry->PosX + playerBoxDistX * cosVal - playerBoxDistY * sinVal);
                float rotPlayerY = float(atEntry->PosY + playerBoxDistY * cosVal + playerBoxDistX * sinVal);

                // box edges are parallel to coordiante axis, so we can treat every dimension independently :D
                float dz = z - atEntry->PosZ;
                float dx = rotPlayerX - atEntry->PosX;
                float dy = rotPlayerY - atEntry->PosY;
                if ((fabs(dx) > atEntry->Box_length / 2 + delta) ||
                        (fabs(dy) > atEntry->Box_width / 2 + delta) ||
                        (fabs(dz) > atEntry->Box_height / 2 + delta))
                {
                    return false;
                }
            }

            return true;
        }
    };
}
