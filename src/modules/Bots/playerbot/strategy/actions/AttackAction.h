#pragma once

#include "../Action.h"
#include "MovementActions.h"

namespace ai
{
    class AttackAction : public MovementAction
    {
    public:
        AttackAction(PlayerbotAI* ai, string name) :
            MovementAction(ai, name), m_tankEngageTarget(),
            m_tankEngageTime(0) {}

    public:
        virtual bool Execute(Event event);
        virtual bool isUseful();

    protected:
        bool Attack(Unit* target);
        ObjectGuid m_tankEngageTarget;
        uint32 m_tankEngageTime;
    };

    class AttackMyTargetAction : public AttackAction
    {
    public:
        AttackMyTargetAction(PlayerbotAI* ai, string name = "attack my target") : AttackAction(ai, name) {}

    public:
        virtual bool Execute(Event event);
    };

    class AttackDuelOpponentAction : public AttackAction
    {
    public:
        AttackDuelOpponentAction(PlayerbotAI* ai, string name = "attack duel opponent") : AttackAction(ai, name) {}

    public:
        virtual bool Execute(Event event);
        virtual bool isUseful();
    };
}
