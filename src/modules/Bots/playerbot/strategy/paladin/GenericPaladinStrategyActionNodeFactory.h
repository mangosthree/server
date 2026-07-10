#pragma once

namespace ai
{
    class GenericPaladinStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
    {
    public:
        GenericPaladinStrategyActionNodeFactory()
        {
            creators["seal of insight"] = &seal_of_insight;
            creators["cleanse poison"] = &cleanse_poison;
            creators["cleanse disease"] = &cleanse_disease;
            creators["cleanse magic"] = &cleanse_magic;
            creators["cleanse poison on party"] = &cleanse_poison_on_party;
            creators["cleanse disease on party"] = &cleanse_disease_on_party;
            creators["seal of justice"] = &seal_of_justice;
            creators["hand of reckoning"] = &hand_of_reckoning;
            creators["judgement"] = &judgement;
            creators["divine shield"] = &divine_shield;
            creators["flash of light"] = &flash_of_light;
            creators["flash of light on party"] = &flash_of_light_on_party;
            creators["holy wrath"] = &holy_wrath;
            creators["lay on hands"] = &lay_on_hands;
            creators["lay on hands on party"] = &lay_on_hands_on_party;
            creators["hammer of wrath"] = &hammer_of_wrath;
            creators["retribution aura"] = &retribution_aura;
            creators["blessing of kings"] = &blessing_of_kings;
            creators["blessing of kings on party"] = &blessing_of_kings_on_party;
        }
    private:
        static ActionNode* blessing_of_kings(PlayerbotAI* ai)
        {
            return new ActionNode ("blessing of kings",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* blessing_of_kings_on_party(PlayerbotAI* ai)
        {
            return new ActionNode ("blessing of kings on party",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* retribution_aura(PlayerbotAI* ai)
        {
            return new ActionNode ("retribution aura",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("devotion aura"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* lay_on_hands(PlayerbotAI* ai)
        {
            return new ActionNode ("lay on hands",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("divine shield"), new NextAction("flash of light"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* lay_on_hands_on_party(PlayerbotAI* ai)
        {
            return new ActionNode ("lay on hands on party",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("flash of light"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* seal_of_insight(PlayerbotAI* ai)
        {
            return new ActionNode ("seal of insight",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("seal of justice"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* cleanse_poison(PlayerbotAI* ai)
        {
            return new ActionNode ("cleanse poison",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* cleanse_magic(PlayerbotAI* ai)
        {
            return new ActionNode ("cleanse magic",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* cleanse_disease(PlayerbotAI* ai)
        {
            return new ActionNode ("cleanse disease",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* cleanse_poison_on_party(PlayerbotAI* ai)
        {
            return new ActionNode ("cleanse poison on party",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* cleanse_disease_on_party(PlayerbotAI* ai)
        {
            return new ActionNode ("cleanse disease on party",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* seal_of_justice(PlayerbotAI* ai)
        {
            return new ActionNode ("seal of justice",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("seal of righteousness"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* hand_of_reckoning(PlayerbotAI* ai)
        {
            return new ActionNode ("hand of reckoning",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("judgement"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* judgement(PlayerbotAI* ai)
        {
            return new ActionNode ("judgement",
                /*P*/ NULL,
                /*A*/ NULL,
                /*C*/ NULL);
        }
        static ActionNode* divine_shield(PlayerbotAI* ai)
        {
            return new ActionNode ("divine shield",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("divine protection"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* flash_of_light(PlayerbotAI* ai)
        {
            return new ActionNode ("flash of light",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("holy light"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* flash_of_light_on_party(PlayerbotAI* ai)
        {
            return new ActionNode ("flash of light on party",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("holy light on party"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* holy_wrath(PlayerbotAI* ai)
        {
            return new ActionNode ("holy wrath",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("consecration"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* hammer_of_wrath(PlayerbotAI* ai)
        {
            return new ActionNode ("hammer of wrath",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
                /*C*/ NULL);
        }
    };

};
