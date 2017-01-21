/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2017  MaNGOS project <https://getmangos.eu>
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

#include "PetAI.h"
#include "Errors.h"
#include "Pet.h"
#include "Player.h"
#include "DBCStores.h"
#include "Spell.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"
#include "Creature.h"
#include "World.h"
#include "Util.h"

int PetAI::Permissible(const Creature* creature)
{
    if (creature->IsPet())
        return PERMIT_BASE_SPECIAL;

    return PERMIT_BASE_NO;
}

PetAI::PetAI(Creature* c) : CreatureAI(c), i_tracker(TIME_INTERVAL_LOOK), inCombat(false)
{
    m_AllySet.clear();
    UpdateAllies();
}

void PetAI::MoveInLineOfSight(Unit* pWho)
{
    if (Unit* victim = m_creature->getVictim())
        if (victim->IsAlive())
            return;

    if (CharmInfo* charmInfo = m_creature->GetCharmInfo())
    {
        if (charmInfo->HasReactState(REACT_AGGRESSIVE)
            && !(m_creature->IsPet() && ((Pet*)m_creature)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)
            && pWho && pWho->IsTargetableForAttack() && pWho->isInAccessablePlaceFor(m_creature)
            && (m_creature->IsHostileTo(pWho) || pWho->IsHostileTo(m_creature->GetCharmerOrOwner()))
            && m_creature->IsWithinDistInMap(pWho, m_creature->GetAttackDistance(pWho))
            && m_creature->GetDistanceZ(pWho) <= CREATURE_Z_ATTACK_RANGE
            && m_creature->IsWithinLOSInMap(pWho))
        {
            AttackStart(pWho);

            if (Unit* owner = m_creature->GetOwner())
                owner->SetInCombatState(true, pWho);
        }
    }
}

