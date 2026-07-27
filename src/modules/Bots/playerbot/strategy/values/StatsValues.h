#pragma once
#include "../Value.h"

class Unit;

namespace ai
{
    class HealthValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        HealthValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class IsDeadValue : public BoolCalculatedValue, public Qualified
    {
    public:
        IsDeadValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual bool Calculate();
    };

    class PetIsDeadValue : public BoolCalculatedValue
    {
    public:
        PetIsDeadValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}
        virtual bool Calculate();
    };

    class PetIsHappyValue : public BoolCalculatedValue
    {
    public:
        PetIsHappyValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}
        virtual bool Calculate();
    };

    class RageValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        RageValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class EnergyValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        EnergyValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class FocusValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        FocusValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class ManaValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        ManaValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class HasManaValue : public BoolCalculatedValue, public Qualified
    {
    public:
        HasManaValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual bool Calculate();
    };

    /// Cata 4.3.4: soul shards are a power type, not items
    class SoulShardsValue : public Uint8CalculatedValue
    {
    public:
        SoulShardsValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}
        virtual uint8 Calculate();
    };

    /// Cata 4.3.4: paladin resource, spent by Templar's Verdict / Shield of the
    /// Righteous / Word of Glory / Inquisition
    class HolyPowerValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        HolyPowerValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    /// Cata 4.3.4: death knight resource; core stores it x10 internally, so
    /// Calculate() divides by 10 to give the 0-100 UI scale the rest of the
    /// rotation guidance (e.g. "40 RP Death Coil") is written against
    class RunicPowerValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        RunicPowerValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    /// Cata 4.3.4: counts the bot's own death knight rune slots that are both
    /// off cooldown and currently typed as the qualifier's rune type
    /// ("blood"|"frost"|"unholy"|"death"). Only meaningful for death knights -
    /// RuneMgr is otherwise uninitialized.
    class AvailableRunesValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        AvailableRunesValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}
        virtual uint8 Calculate();
    };

    class ComboPointsValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        ComboPointsValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };

    class IsMountedValue : public BoolCalculatedValue, public Qualified
    {
    public:
        IsMountedValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual bool Calculate();
    };

    class IsInCombatValue : public BoolCalculatedValue, public Qualified
    {
    public:
        IsInCombatValue(PlayerbotAI* ai) : BoolCalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual bool Calculate() ;
    };

    class BagSpaceValue : public Uint8CalculatedValue
    {
    public:
        BagSpaceValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        virtual uint8 Calculate();
    };

    class SpeedValue : public Uint8CalculatedValue, public Qualified
    {
    public:
        SpeedValue(PlayerbotAI* ai) : Uint8CalculatedValue(ai) {}

        Unit* GetTarget()
        {
            AiObjectContext* ctx = AiObject::context;
            return ctx->GetValue<Unit*>(qualifier)->Get();
        }
        virtual uint8 Calculate();
    };
}
