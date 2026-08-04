#include "botpch.h"
#include "../../playerbot.h"
#include "../values/LastMovementValue.h"
#include "MovementActions.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "../../FleeManager.h"
#include "../../LootObjectStack.h"
#include "../../PlayerbotAIConfig.h"
#include "MotionGenerators/TargetedMovementGenerator.h"
#include "Creature.h"
#include "Map.h"

using namespace ai;

bool MovementAction::MoveNear(uint32 mapId, float x, float y, float z, float distance)
{
    float angle = GetFollowAngle();
    return MoveTo(mapId, x + cos(angle) * distance, y + sin(angle) * distance, z);
}

bool MovementAction::MoveNear(WorldObject* target, float distance)
{
    if (!target)
    {
        return false;
    }

    distance += target->GetObjectBoundingRadius();

    float followAngle = GetFollowAngle();
    for (float angle = followAngle; angle <= followAngle + 2 * M_PI; angle += M_PI / 4)
    {
        float x = target->GetPositionX() + cos(angle) * distance,
             y = target->GetPositionY()+ sin(angle) * distance,
             z = target->GetPositionZ();
        if (!bot->IsWithinLOS(x, y, z))
        {
            continue;
        }
        bool moved = MoveTo(target->GetMapId(), x, y, z);
        if (moved)
        {
            return true;
        }
    }
    return false;
}

bool MovementAction::MoveTo(uint32 mapId, float x, float y, float z, bool unsafe)
{
    if (!bot->IsUnderWater() && !bot->GetTerrain()->IsInWater(x, y, z))
    {
        bot->UpdateGroundPositionZ(x, y, z);
    }

    if (!IsMovingAllowed(mapId, x, y, z))
    {
        return false;
    }

    // Cautious bots refuse a move whose destination sits inside a hostile
    // creature's aggro zone, unless the caller marks the move as intentional.
    if (!unsafe && ai->HasStrategy("cautious") && IsAggroPosition(x, y))
    {
        return false;
    }

    float distance = bot->GetDistance2d(x, y);
    if (distance > sPlayerbotAIConfig.contactDistance)
    {
        WaitForReach(distance);

        if (bot->IsSitState())
        {
            bot->SetStandState(UNIT_STAND_STATE_STAND);
        }

        if (bot->GetLootGuid())
        {
            bot->SetLootGuid(ObjectGuid());
            bot->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
        }

        if (bot->IsNonMeleeSpellCasted(true))
        {
            bot->CastStop();
            ai->InterruptSpell();
        }

        // Underwater moves used to skip PathFinder entirely, building a raw
        // straight spline with no vmap/WMO collision test -- one of the ways
        // a bot could clip through a cove wall (e.g. Darkbrake Cove) into
        // invalid space. Route underwater moves through PathFinder as well.
        bool generatePath = !bot->IsFlying();
        MotionMaster &mm = *bot->GetMotionMaster();
        mm.MovePoint(mapId, x, y, z, generatePath);

        AI_VALUE(LastMovement&, "last movement").Set(x, y, z, bot->GetOrientation());
        return true;
    }

    return false;
}

bool MovementAction::MoveTo(Unit* target, float distance)
{
    if (!IsMovingAllowed(target))
    {
        return false;
    }

    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();
    float bz = bot->GetPositionZ();

    float tx = target->GetPositionX();
    float ty = target->GetPositionY();
    float tz = target->GetPositionZ();

    float distanceToTarget = bot->GetDistance2d(target);
    float angle = bot->GetAngle(target);
    float needToGo = distanceToTarget - distance;

    float maxDistance = sPlayerbotAIConfig.spellDistance;
    if (needToGo > 0 && needToGo > maxDistance)
    {
        needToGo = maxDistance;
    }
    else if (needToGo < 0 && needToGo < -maxDistance)
    {
        needToGo = -maxDistance;
    }

    float dx = cos(angle) * needToGo + bx;
    float dy = sin(angle) * needToGo + by;

    // Cautious: try the direct beeline first, then progressively wider angles,
    // and take the one that travels farthest without entering an aggro zone --
    // so the bot routes around idle mobs instead of just stopping short.
    if (needToGo != 0)
    {
        float travelAngle = needToGo > 0 ? angle : angle + M_PI;
        float travelDist = fabs(needToGo);

        static const float deltas[] = { 0.0f, M_PI / 6, -M_PI / 6, M_PI / 3, -M_PI / 3, M_PI / 2, -M_PI / 2 };
        float bestSafeDist = 0.0f;
        float bestAngle = travelAngle;
        for (float delta : deltas)
        {
            float safe = CalculateAggroFreeDistance(bx, by, travelAngle + delta, travelDist);
            if (safe > bestSafeDist)
            {
                bestSafeDist = safe;
                bestAngle = travelAngle + delta;
            }
            if (bestSafeDist >= travelDist)
            {
                break;
            }
        }

        float moveDist = std::min(bestSafeDist, travelDist);
        if (moveDist < sPlayerbotAIConfig.contactDistance)
        {
            return false;
        }
        dx = cos(bestAngle) * moveDist + bx;
        dy = sin(bestAngle) * moveDist + by;
    }

    return MoveTo(target->GetMapId(), dx, dy, tz, true);
}