void PetAI::AttackStart(Unit* u)
{
    if (!u || (m_creature->IsPet() && ((Pet*)m_creature)->getPetType() == MINI_PET))
        return;

    if (m_creature->Attack(u, true))
    {
        // TMGs call CreatureRelocation which via MoveInLineOfSight can call this function
        // thus with the following clear the original TMG gets invalidated and crash, doh
        // hope it doesn't start to leak memory without this :-/
        // i_pet->Clear();
        if (!m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
            HandleMovementOnAttackStart(u);

        inCombat = true;
    }
}

void PetAI::EnterEvadeMode()
{
}

bool PetAI::IsVisible(Unit* pl) const
{
    return _isVisible(pl);
}

bool PetAI::_needToStop() const
{
    // This is needed for charmed creatures, as once their target was reset other effects can trigger threat
    if (m_creature->IsCharmed() && m_creature->getVictim() == m_creature->GetCharmer())
        return true;

    return !m_creature->getVictim()->IsTargetableForAttack();
}

void PetAI::_stopAttack()
{
    inCombat = false;

    Unit* owner = m_creature->GetCharmerOrOwner();

    if (owner && m_creature->GetCharmInfo() && m_creature->GetCharmInfo()->HasCommandState(COMMAND_FOLLOW))
    {
        m_creature->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }
    else
    {
        m_creature->GetMotionMaster()->Clear(false);
        m_creature->GetMotionMaster()->MoveIdle();
    }
    m_creature->AttackStop();
}

void PetAI::UpdateAI(const uint32 diff)
{
    if (!m_creature->IsAlive())
        return;

    Unit* owner = m_creature->GetCharmerOrOwner();
    Unit* victim = NULL;

    if (!((Pet*)m_creature)->isControlled())
        m_creature->SelectHostileTarget();

    // Creature pets and guardians will always look in threat list for victim
    if (!(m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)
        || (m_creature->IsPet() && ((Pet*)m_creature)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)))
        victim = m_creature->getVictim();

    if (m_updateAlliesTimer <= diff)
        // UpdateAllies self set update timer
        UpdateAllies();
    else
        m_updateAlliesTimer -= diff;

    if (inCombat && !victim)
    {
        m_creature->AttackStop(true);
        inCombat = false;
    }

    if (((Pet*)m_creature)->GetIsRetreating())
    {
        if (!owner->IsWithinDistInMap(m_creature, (PET_FOLLOW_DIST * 2)))
        {
            if (!m_creature->hasUnitState(UNIT_STAT_FOLLOW))
                m_creature->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

            return;
        }
        else
           ((Pet*)m_creature)->SetIsRetreating();
    }
    else if (((Pet*)m_creature)->GetSpellOpener() != 0) // have opener stored
    {
        uint32 minRange = ((Pet*)m_creature)->GetSpellOpenerMinRange();

        if (!(victim = m_creature->getVictim())
            || (minRange != 0 && m_creature->IsWithinDistInMap(victim, minRange)))
            ((Pet*)m_creature)->SetSpellOpener();
        else if (m_creature->IsWithinDistInMap(victim, ((Pet*)m_creature)->GetSpellOpenerMaxRange())
                && m_creature->IsWithinLOSInMap(victim))
        {
            // stop moving
            m_creature->clearUnitState(UNIT_STAT_MOVING);

            // auto turn to target
            m_creature->SetInFront(victim);

            if (victim->GetTypeId() == TYPEID_PLAYER)
                m_creature->SendCreateUpdateToPlayer((Player*)victim);

            if (owner->GetTypeId() == TYPEID_PLAYER)
                m_creature->SendCreateUpdateToPlayer((Player*)owner);

            uint32 spell_id = ((Pet*)m_creature)->GetSpellOpener();
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

            Spell* spell = new Spell(m_creature, spellInfo, false);

            SpellCastResult result = spell->CheckPetCast(victim);

            if (result == SPELL_CAST_OK)
            {
                m_creature->AddCreatureSpellCooldown(spell_id);
                spell->SpellStart(&(spell->m_targets));
            }
            else
                delete spell;

            ((Pet*)m_creature)->SetSpellOpener();
        }
        else
            return;
    }
    // Autocast (casted only in combat or persistent spells in any state)
    else if (!m_creature->IsNonMeleeSpellCasted(false))
    {
        typedef std::vector<std::pair<Unit*, Spell*> > TargetSpellList;
        TargetSpellList targetSpellStore;

        for (uint8 i = 0; i < m_creature->GetPetAutoSpellSize(); ++i)
        {
            uint32 spellID = m_creature->GetPetAutoSpellOnPos(i);
            if (!spellID)
                continue;

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellID);
            if (!spellInfo)
                continue;

            if (m_creature->GetCharmInfo() && m_creature->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
                continue;

            // ignore some combinations of combat state and combat/noncombat spells
            if (!inCombat)
            {
                // ignore attacking spells, and allow only self/around spells
                if (!IsPositiveSpell(spellInfo->Id))
                    continue;

                // non combat spells allowed
                // only pet spells have IsNonCombatSpell and not fit this reqs:
                // Consume Shadows, Lesser Invisibility, so ignore checks for its
                if (!IsNonCombatSpell(spellInfo))
                {
                    int32 duration = GetSpellDuration(spellInfo);
                    SpellPowerEntry const* spellPower = spellInfo->GetSpellPower();
                    if (spellPower && (spellPower->manaCost || spellPower->ManaCostPercentage || spellPower->manaPerSecond) && duration > 0)
                        continue;

                    // allow only spell without cooldown > duration
                    int32 cooldown = GetSpellRecoveryTime(spellInfo);

                    // allow only spell not on cooldown
                    if (cooldown != 0 && duration < cooldown)
                        continue;
                }
            }
            // just ignore non-combat spells
            else if (IsNonCombatSpell(spellInfo))
                continue;

            Spell* spell = new Spell(m_creature, spellInfo, false);

            if (inCombat && !m_creature->hasUnitState(UNIT_STAT_FOLLOW) && spell->CanAutoCast(victim))
            {
                targetSpellStore.push_back(TargetSpellList::value_type(victim, spell));
                continue;
            }
            else
            {
                bool spellUsed = false;
                for (GuidSet::const_iterator tar = m_AllySet.begin(); tar != m_AllySet.end(); ++tar)
                {
                    Unit* Target = m_creature->GetMap()->GetUnit(*tar);

                    // only buff targets that are in combat, unless the spell can only be cast while out of combat
                    if (!Target)
                        continue;

                    if (spell->CanAutoCast(Target))
                    {
                        targetSpellStore.push_back(TargetSpellList::value_type(Target, spell));
                        spellUsed = true;
                        break;
                    }
                }
                if (!spellUsed)
                    delete spell;
            }
        }

        // found units to cast on to
        if (!targetSpellStore.empty())
        {
            uint32 index = urand(0, targetSpellStore.size() - 1);

            Spell* spell  = targetSpellStore[index].second;
            Unit*  target = targetSpellStore[index].first;

            targetSpellStore.erase(targetSpellStore.begin() + index);

            SpellCastTargets targets;
            targets.setUnitTarget(target);

            if (!m_creature->HasInArc(M_PI_F, target))
            {
                m_creature->SetInFront(target);
                if (target->GetTypeId() == TYPEID_PLAYER)
                    m_creature->SendCreateUpdateToPlayer((Player*)target);

                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    m_creature->SendCreateUpdateToPlayer((Player*)owner);
            }

            m_creature->AddCreatureSpellCooldown(spell->m_spellInfo->Id);

            spell->SpellStart(&targets);
        }

        // deleted cached Spell objects
        for (TargetSpellList::const_iterator itr = targetSpellStore.begin(); itr != targetSpellStore.end(); ++itr)
            delete itr->second;
    }

    // Guardians will always look in threat list for victim
    if (!((Pet*)m_creature)->isControlled())
        m_creature->SelectHostileTarget();

    if (!(m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)
        || (m_creature->IsPet() && ((Pet*)m_creature)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)))
        victim = m_creature->getVictim();

    // Stop here if casting spell (No melee and no movement)
    if (m_creature->IsNonMeleeSpellCasted(false))
        return;

    if (victim)
    {
        // i_pet.getVictim() can't be used for check in case stop fighting, i_pet.getVictim() clear at Unit death etc.
        // This is needed for charmed creatures, as once their target was reset other effects can trigger threat
        if ((m_creature->IsCharmed() && victim == m_creature->GetCharmer()) || !victim->IsTargetableForAttack())
        {
            DEBUG_FILTER_LOG(LOG_FILTER_AI_AND_MOVEGENSS, "PetAI (guid = %u) is stopping attack.", m_creature->GetGUIDLow());
            m_creature->CombatStop();
            inCombat = false;
            
            return;
        }

        // if pet misses its target, it will also be the first in threat list
        if (!(m_creature->GetCreatureInfo()->ExtraFlags & CREATURE_EXTRA_FLAG_NO_MELEE)
            && m_creature->CanReachWithMeleeAttack(victim))
        {
            if (!m_creature->HasInArc(2 * M_PI_F / 3, victim))
            {
                m_creature->SetInFront(victim);
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    m_creature->SendCreateUpdateToPlayer((Player*)victim);

                if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    m_creature->SendCreateUpdateToPlayer((Player*)owner);
            }

            if (DoMeleeAttackIfReady())
                victim->AddThreat(m_creature);
        }
        else if (!(m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE)
            || m_creature->hasUnitState(UNIT_STAT_MOVING)))
            AttackStart(victim);
    }
    else if (owner)
    {
        CharmInfo* charmInfo = m_creature->GetCharmInfo();

        if (owner->IsInCombat() && !(m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)
            || (charmInfo && charmInfo->HasReactState(REACT_PASSIVE))))
            AttackStart(owner->getAttackerForHelper());
        else if (!m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        {
            if (charmInfo && charmInfo->HasCommandState(COMMAND_STAY))
            {
                if (Pet* pet = (Pet*)m_creature)
                {
                    //if stay command is set but we dont have stay pos set then we need to establish current pos as stay position
                    if (!pet->IsStayPosSet())
                        pet->SetStayPosition(true);

                    float stayPosX = pet->GetStayPosX();
                    float stayPosY = pet->GetStayPosY();
                    float stayPosZ = pet->GetStayPosZ();

                    if (m_creature->GetPositionX() == stayPosX
                        && m_creature->GetPositionY() == stayPosY
                        && m_creature->GetPositionZ() == stayPosZ)
                    {
                        float StayPosO = pet->GetStayPosO();

                        if (m_creature->hasUnitState(UNIT_STAT_MOVING))
                        {
                            m_creature->GetMotionMaster()->Clear(false);
                            m_creature->GetMotionMaster()->MoveIdle();
                        }
                        else if (m_creature->GetOrientation() != StayPosO)
                            m_creature->SetOrientation(StayPosO);
                    }
                    else
                        pet->GetMotionMaster()->MovePoint(0, stayPosX, stayPosY, stayPosZ, false);
                }
            }
            else if (m_creature->hasUnitState(UNIT_STAT_FOLLOW))
            {
                if (owner->IsWithinDistInMap(m_creature, PET_FOLLOW_DIST))
                {
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->GetMotionMaster()->MoveIdle();
                }
            }
            else if (charmInfo && charmInfo->HasCommandState(COMMAND_FOLLOW)
                && !owner->IsWithinDistInMap(m_creature, (PET_FOLLOW_DIST * 2)))
                m_creature->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        }
    }
}

