#ifndef _PartyTankValue_H
#define _PartyTankValue_H

#include "PartyMemberValue.h"

namespace ai
{
    /// The group member acting as tank (a bot running the tank strategy, or a
    /// tank-capable class / bear-form druid via PlayerbotAI::IsTank). Used to
    /// target friendly maintenance spells on the tank -- Beacon of Light,
    /// Earth Shield, Lifebloom, Misdirection -- unlike "tank target" which
    /// returns the ENEMY the tank is holding.
    class PartyTankValue : public PartyMemberValue
    {
    public:
        PartyTankValue(PlayerbotAI* ai) : PartyMemberValue(ai) {}

    protected:
        virtual Unit* Calculate()
        {
            struct FindTankPlayer : public FindPlayerPredicate
            {
                FindTankPlayer(PlayerbotAI* ai) : ai(ai) {}
                virtual bool Check(Unit* unit)
                {
                    Player* player = dynamic_cast<Player*>(unit);
                    return player && player->IsAlive() && ai->IsTank(player);
                }
                PlayerbotAI* ai;
            };

            FindTankPlayer finder(ai);
            return FindPartyMember(finder);
        }
    };
}

#endif