float MovementAction::GetFollowAngle()
{
    Player* master = GetMaster();
    Group* group = master ? master->GetGroup() : bot->GetGroup();
    if (!group)
    {
        return 0.0f;
    }

    int index = 1;
    for (GroupReference *ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        if ( ref->getSource() == master)
        {
            continue;
        }

        if ( ref->getSource() == bot)
        {
            // Fan followers across a rear arc behind the master (centred on
            // straight-behind) instead of a full circle, so bots do not stand in
            // front of / on top of the master.
            int botCount = (int)group->GetMembersCount() - 1;
            return M_PI / 2.0f + M_PI * (index - 1) / std::max(botCount - 1, 1);
        }

        index++;
    }
    return 0;
}

bool MovementAction::IsMovingAllowed(Unit* target)
{
    if (!target)
    {
        return false;
    }

    if (bot->GetMapId() != target->GetMapId())
    {
        return false;
    }

    float distance = bot->GetDistance(target);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed(uint32 mapId, float x, float y, float z)
{
    float distance = bot->GetDistance(x, y, z);
    if (distance > sPlayerbotAIConfig.reactDistance)
    {
        return false;
    }

    return IsMovingAllowed();
}

bool MovementAction::IsMovingAllowed()
{
    if (bot->IsFrozen() || bot->IsPolymorphed() ||
            (bot->IsDead() && !bot->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST)) ||
            bot->IsBeingTeleported() ||
            bot->IsInRoots() ||
            bot->HasAuraType(SPELL_AURA_MOD_CONFUSE) || bot->IsCharmed() ||
            bot->HasAuraType(SPELL_AURA_MOD_STUN) || bot->IsFlying())
        return false;

    MotionMaster &mm = *bot->GetMotionMaster();
    return mm.GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE;
}

bool MovementAction::Follow(Unit* target, float distance)
{
    return Follow(target, distance, GetFollowAngle());
}

bool MovementAction::Follow(Unit* target, float distance, float angle)
{
    MotionMaster &mm = *bot->GetMotionMaster();

    if (!target)
    {
        return false;
    }

    if (bot->GetDistance2d(target->GetPositionX(), target->GetPositionY()) <= sPlayerbotAIConfig.sightDistance &&
            abs(bot->GetPositionZ() - target->GetPositionZ()) >= sPlayerbotAIConfig.spellDistance)
    {
        float x = bot->GetPositionX(), y = bot->GetPositionY(), z = target->GetPositionZ();
        if (target->GetMapId() && bot->GetMapId() != target->GetMapId())
        {
            mm.Clear();
            bot->TeleportTo(target->GetMapId(), x, y, z, bot->GetOrientation());
            AI_VALUE(LastMovement&, "last movement").Set(target);
            return true;
        }

        // Same-map vertical snap used to fire blind, which could drop the
        // bot straight into a cove wall (e.g. Darkbrake Cove, Vashj'ir) and
        // out under the map. Only snap when there is clear LOS to the
        // destination column and the destination itself is valid (swimmable
        // water or a real terrain height); otherwise fall through to the
        // pathfinder-guarded MoveFollow path below. Note the terrain ray
        // tests terrain + WMO + M2 hulls; a collidable doodad on the ray
        // only costs the snap, never a wrong move.
        bool hasLos = bot->GetMap()->IsInLineOfSight(bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ() + 0.5f,
                          x, y, z + 0.5f, bot->GetPhaseMask());
        float heightZ = z;
        bool validDest = bot->GetTerrain()->IsSwimmable(x, y, z, bot->GetObjectBoundingRadius()) ||
                          bot->GetMap()->GetHeightInRange(bot->GetPhaseMask(), x, y, heightZ);
        if (hasLos && validDest)
        {
            mm.Clear();
            bot->Relocate(x, y, z, bot->GetOrientation());
            AI_VALUE(LastMovement&, "last movement").Set(target);
            return true;
        }
    }

    if (!IsMovingAllowed(target))
    {
        return false;
    }

    if (target->IsFriendlyTo(bot) && bot->IsMounted() && AI_VALUE(list<ObjectGuid>, "possible targets").empty())
    {
        distance += angle;
    }

    if (bot->GetDistance2d(target) <= sPlayerbotAIConfig.followDistance)
    {
        return false;
    }

    if (bot->IsSitState())
    {
        bot->SetStandState(UNIT_STAND_STATE_STAND);
    }

    if (bot->IsNonMeleeSpellCasted(true))
    {
        bot->CastStop();
        ai->InterruptSpell();
    }

    AI_VALUE(LastMovement&, "last movement").Set(target);

    if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
    {
        Unit *currentTarget = static_cast<TargetedMovementGenerator const*>(bot->GetMotionMaster()->GetCurrent())->GetTarget();
        if (currentTarget && currentTarget->GetObjectGuid() == target->GetObjectGuid()) return false;
    }

    // Cautious: do not follow into a spot that would pull a hostile creature.
    float followX = target->GetPositionX() + cos(angle) * distance;
    float followY = target->GetPositionY() + sin(angle) * distance;
    if (IsAggroPosition(followX, followY))
    {
        return false;
    }

    mm.MoveFollow(target, distance, angle);
    return true;
}

