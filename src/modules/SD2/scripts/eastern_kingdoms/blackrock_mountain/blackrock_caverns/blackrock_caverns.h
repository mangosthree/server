/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_BLACKROCK_CAVERNS_H
#define DEF_BLACKROCK_CAVERNS_H

enum
{
    MAX_ENCOUNTER               = 5,

    TYPE_ROMOGG                 = 0,
    TYPE_CORLA                  = 1,
    TYPE_KARSH                  = 2,
    TYPE_BEAUTY                 = 3,
    TYPE_OBSIDIUS               = 4,

    NPC_ROMOGG                  = 39665,
    NPC_CORLA                   = 39679,
    NPC_KARSH                   = 39698,
    NPC_BEAUTY                  = 39700,
    NPC_OBSIDIUS                = 39705,
};

class instance_blackrock_caverns : public ScriptedInstance
{
    public:
        instance_blackrock_caverns(Map* pMap);

        void Initialize() override;

        void OnCreatureCreate(Creature* pCreature) override;
        void OnObjectCreate(GameObject* pGo) override;

        void SetData(uint32 uiType, uint32 uiData) override;
        uint32 GetData(uint32 uiType) const override;

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override;

    private:

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;
};

#endif