bool PetAI::_isVisible(Unit* u) const
{
    return m_creature->IsWithinDist(u, sWorld.getConfig(CONFIG_FLOAT_SIGHT_GUARDER))
           && u->IsVisibleForOrDetect(m_creature, m_creature, true);
}

void PetAI::UpdateAllies()
{
    Unit* owner = m_creature->GetCharmerOrOwner();
    Group* pGroup = NULL;

    m_updateAlliesTimer = 10 * IN_MILLISECONDS;             // update friendly targets every 10 seconds, lesser checks increase performance

    if (!owner)
        return;
    else if (owner->GetTypeId() == TYPEID_PLAYER)
        pGroup = ((Player*)owner)->GetGroup();

    // only pet and owner/not in group->ok
    if (m_AllySet.size() == 2 && !pGroup)
        return;
    // owner is in group; group members filled in already (no raid -> subgroupcount = whole count)
    if (pGroup && !pGroup->isRaidGroup() && m_AllySet.size() == (pGroup->GetMembersCount() + 2))
        return;

    m_AllySet.clear();
    m_AllySet.insert(m_creature->GetObjectGuid());
    if (pGroup)                                             // add group
    {
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* target = itr->getSource();
            if (!target || !pGroup->SameSubGroup((Player*)owner, target))
                continue;

            if (target->GetObjectGuid() == owner->GetObjectGuid())
                continue;

            m_AllySet.insert(target->GetObjectGuid());
        }
    }
    else                                                    // remove group
        m_AllySet.insert(owner->GetObjectGuid());
}

void PetAI::AttackedBy(Unit* attacker)
{
    // when attacked, fight back if no victim unless we have a charm state set to passive
    if (!(m_creature->getVictim() || ((Pet*)m_creature)->GetIsRetreating() == true)
        && !(m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)
        || (m_creature->GetCharmInfo() && m_creature->GetCharmInfo()->HasReactState(REACT_PASSIVE))))
        AttackStart(attacker);
}