/*
 * Farthest distance along the beeline from (bx,by) at the given angle that does
 * not enter any hostile creature's aggro zone. Returns maxDist if the path is
 * clear, and is a no-op (returns maxDist) unless the "cautious" strategy is on.
 */
float MovementAction::CalculateAggroFreeDistance(float bx, float by, float angle, float maxDist)
{
    if (!ai->HasStrategy("cautious"))
    {
        return maxDist;
    }

    float cosA = cos(angle);
    float sinA = sin(angle);
    float safeDist = maxDist;

    list<ObjectGuid> targets = AI_VALUE(list<ObjectGuid>, "possible targets");
    for (list<ObjectGuid>::iterator i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !unit->IsAlive() || unit->IsInCombat() || !unit->IsHostileTo(bot) || unit == bot->getVictim())
        {
            continue;
        }

        Creature* creature = dynamic_cast<Creature*>(unit);
        if (!creature || !creature->CanInitiateAttack())
        {
            continue;
        }

        // Solve for where the beeline crosses the creature's aggro circle.
        float aggroRange = creature->GetAttackDistance(bot);
        float ex = bx - creature->GetPositionX();
        float ey = by - creature->GetPositionY();
        float b = ex * cosA + ey * sinA;
        float c = ex * ex + ey * ey - aggroRange * aggroRange;

        float disc = b * b - c;
        if (disc < 0)
        {
            continue;
        }

        float tEntry = -b - sqrt(disc);
        if (tEntry < 0)
        {
            continue;
        }

        if (tEntry < safeDist)
        {
            safeDist = std::max(0.0f, tEntry - 2.0f);
        }
    }

    return safeDist;
}

bool MovementAction::IsAggroPosition(float x, float y)
{
    float bx = bot->GetPositionX();
    float by = bot->GetPositionY();

    float dx = x - bx;
    float dy = y - by;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.1f)
    {
        return false;
    }

    float angle = atan2(dy, dx);
    return CalculateAggroFreeDistance(bx, by, angle, dist) < dist;
}

void MovementAction::WaitForReach(float distance)
{
    float delay = 1000.0f * distance / bot->GetSpeed(MOVE_RUN) + sPlayerbotAIConfig.reactDelay;

    if (delay > sPlayerbotAIConfig.maxWaitForMove)
    {
        delay = sPlayerbotAIConfig.maxWaitForMove;
    }

    Unit* target = *ai->GetAiObjectContext()->GetValue<Unit*>("current target");
    Unit* player = *ai->GetAiObjectContext()->GetValue<Unit*>("enemy player target");
    if ((player || target) && delay > sPlayerbotAIConfig.globalCoolDown)
    {
        delay = sPlayerbotAIConfig.globalCoolDown;
    }

    ai->SetNextCheckDelay((uint32)delay);
}

