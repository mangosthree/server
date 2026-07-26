#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{

    class KickInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        KickInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "kick") {}
    };

    class SliceAndDiceTrigger : public BuffTrigger
    {
    public:
        SliceAndDiceTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "slice and dice") {}
    };

    class RuptureTrigger : public DebuffTrigger
    {
    public:
        RuptureTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "rupture") {}

        // Assassination/Subtlety only. Combat (probed via Revealing Strike,
        // 84617) treats Rupture as a DPS loss, so opt it out.
        virtual bool IsActive()
        {
            return DebuffTrigger::IsActive() && AI_VALUE2(uint8, "combo", "current target") >= 5
                && !bot->HasSpell(84617);
        }
    };

    class ExposeArmorTrigger : public DebuffTrigger
    {
    public:
        ExposeArmorTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "expose armor") {}
    };

    class RevealingStrikeTrigger : public DebuffTrigger
    {
    public:
        RevealingStrikeTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "revealing strike") {}

        // Apply Revealing Strike at exactly 4 combo points; at 5 the dump wins.
        virtual bool IsActive()
        {
            return DebuffTrigger::IsActive() && AI_VALUE2(uint8, "combo", "current target") == 4;
        }
    };

    class BladeFlurryTrigger : public BoostTrigger
    {
    public:
        BladeFlurryTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "blade flurry") {}
    };

    class AdrenalineRushTrigger : public BoostTrigger
    {
    public:
        AdrenalineRushTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "adrenaline rush") {}
    };

    class KillingSpreeTrigger : public BoostTrigger
    {
    public:
        KillingSpreeTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "killing spree") {}

        // Best used as an Energy dump when very low on Energy.
        virtual bool IsActive()
        {
            return BoostTrigger::IsActive() && AI_VALUE2(uint8, "energy", "self target") < 30;
        }
    };

    class VendettaTrigger : public BoostTrigger
    {
    public:
        VendettaTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "vendetta") {}
    };

    class ColdBloodTrigger : public BoostTrigger
    {
    public:
        ColdBloodTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "cold blood") {}
    };

    class ShadowDanceTrigger : public BoostTrigger
    {
    public:
        ShadowDanceTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "shadow dance") {}
    };

    class ShadowDanceActiveTrigger : public HasAuraIdTrigger
    {
    public:
        ShadowDanceActiveTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "shadow dance active", 51713) {}
    };

    class HemorrhageTrigger : public DebuffTrigger
    {
    public:
        HemorrhageTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "hemorrhage") {}
    };

    class KickInterruptEnemyHealerSpellTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        KickInterruptEnemyHealerSpellTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "kick") {}
    };

    class StealthTrigger : public Trigger
    {
    public:
        StealthTrigger(PlayerbotAI* ai) : Trigger(ai, "stealth") {}

        virtual bool IsActive()
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            if (target && !target->IsAlive())
                target = NULL;
            if (!target && !bot->IsInCombat() && ai->HasAura("stealth", bot))
                bot->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
            return target && !ai->HasAura("stealth", bot);
        }
    };

    class ComboPointsForTargetAvailableTrigger : public Trigger
    {
    public:
        ComboPointsForTargetAvailableTrigger(PlayerbotAI* ai)
            : Trigger(ai, "combo points for target available"), m_threshold(2) {}

        virtual bool IsActive()
        {
            Unit* target = AI_VALUE(Unit*, "current target");
            if (!target)
                return false;

            ObjectGuid guid = target->GetObjectGuid();
            if (guid != m_lastTargetGuid)
            {
                m_lastTargetGuid = guid;
                m_threshold = 2;
            }

            uint8 combo = AI_VALUE2(uint8, "combo", "current target");
            if (combo >= m_threshold)
            {
                m_threshold = nextThreshold(target);
                return true;
            }
            return false;
        }

    private:
        uint8 m_threshold;
        ObjectGuid m_lastTargetGuid;

        uint8 nextThreshold(Unit* target)
        {
            Creature* creature = dynamic_cast<Creature*>(target);
            if (creature && creature->GetCreatureInfo() &&
                creature->GetCreatureInfo()->Rank > CREATURE_ELITE_NORMAL)
                return 3 + urand(0, 2);  // 3, 4, or 5
            else
                return 1 + urand(0, 1);  // 1 or 2
        }
    };
}
