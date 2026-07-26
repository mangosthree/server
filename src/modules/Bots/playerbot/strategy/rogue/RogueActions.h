#pragma once

#include "../actions/GenericActions.h"
#include "RogueComboActions.h"
#include "RogueOpeningActions.h"
#include "RogueFinishingActions.h"

namespace ai
{
    class CastEvasionAction : public CastBuffSpellAction
    {
    public:
        CastEvasionAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "evasion") {}
    };

    class CastSprintAction : public CastBuffSpellAction
    {
    public:
        CastSprintAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "sprint") {}
    };

    class CastKickAction : public CastSpellAction
    {
    public:
        CastKickAction(PlayerbotAI* ai) : CastSpellAction(ai, "kick") {}
    };

    class CastFeintAction : public CastBuffSpellAction
    {
    public:
        CastFeintAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "feint") {}
    };

    class CastDismantleAction : public CastSpellAction
    {
    public:
        CastDismantleAction(PlayerbotAI* ai) : CastSpellAction(ai, "dismantle") {}
    };

    class CastDistractAction : public CastSpellAction
    {
    public:
        CastDistractAction(PlayerbotAI* ai) : CastSpellAction(ai, "distract") {}
    };

    class CastVanishAction : public CastBuffSpellAction
    {
    public:
        CastVanishAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "vanish") {}
    };

    class CastBlindAction : public CastDebuffSpellAction
    {
    public:
        CastBlindAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "blind") {}
    };


    class CastBladeFlurryAction : public CastBuffSpellAction
    {
    public:
        CastBladeFlurryAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blade flurry") {}
    };

    class CastAdrenalineRushAction : public CastBuffSpellAction
    {
    public:
        CastAdrenalineRushAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "adrenaline rush") {}
    };

    class CastKillingSpreeAction : public CastBuffSpellAction
    {
    public:
        CastKillingSpreeAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "killing spree") {}
    };

    class CastRevealingStrikeAction : public CastDebuffSpellAction
    {
    public:
        CastRevealingStrikeAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "revealing strike") {}
    };

    class CastKickOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastKickOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "kick") {}
    };

    class CastVendettaAction : public CastDebuffSpellAction
    {
    public:
        CastVendettaAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "vendetta") {}
    };

    class CastColdBloodAction : public CastBuffSpellAction
    {
    public:
        CastColdBloodAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "cold blood") {}
    };

    class CastShadowDanceAction : public CastBuffSpellAction
    {
    public:
        CastShadowDanceAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "shadow dance") {}
    };

    // AoE finisher. Requires a thrown weapon equipped in the ranged slot;
    // without one the cast fails, so gate on the equipped subclass.
    class CastFanOfKnivesAction : public CastMeleeSpellAction
    {
    public:
        CastFanOfKnivesAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "fan of knives") {}

        virtual bool isUseful()
        {
            Item* ranged = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
            return ranged && ranged->GetProto() &&
                ranged->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_THROWN;
        }
    };

    class CastTricksOfTheTradeAction : public BuffOnPartyAction
    {
    public:
        CastTricksOfTheTradeAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "tricks of the trade") {}
    };

    class CastStealthAction : public CastBuffSpellAction
    {
    public:
        CastStealthAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "stealth") {}
    };

    class BeginSapAction : public Action
    {
    public:
        BeginSapAction(PlayerbotAI* ai) : Action(ai, "begin sap") {}

        virtual bool Execute(Event event)
        {
            Player* master = ai->GetMaster();
            if (!master)
                return false;

            ObjectGuid targetGuid = master->GetSelectionGuid();
            if (targetGuid.IsEmpty())
                return false;

            Unit* target = ai->GetUnit(targetGuid);
            if (!target || !target->IsAlive())
                return false;

            bot->SetSelectionGuid(targetGuid);
            context->GetValue<Unit*>("current target")->Set(target);
            ai->ChangeStrategy("+sap", BOT_STATE_NON_COMBAT);
            return true;
        }
    };

    class EndSapAction : public Action
    {
    public:
        EndSapAction(PlayerbotAI* ai) : Action(ai, "end sap") {}

        virtual bool isUseful()
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            if (!target || !target->IsAlive())
                return true;
            if (ai->HasAura("sap", target))
                return true;
            if (bot->IsInCombat())
                return true;
            return false;
        }

        virtual bool Execute(Event event)
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            bool sapSucceeded = target && ai->HasAura("sap", target);
            if (sapSucceeded)
            {
                target->DeleteThreatList();
                target->CombatStop();
                bot->CombatStop();
            }
            bot->AttackStop();
            context->GetValue<Unit*>("current target")->Set(NULL);
            bot->SetSelectionGuid(ObjectGuid());
            ai->ChangeStrategy("-sap", BOT_STATE_NON_COMBAT);
            return true;
        }
    };

    class BeginAmbushAction : public Action
    {
    public:
        BeginAmbushAction(PlayerbotAI* ai) : Action(ai, "begin ambush") {}

        virtual bool Execute(Event event)
        {
            Player* master = ai->GetMaster();
            if (!master)
                return false;

            ObjectGuid guid = master->GetSelectionGuid();
            if (guid.IsEmpty())
                return false;

            Unit* target = ai->GetUnit(guid);
            if (!target || !target->IsAlive())
                return false;

            if (bot->IsFriendlyTo(target))
                return false;

            if (!bot->IsWithinLOSInMap(target))
                return false;

            bot->SetSelectionGuid(guid);
            context->GetValue<Unit*>("current target")->Set(target);
            ai->ChangeStrategy("+ambush", BOT_STATE_NON_COMBAT);
            return true;
        }
    };

    class RogueEndAmbushAction : public Action
    {
    public:
        RogueEndAmbushAction(PlayerbotAI* ai) : Action(ai, "end ambush") {}

        virtual bool isUseful()
        {
            return bot->IsInCombat();
        }

        virtual bool Execute(Event event)
        {
            ai->ChangeStrategy("-ambush", BOT_STATE_NON_COMBAT);
            ai->ChangeEngine(BOT_STATE_COMBAT);
            return true;
        }
    };
}