bool MovementAction::Flee(Unit *target)
{
    Player* master = GetMaster();
    if (!target)
    {
        target = master;
    }

    if (!target)
    {
        return false;
    }

    if (!sPlayerbotAIConfig.fleeingEnabled)
    {
        return false;
    }

    if (!IsMovingAllowed())
    {
        return false;
    }

    FleeManager manager(bot, sPlayerbotAIConfig.fleeDistance, bot->GetAngle(target) + M_PI);

    float rx, ry, rz;
    if (!manager.CalculateDestination(&rx, &ry, &rz))
    {
        return false;
    }

    return MoveTo(target->GetMapId(), rx, ry, rz);
}

bool FleeAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "current target"));
}

bool FleeAction::isUseful()
{
    return AI_VALUE(uint8, "attacker count") > 0 &&
            AI_VALUE2(float, "distance", "current target") <= sPlayerbotAIConfig.shootDistance;
}

bool RunAwayAction::Execute(Event event)
{
    return Flee(AI_VALUE(Unit*, "master target"));
}

template<typename F>
static std::vector<WorldObject*> CollectValidUnits(PlayerbotAI* ai, Player* bot, const std::list<ObjectGuid>& guids, F filter)
{
    std::vector<WorldObject*> results;
    for (std::list<ObjectGuid>::const_iterator i = guids.begin(); i != guids.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (unit && filter(unit) && bot->GetDistance(unit) > sPlayerbotAIConfig.tooCloseDistance)
        {
            results.push_back(unit);
        }
    }
    return results;
}

template<typename F>
static std::vector<WorldObject*> CollectValidGameObjects(PlayerbotAI* ai, Player* bot, const std::list<ObjectGuid>& guids, F filter)
{
    std::vector<WorldObject*> results;
    for (std::list<ObjectGuid>::const_iterator i = guids.begin(); i != guids.end(); ++i)
    {
        GameObject* go = ai->GetGameObject(*i);
        if (go && filter(go) && bot->GetDistance(go) > sPlayerbotAIConfig.tooCloseDistance)
        {
            results.push_back(go);
        }
    }
    return results;
}

