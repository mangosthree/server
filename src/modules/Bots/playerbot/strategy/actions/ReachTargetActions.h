#pragma once

#include "../Action.h"
#include "MovementActions.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class ReachTargetAction : public MovementAction
    {
    public:
        ReachTargetAction(PlayerbotAI* ai, string name, float distance) : MovementAction(ai, name)
        {
            this->distance = distance;
        }
        virtual bool Execute(Event event)
        {
            return MoveTo(AI_VALUE(Unit*, "current target"), distance);
        }
        virtual bool isUseful()
        {
            return AI_VALUE2(float, "distance", "current target") > (distance + sPlayerbotAIConfig.contactDistance);
        }
        virtual string GetTargetName() { return "current target"; }

    protected:
        float distance;
    };

    class CastReachTargetSpellAction : public CastSpellAction
    {
    public:
        CastReachTargetSpellAction(PlayerbotAI* ai, string spell, float distance) : CastSpellAction(ai, spell)
        {
            this->distance = distance;
        }
        virtual bool isUseful()
        {
            return AI_VALUE2(float, "distance", "current target") > (distance + sPlayerbotAIConfig.contactDistance);
        }

    protected:
        float distance;
    };

    class ReachMeleeAction : public ReachTargetAction
    {
    public:
        ReachMeleeAction(PlayerbotAI* ai) : ReachTargetAction(ai, "reach melee", sPlayerbotAIConfig.meleeDistance) {}

        // Account for the bot's body radius: "distance" is measured center-to-center,
        // so without the bounding radius the bot keeps trying to close in while it is
        // already in melee contact -- the dead-zone jitter of standing next to a mob
        // without attacking.
        virtual bool isUseful()
        {
            return AI_VALUE2(float, "distance", "current target") >
                distance + sPlayerbotAIConfig.contactDistance + bot->GetObjectBoundingRadius();
        }
    };

    // Move away from the target until at least meleeDistance + contact away, so a
    // caster/ranged bot with a mob in its face steps back into casting range.
    class BackOffAction : public ReachTargetAction
    {
    public:
        BackOffAction(PlayerbotAI* ai) : ReachTargetAction(ai, "back off", sPlayerbotAIConfig.meleeDistance) {}

        virtual bool isUseful()
        {
            return AI_VALUE2(float, "distance", "current target") <
                distance + sPlayerbotAIConfig.contactDistance + bot->GetObjectBoundingRadius();
        }
    };

    class ReachSpellAction : public ReachTargetAction
    {
    public:
        ReachSpellAction(PlayerbotAI* ai) : ReachTargetAction(ai, "reach spell", sPlayerbotAIConfig.spellDistance) {}

        // The target distance is set per-spell via the "reach spell distance" value
        // (by CastSpellAction::getPrerequisites) so the bot reaches that spell's range.
        virtual bool Execute(Event event)
        {
            distance = AI_VALUE(float, "reach spell distance");
            return ReachTargetAction::Execute(event);
        }
        virtual bool isUseful()
        {
            distance = AI_VALUE(float, "reach spell distance");
            return ReachTargetAction::isUseful();
        }
    };
}
