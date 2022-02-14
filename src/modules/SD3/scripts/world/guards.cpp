/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2022 MaNGOS <https://getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * ScriptData
 * SDName:      Guards
 * SD%Complete: 100
 * SDComment:   Zone guards
 * SDCategory:  Guards
 * EndScriptData
 */

/**
 * ContentData
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * guard_azuremyst
#endif
 * guard_bluffwatcher
 * guard_contested
 * guard_darnassus
 * guard_dunmorogh
 * guard_durotar
 * guard_elwynnforest
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * guard_eversong
 * guard_exodar
#endif
 * guard_ironforge
 * guard_mulgore
 * guard_orgrimmar
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * guard_shattrath
 * guard_shattrath_aldor
 * guard_shattrath_scryer
 * guard_silvermoon
#endif
 * guard_stormwind
 * guard_teldrassil
 * guard_tirisfal
 * guard_undercity
 * EndContentData
 */

#include "precompiled.h"
#include "guard_ai.h"

struct guard_generic : public CreatureScript
{
    guard_generic() : CreatureScript("guard_generic") {}

    CreatureAI* GetAI(Creature *pCreature) override
    {
        return new guardAI(pCreature);
    }
};

struct guard_orgrimmar : public CreatureScript
{
    guard_orgrimmar() : CreatureScript("guard_orgrimmar") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new guardAI_orgrimmar(pCreature);
    }
};

struct guard_stormwind : public CreatureScript
{
    guard_stormwind() : CreatureScript("guard_stormwind") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new guardAI_stormwind(pCreature);
    }
};

// common AI Part
struct guard_shattrath_asAI : public guardAI
{
    guard_shattrath_asAI(Creature* pCreature, uint32 aura) : guardAI(pCreature), m_uiAuraBanished(aura) {}

    uint32 m_uiExile_Timer;
    uint32 m_uiBanish_Timer;
    uint32 m_uiAuraBanished;
    ObjectGuid m_playerGuid;
    bool m_bCanTeleport;

    void Reset() override
    {
        m_uiBanish_Timer = 5000;
        m_uiExile_Timer = 8500;
        m_playerGuid.Clear();
        m_bCanTeleport = false;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            return;
        }

        if (m_bCanTeleport)
        {
            if (m_uiExile_Timer < uiDiff)
            {
                if (Player* pTarget = m_creature->GetMap()->GetPlayer(m_playerGuid))
                {
                    pTarget->CastSpell(pTarget, SPELL_EXILE, true);
                    pTarget->CastSpell(pTarget, SPELL_BANISH_TELEPORT, true);
                }

                m_playerGuid.Clear();
                m_uiExile_Timer = 8500;
                m_bCanTeleport = false;
            }
            else
            {
                m_uiExile_Timer -= uiDiff;
            }
        }
        else if (m_uiBanish_Timer < uiDiff)
        {
            Unit* pVictim = m_creature->getVictim();

            if (pVictim && pVictim->GetTypeId() == TYPEID_PLAYER)
            {
                DoCastSpellIfCan(pVictim, m_uiAuraBanished);
                m_uiBanish_Timer = 9000;
                m_playerGuid = pVictim->GetObjectGuid();
                m_bCanTeleport = true;
            }
        }
        else
        {
            m_uiBanish_Timer -= uiDiff;
        }

        DoMeleeAttackIfReady();
    }
};

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)

struct guard_shattrath_aldor : public CreatureScript
{
    guard_shattrath_aldor() : CreatureScript("guard_shattrath_aldor") {}

    struct guard_shattrath_aldorAI : public guard_shattrath_asAI
    {
        guard_shattrath_aldorAI(Creature* pCreature) : guard_shattrath_asAI(pCreature, SPELL_BANISHED_SHATTRATH_A) { }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new guard_shattrath_aldorAI(pCreature);
    }
};

struct guard_shattrath_scryer : public CreatureScript
{
    guard_shattrath_scryer() : CreatureScript("guard_shattrath_scryer") {}

    struct guard_shattrath_scryerAI : public guard_shattrath_asAI
    {
        guard_shattrath_scryerAI(Creature* pCreature) : guard_shattrath_asAI(pCreature, SPELL_BANISHED_SHATTRATH_S) { }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new guard_shattrath_scryerAI(pCreature);
    }
};
#endif

void AddSC_guards()
{
    Script* s;
    s = new guard_generic();
    s->RegisterSelf();
    s = new guard_orgrimmar();
    s->RegisterSelf();
    s = new guard_stormwind();
    s->RegisterSelf();

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new guard_shattrath_aldor();
    s->RegisterSelf();
    s = new guard_shattrath_scryer();
    s->RegisterSelf();
#endif

    //pNewScript = new Script;
    //pNewScript->Name = "guard_azuremyst";
    //pNewScript->GetAI = &GetAI_guard_azuremyst;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_contested";
    //pNewScript->GetAI = &GetAI_guard_contested;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_darnassus";
    //pNewScript->GetAI = &GetAI_guard_darnassus;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_dunmorogh";
    //pNewScript->GetAI = &GetAI_guard_dunmorogh;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_durotar";
    //pNewScript->GetAI = &GetAI_guard_durotar;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_elwynnforest";
    //pNewScript->GetAI = &GetAI_guard_elwynnforest;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_ironforge";
    //pNewScript->GetAI = &GetAI_guard_ironforge;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_eversong";
    //pNewScript->GetAI = &GetAI_guard_eversong;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_exodar";
    //pNewScript->GetAI = &GetAI_guard_exodar;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_stormwind";
    //pNewScript->GetAI = &GetAI_guard_stormwind;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_teldrassil";
    //pNewScript->GetAI = &GetAI_guard_teldrassil;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_tirisfal";
    //pNewScript->GetAI = &GetAI_guard_tirisfal;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "guard_shattrath";
    //pNewScript->GetAI = &GetAI_guard_shattrath;
    //pNewScript->RegisterSelf();
}