bool MoveRandomAction::Execute(Event event)
{
    if (m_hasFaceTarget)
    {
        if (bot->IsStopped())
        {
            m_hasFaceTarget = false;
            bot->SetFacingTo(bot->GetAngle(m_faceX, m_faceY));
        }
        return true;
    }

    WorldObject* target = NULL;

    // If no configured targets, fall through to the random-position fallback.
    if (!sPlayerbotAIConfig.randomMovementTargets.empty())
    {
        // Cache all value queries ONCE per Execute()
        std::list<ObjectGuid> npcs = AI_VALUE(std::list<ObjectGuid>, "nearest npcs");
        std::list<ObjectGuid> players = AI_VALUE(std::list<ObjectGuid>, "nearest players");
        std::list<ObjectGuid> gos = AI_VALUE(std::list<ObjectGuid>, "nearest game objects");

        struct CategoryPick { WorldObject* target; int weight; };
        std::vector<CategoryPick> picks;
        size_t configSize = sPlayerbotAIConfig.randomMovementTargets.size();

        for (size_t idx = 0; idx < configSize; ++idx)
        {
            const std::string& type = sPlayerbotAIConfig.randomMovementTargets[idx];
            int weight = 1 << (int)(configSize - 1 - idx);

            if (type == "random")
            {
                picks.push_back({nullptr, weight});
                continue;
            }

            std::vector<WorldObject*> matches;

            if (type == "anynpcs")
            {
                matches = CollectValidUnits(ai, bot, npcs, [](Unit*) { return true; });
            }
            else if (type == "usefulnpcs")
            {
                matches = CollectValidUnits(ai, bot, npcs, [](Unit* u)
                {
                    Creature* c = dynamic_cast<Creature*>(u);
                    return c && c->GetUInt32Value(UNIT_NPC_FLAGS) != UNIT_NPC_FLAG_NONE;
                });
            }
            else if (type == "players")
            {
                matches = CollectValidUnits(ai, bot, players, [](Unit*) { return true; });
            }
            else if (type == "tradeskillitems")
            {
                for (std::list<ObjectGuid>::const_iterator gi = gos.begin(); gi != gos.end(); ++gi)
                {
                    LootObject loot(bot, *gi);
                    if (loot.skillId != SKILL_NONE && bot->HasSkill(loot.skillId))
                    {
                        GameObject* go = ai->GetGameObject(*gi);
                        if (go && bot->GetDistance(go) > sPlayerbotAIConfig.tooCloseDistance)
                        {
                            matches.push_back(go);
                        }
                    }
                }
            }
            else if (type == "interactableitems")
            {
                matches = CollectValidGameObjects(ai, bot, gos, [](GameObject* go)
                {
                    uint32 t = go->GetGOInfo()->type;
                    return t == GAMEOBJECT_TYPE_CHEST || t == GAMEOBJECT_TYPE_QUESTGIVER ||
                           t == GAMEOBJECT_TYPE_SPELL_FOCUS || t == GAMEOBJECT_TYPE_GOOBER;
                });
            }
            else if (type == "anyitems")
            {
                matches = CollectValidGameObjects(ai, bot, gos, [](GameObject*) { return true; });
            }

            if (!matches.empty())
            {
                picks.push_back({matches[urand(0, matches.size() - 1)], weight});
            }
        }

        if (!picks.empty())
        {
            int totalWeight = 0;
            for (vector<CategoryPick>::const_iterator i = picks.begin(); i != picks.end(); ++i)
            {
                totalWeight += i->weight;
            }

            int roll = urand(0, totalWeight - 1);
            for (vector<CategoryPick>::const_iterator i = picks.begin(); i != picks.end(); ++i)
            {
                if (roll < i->weight)
                {
                    target = i->target;
                    break;
                }
                roll -= i->weight;
            }
        }
    }

    if (target)
    {
        bool moved = MoveNear(target);
        if (moved)
        {
            m_faceX = target->GetPositionX();
            m_faceY = target->GetPositionY();
            m_hasFaceTarget = true;
        }
        return moved;
    }

    // No specific target (empty config, or the "random" category won the
    // weighted roll): fall back to a random ground position near the bot,
    // rejecting underwater/off-mesh points.
    vector<WorldLocation> locs;
    float distance = sPlayerbotAIConfig.grindDistance;
    Map* map = bot->GetMap();
    for (int i = 0; i < 10; ++i)
    {
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        float z = bot->GetPositionZ();
        x += urand(0, distance) - distance / 2;
        y += urand(0, distance) - distance / 2;
        bot->UpdateGroundPositionZ(x, y, z);

        const TerrainInfo* terrain = map->GetTerrain();
        if (terrain->IsUnderWater(x, y, z) ||
            terrain->IsInWater(x, y, z))
            continue;

        float ground = map->GetHeight(PHASEMASK_NORMAL, x, y, z + 0.5f);
        if (ground <= INVALID_HEIGHT)
        {
            continue;
        }

        z = 0.05f + ground;
        if (abs(z - bot->GetPositionZ()) > sPlayerbotAIConfig.tooCloseDistance)
        {
            continue;
        }

        WorldLocation loc(bot->GetMapId(), x, y, z);
        locs.push_back(loc);
    }

    if (locs.empty())
    {
        return false;
    }

    WorldLocation randomLoc = locs[urand(0, locs.size() - 1)];
    return MoveNear(randomLoc.mapid, randomLoc.coord_x, randomLoc.coord_y, randomLoc.coord_z);
}

bool MoveToLootAction::Execute(Event event)
{
    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
    {
        return false;
    }

    WorldObject *wo = loot.GetWorldObject(bot);
    return MoveNear(wo);
}

bool MoveOutOfEnemyContactAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    return MoveNear(target, sPlayerbotAIConfig.meleeDistance);
}

bool MoveOutOfEnemyContactAction::isUseful()
{
    return AI_VALUE2(float, "distance", "current target") < (sPlayerbotAIConfig.meleeDistance + sPlayerbotAIConfig.contactDistance);
}

bool SetFacingTargetAction::Execute(Event event)
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }

    if (bot->IsTaxiFlying())
    {
        return true;
    }

    bot->SetFacingTo(bot->GetAngle(target));
    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    return true;
}

bool SetFacingTargetAction::isUseful()
{
    return !AI_VALUE2(bool, "facing", "current target");
}

bool SwimToSurfaceAction::isUseful()
{
    return bot->IsUnderWater();
}

bool SwimToSurfaceAction::Execute(Event event)
{
    float x = bot->GetPositionX();
    float y = bot->GetPositionY();
    float z = bot->GetPositionZ();

    const auto waterLevel = bot->GetTerrain()->GetWaterLevel(x, y, z);
    if (!waterLevel)
    {
        return false;
    }

    ai->SetNextCheckDelay(sPlayerbotAIConfig.globalCoolDown);
    return MoveTo(bot->GetMapId(), x, y, *waterLevel - 0.5f);
}
