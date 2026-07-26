#pragma once

namespace ai {
    class CastBearFormAction : public CastBuffSpellAction {
    public:
        CastBearFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "bear form") {}
    };

    /// Cata 4.3.4: Dire Bear Form was merged into Bear Form (key kept for wiring)
    class CastDireBearFormAction : public CastBuffSpellAction {
    public:
        CastDireBearFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "bear form") {}
    };

    class CastCatFormAction : public CastBuffSpellAction {
    public:
        CastCatFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "cat form") {}
    };

    class CastTreeFormAction : public CastBuffSpellAction {
    public:
        CastTreeFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "tree of life") {}
    };

    class CastMoonkinFormAction : public CastBuffSpellAction {
    public:
        CastMoonkinFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "moonkin form") {}
    };

    class CastCasterFormAction : public CastBuffSpellAction {
    public:
        CastCasterFormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "caster form") {}

        virtual bool isUseful() {
            return ai->HasAnyAuraOf(GetTarget(), "dire bear form", "bear form", "cat form", "travel form", "aquatic form",
                "flight form", "swift flight form", "moonkin form", "tree of life", NULL);
        }
        virtual bool isPossible() { return true; }

        virtual bool Execute(Event event);
    };

}
