#include "Config/Config.h"
#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "Player.h"
#include "AiFactory.h"
#include "GuildTaskMgr.h"
#include "PlayerbotCommandServer.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "FleeManager.h"
#include "GridDefines.h"
#include "Map.h"
#include "Timer.h"
#include "MapManager.h"
#include "MoveMap.h"
#include "PathFinder.h"

using namespace ai;
using namespace MaNGOS;

INSTANTIATE_SINGLETON_1(RandomPlayerbotMgr);

/**
 * RandomPlayerbotMgr is responsible for managing random player bots in the game.
 * It handles the creation, updating, and processing of these bots, ensuring they
 * behave in a way that simulates real player activity.
 */
RandomPlayerbotMgr::RandomPlayerbotMgr() : PlayerbotHolder(), processTicks(0), m_processBotCursor(0)
{
    sPlayerbotCommandServer.Start();
    //PrepareTeleportCache();
}

RandomPlayerbotMgr::~RandomPlayerbotMgr()
{
}

int RandomPlayerbotMgr::GetMaxAllowedBotCount()
{
    return GetEventValue(0, "bot_count");
}

void RandomPlayerbotMgr::UpdateAIInternal(uint32 elapsed)
{
    SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateInterval * 1000);

    if (!sPlayerbotAIConfig.randomBotAutologin || !sPlayerbotAIConfig.enabled)
    {
        return;
    }

    sLog.outBasic("Processing random bots...");

    uint32 cachedMin = GetEventValue(0, "config_min");
    uint32 cachedMax = GetEventValue(0, "config_max");

    if (cachedMin != sPlayerbotAIConfig.minRandomBots ||
        cachedMax != sPlayerbotAIConfig.maxRandomBots)
    {
        sLog.outString("Bot count range changed from %d-%d to %d-%d, regenerating target...",
            cachedMin, cachedMax,
            sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);

        SetEventValue(0, "bot_count", 0, 0);  // Invalidate
        SetEventValue(0, "config_min", sPlayerbotAIConfig.minRandomBots, 999999);
        SetEventValue(0, "config_max", sPlayerbotAIConfig.maxRandomBots, 999999);
    }

    int maxAllowedBotCount = GetEventValue(0, "bot_count");
    if (!maxAllowedBotCount)
    {
        maxAllowedBotCount = urand(sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);
        SetEventValue(0, "bot_count", maxAllowedBotCount,
            urand(sPlayerbotAIConfig.randomBotCountChangeMinInterval, sPlayerbotAIConfig.randomBotCountChangeMaxInterval));
    }

    list<uint32> bots = GetBots();
    int botCount = bots.size();
    int randomBotsPerInterval = (int)urand(sPlayerbotAIConfig.minRandomBotsPerInterval, sPlayerbotAIConfig.maxRandomBotsPerInterval);
    if (!processTicks)
    {
        if (sPlayerbotAIConfig.randomBotLoginAtStartup)
        {
            randomBotsPerInterval = bots.size();
        }
    }

    if (botCount < maxAllowedBotCount)
    {
        AddRandomBots();
    }

    // Pace the pass against a wall-clock budget so a single update tick can not be
    // monopolised by a burst of bot logins/randomizes/saves. Work that does not fit
    // in the budget is carried over to the next (soon-rescheduled) pass.
    uint32 passStart = getMSTime();
    uint32 budgetMs = sPlayerbotAIConfig.randomBotProcessBudgetMs;
    bool overBudget = false;

    // Resume from the bot after the one last examined so a pass that stops early on
    // its time budget or per-interval cap still works through every bot over
    // successive passes instead of repeatedly re-processing the head of the list.
    // A GUID cursor (not an index) is used because GetBots() is unordered and churns.
    int botProcessed = 0;
    if (!bots.empty())
    {
        list<uint32>::iterator i = bots.begin();
        if (m_processBotCursor)
        {
            list<uint32>::iterator cursor = std::find(bots.begin(), bots.end(), m_processBotCursor);
            if (cursor != bots.end())
            {
                i = cursor;
                if (++i == bots.end())
                {
                    i = bots.begin();
                }
            }
        }

        for (size_t examined = 0; examined < bots.size(); ++examined)
        {
            if (budgetMs && getMSTimeDiff(passStart, getMSTime()) >= budgetMs)
            {
                overBudget = true;
                break;
            }

            uint32 bot = *i;
            m_processBotCursor = bot;

            if (ProcessBot(bot))
            {
                botProcessed++;
            }

            if (botProcessed >= randomBotsPerInterval)
            {
                break;
            }

            if (++i == bots.end())
            {
                i = bots.begin();
            }
        }
    }

    // Still work left this pass - come back quickly instead of waiting a full interval.
    if (overBudget && sPlayerbotAIConfig.randomBotCatchupInterval < sPlayerbotAIConfig.randomBotUpdateInterval)
    {
        SetNextCheckDelay(sPlayerbotAIConfig.randomBotCatchupInterval * 1000);
    }

    // Internal scheduler status: keep it out of player chat (was broadcast via
    // SendWorldText every pass = chat spam) and off the console at normal levels.
    sLog.outDetail("Random bots processed; next pass in %u seconds",
        overBudget ? sPlayerbotAIConfig.randomBotCatchupInterval : sPlayerbotAIConfig.randomBotUpdateInterval);

    // PrintStats walks every bot and logs a full breakdown; running it on every
    // pass floods the log, so emit it roughly once a minute.
    if (processTicks % 12 == 0)
    {
        PrintStats();
    }

    // Advance the pass counter. It was never incremented, so the "!processTicks"
    // startup-burst branch above ran on *every* pass -- forcing a full-bot-list
    // process each time instead of only at startup. Incrementing restores the
    // intended one-time burst and steady per-interval throttling afterwards.
    ++processTicks;
}

uint32 RandomPlayerbotMgr::AddRandomBots()
{
    set<uint32> bots;

    QueryResult* results = CharacterDatabase.PQuery(
        "select `bot` from ai_playerbot_random_bots where event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.insert(bot);
        } while (results->NextRow());
        delete results;
    }

    vector<uint32> guids;
    int maxAllowedBotCount = GetEventValue(0, "bot_count");
    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); i++)
    {
        uint32 accountId = *i;
        if (!sAccountMgr.GetCharactersCount(accountId))
        {
            continue;
        }

        QueryResult* result = CharacterDatabase.PQuery("SELECT guid, race FROM characters WHERE account = '%u'", accountId);
        if (!result)
        {
            continue;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint8 race = fields[1].GetUInt8();
            bool alliance = guids.size() % 2 == 0;
            if (bots.find(guid) == bots.end() &&
                ((alliance && IsAlliance(race)) || ((!alliance && !IsAlliance(race))
                    )))
            {
                guids.push_back(guid);
                uint32 bot = guid;
                SetEventValue(bot, "add", 1, urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime));
                uint32 randomTime = 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
                ScheduleRandomize(bot, randomTime);
                bots.insert(bot);
                sLog.outString("New random bot %d added", bot);
                if (bots.size() >= maxAllowedBotCount) break;
            }
        } while (result->NextRow());
        delete result;
    }

    return guids.size();
}

void RandomPlayerbotMgr::ScheduleRandomize(uint32 bot, uint32 time)
{
    SetEventValue(bot, "randomize", 1, time);
    SetEventValue(bot, "logout", 1, time + 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

void RandomPlayerbotMgr::ScheduleTeleport(uint32 bot, uint32 time)
{
    if (!time)
    {
        time = 60 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
    }
    SetEventValue(bot, "teleport", 1, time);
}

bool RandomPlayerbotMgr::ProcessBot(uint32 bot)
{
    uint32 isValid = GetEventValue(bot, "add");
    if (!isValid)
    {
        Player* player = GetPlayerBot(bot);
        if (!player || !player->GetGroup())
        {
            sLog.outString("Bot %d expired", bot);
            SetEventValue(bot, "add", 0, 0);
        }
        return true;
    }

    // Offline-dead / sentinel-wedged bots: the dead->revive branch normally
    // lives in ProcessBot(Player*), which only runs for bots that are online
    // and ticking their own AI trigger. A bot that logs out while dead - or
    // whose "dead" row is wedged behind a runaway validIn (e.g. the historic
    // 2000000000-second sentinel) - would then never get revived. Drain it
    // here instead, independent of online status, every scheduler pass.
    uint32 deadFlag = GetEventValue(bot, "dead");
    if (deadFlag)
    {
        QueryResult* deadRow = CharacterDatabase.PQuery(
            "SELECT `validIn` FROM `ai_playerbot_random_bots` "
            "WHERE `owner` = 0 AND `bot` = '%u' AND `event` = 'dead'", bot);
        if (deadRow)
        {
            uint32 deadValidIn = deadRow->Fetch()[0].GetUInt32();
            delete deadRow;

            uint32 const sentinelValidIn = sPlayerbotAIConfig.maxRandomBotReviveTime * 10;
            if (deadValidIn >= sentinelValidIn)
            {
                sLog.outDetail("Bot %d dead row wedged (validIn=%u), self-healing", bot, deadValidIn);
                SetEventValue(bot, "dead", 0, 0);
                SetEventValue(bot, "revive", 0, 0);
                deadFlag = 0;
            }
        }
    }

    if (deadFlag && !GetEventValue(bot, "revive"))
    {
        Player* player = GetPlayerBot(bot);
        if (!player)
        {
            sLog.outDetail("Bot %d logged in for offline revive", bot);
            AddPlayerBot(bot, 0);
            player = GetPlayerBot(bot);
        }

        if (player)
        {
            Revive(player);
        }
        return true;
    }

    if (!GetPlayerBot(bot))
    {
        sLog.outDetail("Bot %d logged in", bot);
        AddPlayerBot(bot, 0);
        if (!GetEventValue(bot, "online"))
        {
            SetEventValue(bot, "online", 1, sPlayerbotAIConfig.minRandomBotInWorldTime);
            ScheduleTeleport(bot, 30);
        }
        return true;
    }

    Player* player = GetPlayerBot(bot);
    if (!player)
    {
        return false;
    }

    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai)
    {
        return false;
    }

    if (player->GetGroup())
    {
        sLog.outString("Skipping bot %d as it is in group", bot);
        return false;
    }

    ai->GetAiObjectContext()->GetValue<bool>("random bot update")->Set(true);
    return true;
}

bool RandomPlayerbotMgr::ProcessBot(Player* player)
{
    player->GetPlayerbotAI()->GetAiObjectContext()->GetValue<bool>("random bot update")->Set(false);

    uint32 bot = player->GetGUIDLow();
    if (player->IsDead())
    {
        if (!GetEventValue(bot, "dead"))
        {
            sLog.outDetail("Setting dead flag for bot %d", bot);
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotReviveTime, sPlayerbotAIConfig.maxRandomBotReviveTime);
            SetEventValue(bot, "dead", 1, randomTime);
            SetEventValue(bot, "revive", 1, randomTime - 60);
            return false;
        }

        if (!GetEventValue(bot, "revive"))
        {
            Revive(player);
            return true;
        }

        return false;
    }

    // Vashj'ir survival: keep the Sea Legs water-breathing buff on random bots in-zone.
    static uint32 const SPELL_SEA_LEGS = 73701;
    if (player->IsInVashjir())
    {
        if (!player->HasAura(SPELL_SEA_LEGS))
        {
            player->CastSpell(player, SPELL_SEA_LEGS, true);
        }
    }
    else if (player->HasAura(SPELL_SEA_LEGS))
    {
        player->RemoveAurasDueToSpell(SPELL_SEA_LEGS);
    }

    if (player->GetGuildId())
    {
        Guild* guild = sGuildMgr.GetGuildById(player->GetGuildId());
        if (guild->GetLeaderGuid().GetRawValue() == player->GetObjectGuid().GetRawValue()) {
            for (vector<Player*>::iterator i = players.begin(); i != players.end(); ++i)
            {
                sGuildTaskMgr.Update(*i, player);
            }
        }
    }

    uint32 randomize = GetEventValue(bot, "randomize");
    if (!randomize)
    {
        sLog.outString("Randomizing bot %d", bot);
        Randomize(player);
        uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
        ScheduleRandomize(bot, randomTime);
        return true;
    }

    uint32 logout = GetEventValue(bot, "logout");
    if (!logout)
    {
        sLog.outString("Logging out bot %d", bot);
        LogoutPlayerBot(bot);
        SetEventValue(bot, "logout", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    if (!IsZoneSafeForBot(player, player->GetMapId(), player->GetPositionX(),
                          player->GetPositionY(), player->GetPositionZ()))
    {
        sLog.outDetail("Bot %d is in unsafe zone, forcing teleport", bot);
        RandomTeleportForLevel(player);
        SetEventValue(bot, "teleport", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    // Check if bot level is outside configured min/max range
    uint32 botLevel = player->getLevel();
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }
    if (botLevel < sPlayerbotAIConfig.randomBotMinLevel || botLevel > maxLevel)
    {
        sLog.outDetail("Bot %d level %d is outside valid range (%d-%d), scheduling immediate re-randomization",
                       bot, botLevel, sPlayerbotAIConfig.randomBotMinLevel, maxLevel);
        ScheduleRandomize(bot, 0);
        return true;
    }

    uint32 teleport = GetEventValue(bot, "teleport");
    if (!teleport)
    {
        sLog.outDetail("Random teleporting bot %d", bot);
        RandomTeleportForLevel(player);
        SetEventValue(bot, "teleport", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::Revive(Player* player)
{
    uint32 bot = player->GetGUIDLow();
    sLog.outDetail("Reviving dead bot %d", bot);
    SetEventValue(bot, "dead", 0, 0);
    SetEventValue(bot, "revive", 0, 0);
    Refresh(player);
    RandomTeleportForLevel(player);
}

/// Rejects void/off-mesh/deep-water destinations; snaps z to ground on success
bool RandomPlayerbotMgr::IsSafeTeleportPosition(uint32 mapId, float x, float y, float& z)
{
    static float const MAX_SAFE_WATER_DEPTH = 4.0f;
    static float const NAVMESH_SEARCH_EXTENTS[VERTEX_SIZE] = { 3.0f, 5.0f, 3.0f };

    Map* map = sMapMgr.FindMap(mapId);
    if (!map)
    {
        return false;
    }

    TerrainInfo const* terrain = map->GetTerrain();
    if (!terrain)
    {
        return false;
    }

    // Ground snap + void check: reject destinations with no map data underneath.
    float groundZ = terrain->GetHeightStatic(x, y, z, true, MAX_HEIGHT);
    if (groundZ <= INVALID_HEIGHT)
    {
        return false;
    }

    // Vashj'ir is an underwater zone by design: bots are kept alive there via the
    // Sea Legs buff + fatigue/breath exemption, and they swim rather than path on
    // the navmesh. Exempt it from the deep-water and off-mesh rejections below,
    // but leave every other deep-ocean destination rejected as before.
    uint32 zoneId = terrain->GetZoneId(x, y, groundZ);
    bool inVashjir = Player::IsVashjirZone(zoneId);

    // Deep-water reject by depth, not a bare in/under-water boolean, so shallow
    // fords and beaches stay teleportable while Vashj'ir-style deep ocean does not.
    if (!inVashjir)
    {
        GridMapLiquidData liquidData;
        GridMapLiquidStatus liquidStatus = terrain->getLiquidStatus(x, y, groundZ, MAP_ALL_LIQUIDS, &liquidData);
        if (liquidStatus != LIQUID_MAP_NO_WATER && liquidData.level - liquidData.depth_level > MAX_SAFE_WATER_DEPTH)
        {
            return false;
        }
    }

    // Navmesh reachability kills cliff/fall/inside-geometry destinations. If the
    // mmap isn't loaded for this map, skip the check rather than hard-failing so
    // non-mmap maps stay teleportable. Vashj'ir dark-water quads carry no poly,
    // so the check would reject every point there; swimming bots reach the spot
    // via PathFinder::BuildSwimShortcut regardless.
    if (!inVashjir)
    {
        MMAP::MMapManager* mmapManager = MMAP::MMapFactory::createOrGetMMapManager();
        dtNavMesh const* navMesh = mmapManager->GetNavMesh(mapId);
        dtNavMeshQuery const* navMeshQuery = mmapManager->GetNavMeshQuery(mapId, 0);
        if (navMesh && navMeshQuery)
        {
            float const center[VERTEX_SIZE] = { y, groundZ, x };
            float closestPoint[VERTEX_SIZE];
            dtQueryFilter filter;
            dtPolyRef polyRef = INVALID_POLYREF;
            navMeshQuery->findNearestPoly(center, NAVMESH_SEARCH_EXTENTS, &filter, &polyRef, closestPoint);
            if (polyRef == INVALID_POLYREF)
            {
                return false;
            }

            groundZ = closestPoint[1];
        }
    }

    // Small lift off the ground plane to avoid landing embedded in geometry,
    // matching the epsilon the prior teleport code used.
    z = groundZ + 0.05f;
    return true;
}

/**
 * @brief One-shot expiry for post-teleport spawn protection on a random bot.
 * Clears UNIT_FLAG_NON_ATTACKABLE whether the timer runs to completion or
 * the event is aborted early (bot despawned/removed first), so the flag
 * can never get stuck on.
 */
class RandomBotSpawnProtectionEvent : public BasicEvent
{
    public:
        explicit RandomBotSpawnProtectionEvent(Unit& owner) : BasicEvent(), m_owner(owner) {}

        bool Execute(uint64 /*e_time*/, uint32 /*p_time*/)
        {
            m_owner.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            return true;
        }

        void Abort(uint64 /*e_time*/)
        {
            m_owner.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

    private:
        Unit& m_owner;
};

void RandomPlayerbotMgr::RandomTeleport(Player* bot, vector<WorldLocation> &locs)
{
    // Brief, self-expiring window so a bot survives initial contact on a
    // dense spawn (e.g. Vashj'ir/L'ghorek) and its AI can orient/react
    // before being focus-fired; not a permanent invulnerability. Tune this
    // if bots still die on arrival, or if it trivialises normal combat.
    static uint32 const SPAWN_PROTECTION_DURATION_MS = 10000;

    if (bot->IsBeingTeleported())
    {
        return;
    }

    if (locs.empty())
    {
        sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
        return;
    }

    for (int attemtps = 0; attemtps < 10; ++attemtps)
    {
        int index = urand(0, locs.size() - 1);
        WorldLocation loc = locs[index];
        float x = loc.coord_x + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float y = loc.coord_y + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float z = loc.coord_z;

        if (!IsSafeTeleportPosition(loc.mapid, x, y, z))
        {
            continue;
        }

        sLog.outDetail("Random teleporting bot %s to %u %f,%f,%f", bot->GetName(), loc.mapid, x, y, z);
        bot->TeleportTo(loc.mapid, x, y, z, 0);

        // Applied to all random teleports, not just Vashj'ir - landing
        // inside a dense mob pack is a general risk for any destination.
        if (!bot->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
        {
            bot->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            bot->m_Events.AddEvent(new RandomBotSpawnProtectionEvent(*bot),
                bot->m_Events.CalculateTime(SPAWN_PROTECTION_DURATION_MS));
        }
        return;
    }

    sLog.outError("Cannot teleport bot %s - no locations available", bot->GetName());
}

void RandomPlayerbotMgr::RandomTeleportForLevel(Player* bot)
{
    // Bounded to avoid sorting/scanning every matching spawn on maps with a
    // huge creature count; tune against live query performance if needed.
    static uint32 const RANDOM_TELEPORT_CANDIDATE_LIMIT = 300;

    vector<WorldLocation> locs;
    QueryResult* results = WorldDatabase.PQuery(
        "SELECT `c`.`map`, `c`.`position_x`, `c`.`position_y`, `c`.`position_z` FROM `creature` `c` "
        "INNER JOIN `creature_template` `t` ON `c`.`id` = `t`.`entry` "
        "WHERE (%u - (`t`.`maxlevel` + `t`.`minlevel`) / 2) BETWEEN 0 AND %u AND `c`.`map` IN (%s) "
        "ORDER BY RAND() LIMIT %u",
        bot->getLevel(), sPlayerbotAIConfig.randomBotTeleLevel, sPlayerbotAIConfig.randomBotMapsAsString.c_str(),
        RANDOM_TELEPORT_CANDIDATE_LIMIT);

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 mapId = fields[0].GetUInt32();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            if (IsZoneSafeForBot(bot, mapId, x, y, z))
            {
                WorldLocation loc(mapId, x, y, z, 0);
                locs.push_back(loc);
            }
        } while (results->NextRow());
        delete results;
    }

    RandomTeleport(bot, locs);
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, uint32 mapId, float teleX, float teleY, float teleZ)
{
    Refresh(bot);

    vector<WorldLocation> locs;
    QueryResult* results = WorldDatabase.PQuery("SELECT `position_x`, `position_y`, `position_z` FROM `creature` WHERE `map` = '%u' AND ABS(`position_x` - '%f') < '%u' AND ABS(`position_y` - '%f') < '%u'",
        +mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            float x = fields[0].GetFloat();
            float y = fields[1].GetFloat();
            float z = fields[2].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
        } while (results->NextRow());
        delete results;
    }

    RandomTeleport(bot, locs);
    Refresh(bot);
}

void RandomPlayerbotMgr::Randomize(Player* bot)
{
    if (bot->getLevel() == 1)
    {
        RandomizeFirst(bot);
    }
    else
    {
        IncreaseLevel(bot);
    }
}

void RandomPlayerbotMgr::IncreaseLevel(Player* bot)
{
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    // A bot already at the level cap has nowhere to grow: re-roll it fresh
    // (new level/zone/build via RandomizeFirst) instead of a no-op increment,
    // so the population keeps cycling instead of stagnating at the cap.
    if (bot->getLevel() >= maxLevel)
    {
        RandomizeFirst(bot);
        return;
    }

    uint32 level = min((uint32)(bot->getLevel() + 1), maxLevel);
    PlayerbotFactory factory(bot, level);
    if (bot->GetGuildId())
    {
        factory.Refresh();

        // Guilded bots skip the full randomize (Refresh only re-rolls gear), so
        // repair a broken build in place. Respec when the tree is unset (a
        // pre-Cata build) or points are unspent.
        bool changed = false;
        if (!bot->GetPrimaryTalentTree(bot->GetActiveSpec()) || bot->GetFreeTalentPoints())
        {
            factory.InitTalents();
            changed = true;
        }

        // Fill glyphs for bots that have slots (level 25+) but none applied.
        bool hasGlyph = false;
        for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
        {
            if (bot->GetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot))
            {
                hasGlyph = true;
                break;
            }
        }
        if (bot->getLevel() >= 25 && !hasGlyph)
        {
            factory.InitGlyphs();
            changed = true;
        }

        if (changed)
        {
            bot->SaveToDB();
        }
    }
    else
    {
        factory.Randomize();
    }
    RandomTeleportForLevel(bot);
}

void RandomPlayerbotMgr::RandomizeFirst(Player* bot)
{
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    }

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        int index = urand(0, sPlayerbotAIConfig.randomBotMaps.size() - 1);
        uint32 mapId = sPlayerbotAIConfig.randomBotMaps[index];

        vector<GameTele const*> locs;
        GameTeleMap const & teleMap = sObjectMgr.GetGameTeleMap();
        for (GameTeleMap::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
        {
            GameTele const* tele = &itr->second;
            // Collect all teleports on this map; zone safety is checked below
            // against the bot's *target* level (computed from the zone), not the
            // fresh bot's level 1, so a new bot is not confined to low-level zones.
            if (tele->mapId == mapId)
            {
                locs.push_back(tele);
            }
        }
        if (locs.empty()) // no safe locations found, so try another map
        {
            continue;
        }

        index = urand(0, locs.size() - 1);
        if (index >= locs.size())
        {
            return;
        }
        GameTele const* tele = locs[index];
        uint32 level = GetZoneLevel(tele->mapId, tele->position_x, tele->position_y, tele->position_z);
        if (level > maxLevel + 5)
        {
            continue;
        }

        level = min(level, maxLevel);
        if (!level) level = 1;

        // only create a high level mob if they are in a high level zone
        if ((urand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance) && level >= 40)
        {
            level = maxLevel;
        }

        if (level < sPlayerbotAIConfig.randomBotMinLevel)
        {
            continue;
        }

        // Now that the target level is known, reject the zone if its creature band
        // (or opposing-faction guards) does not fit that level.
        if (!IsZoneSafeForBot(bot, tele->mapId, tele->position_x, tele->position_y, tele->position_z, level))
        {
            continue;
        }

        PlayerbotFactory factory(bot, level);
        factory.CleanRandomize();
        RandomTeleportForLevel(bot);
        break;
    }
}

uint32 RandomPlayerbotMgr::GetZoneLevel(uint16 mapId, float teleX, float teleY, float teleZ)
{
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);

    uint32 level;
    QueryResult *results = WorldDatabase.PQuery("SELECT AVG(`t`.`minlevel`) `minlevel`, AVG(`t`.`maxlevel`) `maxlevel` FROM `creature` `c` "
            "INNER JOIN `creature_template` `t` ON `c`.`id` = `t`.`entry` "
            "WHERE `map` = '%u' AND `minlevel` > 1 AND ABS(`position_x` - '%f') < '%u' AND ABS(`position_y` - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);

    if (results)
    {
        Field* fields = results->Fetch();
        uint8 minLevel = fields[0].GetUInt8();
        uint8 maxLevel = fields[1].GetUInt8();
        level = urand(minLevel, maxLevel);
        if (level > maxLevel)
        {
            level = maxLevel;
        }
        delete results;
    }
    else
    {
        level = urand(1, maxLevel);
    }

    return level;
}

void RandomPlayerbotMgr::Refresh(Player* bot)
{
    sLog.outDetail("Refreshing bot %s", bot->GetName());
    if (bot->IsDead())
    {
        // Resurrect the bot if it is dead
        bot->ResurrectPlayer(1.0f);
        bot->SpawnCorpseBones();
        bot->GetPlayerbotAI()->ResetStrategies();
    }

    // Reset the bot's AI
    bot->GetPlayerbotAI()->Reset();

    // Clear all hostile references and combat states
    HostileReference *ref = bot->GetHostileRefManager().getFirst();
    while (ref)
    {
        ThreatManager *threatManager = ref->getSource();
        Unit *unit = threatManager->getOwner();
        float threat = ref->getThreat();

        unit->RemoveAllAttackers();
        unit->ClearInCombat();

        ref = ref->next();
    }

    bot->RemoveAllAttackers();
    bot->ClearInCombat();

    // Repair all items, set health and power to maximum, and enable PvP
    bot->DurabilityRepairAll(false, 1.0f, false);
    bot->SetHealthPercent(100);
    bot->SetPvP(true);

    if (bot->GetMaxPower(POWER_MANA) > 0)
    {
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
    }

    if (bot->GetMaxPower(POWER_ENERGY) > 0)
    {
        bot->SetPower(POWER_ENERGY, bot->GetMaxPower(POWER_ENERGY));
    }
}

bool RandomPlayerbotMgr::IsRandomBot(Player* bot)
{
    if (!bot)
    {
        return false;
    }
    return IsRandomBot(bot->GetGUIDLow());
}

bool RandomPlayerbotMgr::IsRandomBot(uint32 bot)
{
    std::unordered_map<uint32, bool>::iterator it = m_randomBotCache.find(bot);
    if (it != m_randomBotCache.end())
    {
        return it->second;
    }

    bool value = (GetEventValue(bot, "add") != 0);
    m_randomBotCache[bot] = value;
    return value;
}

list<uint32> RandomPlayerbotMgr::GetBots()
{
    list<uint32> bots;

    // Query the database to get the list of random bots
    QueryResult* results = CharacterDatabase.Query(
            "SELECT `bot` FROM `ai_playerbot_random_bots` WHERE `owner` = 0 AND `event` = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = fields[0].GetUInt32();
            bots.push_back(bot);
        } while (results->NextRow());
        delete results;
    }

    return bots;
}

bool RandomPlayerbotMgr::IsZoneSafeForBot(Player* bot, uint32 mapId, float x, float y, float z, uint32 useLevel)
{
    Map* map = sMapMgr.FindMap(mapId);
    if (!map)
        return false;
    TerrainInfo const* terrain = map->GetTerrain();
    if (!terrain)
        return false;

    CellPair cell_pair = MaNGOS::ComputeCellPair(x, y);
    uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;
    std::pair<uint32, uint32> mapCell = std::make_pair(mapId, cell_id);

    uint32 areaId = 0;
    std::map<std::pair<uint32, uint32>, uint32>::iterator cacheItr = m_cellToAreaCache.find(mapCell);
    if (cacheItr != m_cellToAreaCache.end())
    {
        areaId = cacheItr->second;
    }
    else
    {
        areaId = terrain->GetAreaId(x, y, z);
        m_cellToAreaCache[mapCell] = areaId;
    }

    AreaTableEntry const* area = sAreaStore.LookupEntry(areaId);
    if (!area)
        return true;

    // Area creature-level bands and contested-zone guard areas are scanned once
    // (lazily) and cached. The scan walks every creature in the world (hundreds
    // of thousands of terrain lookups = tens of seconds) on the world thread, so
    // it is opt-in via AiPlayerbot.SpawnZoneStats; without it, only the cheap
    // faction/team zone check applies. The computed-once flag stops an empty
    // result from re-triggering the scan every call.
    if (sPlayerbotAIConfig.spawnZoneStats && !m_areaCreatureStatsComputed)
    {
        CalculateAreaCreatureStats();
    }

    if (area->team != AREATEAM_NONE)
    {
        bool botIsAlliance = IsAlliance(bot->getRace());
        if (botIsAlliance && area->team != AREATEAM_ALLY)
            return false;
        if (!botIsAlliance && area->team != AREATEAM_HORDE)
            return false;
    }
    else // contested area: keep bots out of zones guarded by the opposing faction
    {
        bool botIsAlliance = IsAlliance(bot->getRace());
        if (botIsAlliance && m_hordeGuardAreas.find(area->ID) != m_hordeGuardAreas.end())
            return false;
        if (!botIsAlliance && m_allianceGuardAreas.find(area->ID) != m_allianceGuardAreas.end())
            return false;
    }

    // Reject zones whose creature level band does not fit the bot; when a zone has
    // no meaningful stats we accept it rather than reject-and-retry forever.
    std::map<uint32, AreaCreatureStats>::const_iterator statsItr = m_areaCreatureStatsMap.find(area->ID);
    if (statsItr != m_areaCreatureStatsMap.end() && statsItr->second.creatureCount > 0)
    {
        AreaCreatureStats const& stats = statsItr->second;
        uint8 botLevel = useLevel ? useLevel : bot->getLevel();
        uint8 tolerance = sPlayerbotAIConfig.randomBotTeleLevel;
        if (botLevel < stats.minLevel - tolerance || botLevel > stats.maxLevel + tolerance)
            return false;
    }
    return true;
}

uint32 RandomPlayerbotMgr::GetEventValue(uint32 bot, string event)
{
    uint32 value = 0;

    // Query the database to get the event value for the specified bot
    QueryResult* results = CharacterDatabase.PQuery(
            "SELECT `value`, `time`, `validIn` FROM `ai_playerbot_random_bots` WHERE `owner` = 0 AND `bot` = '%u' AND `event` = '%s'",
            bot, event.c_str());

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 validIn = fields[2].GetUInt32();
        if ((time(0) - lastChangeTime) >= validIn)
        {
            value = 0;
        }
        delete results;
    }

    return value;
}

uint32 RandomPlayerbotMgr::SetEventValue(uint32 bot, string event, uint32 value, uint32 validIn)
{
    // Delete the existing event value for the specified bot
    CharacterDatabase.PExecute("DELETE FROM `ai_playerbot_random_bots` WHERE `owner` = 0 and `bot` = '%u' and `event` = '%s'",
            bot, event.c_str());
    if (value)
    {
        // Insert the new event value for the specified bot
        CharacterDatabase.PExecute(
                "INSERT INTO `ai_playerbot_random_bots` (`owner`, `bot`, `time`, `validIn`, `event`, `value`) VALUES ('%u', '%u', '%u', '%u', '%s', '%u')",
                0, bot, (uint32)time(0), validIn, event.c_str(), value);
    }

    if (event == "add")
    {
        m_randomBotCache[bot] = (value != 0);
    }

    return value;
}

void RandomPlayerbotMgr::CalculateAreaCreatureStats()
{
    sLog.outString(">> [Playerbots] Calculating area creature statistics...");

    std::map<std::pair<uint32, uint32>, uint32> cellToAreaCache; // (mapId, cellId) -> areaId
    std::map<uint32, std::vector<uint8>> areaLevels;

    m_allianceGuardAreas.clear();
    m_hordeGuardAreas.clear();

    uint32 getAreaIdCalls = 0;
    uint32 totalCreatures = 0;

    CreatureDataMap const* creatureDataMap = sObjectMgr.GetCreatureDataMap();
    for (CreatureDataMap::const_iterator itr = creatureDataMap->begin(); itr != creatureDataMap->end(); ++itr)
    {
        CreatureData const& data = itr->second;
        CreatureInfo const* cInfo = sObjectMgr.GetCreatureTemplate(data.id);

        if (!cInfo)
        {
            continue;
        }

        totalCreatures++;

        CellPair cell_pair = MaNGOS::ComputeCellPair(data.posX, data.posY);
        uint32 cell_id = (cell_pair.y_coord * TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;
        std::pair<uint32, uint32> mapCell = std::make_pair(data.mapid, cell_id);

        uint32 areaId = 0;

        std::map<std::pair<uint32, uint32>, uint32>::iterator cacheItr = cellToAreaCache.find(mapCell);
        if (cacheItr != cellToAreaCache.end())
        {
            areaId = cacheItr->second;
        }
        else
        {
            Map* map = const_cast<Map*>(sMapMgr.FindMap(data.mapid));
            if (!map || !map->GetTerrain())
            {
                continue;
            }

            areaId = map->GetTerrain()->GetAreaId(data.posX, data.posY, data.posZ);
            cellToAreaCache[mapCell] = areaId; // Cache for future lookups
            getAreaIdCalls++;
        }

        if (areaId == 0)
        {
            continue;
        }

        // Classify contested-zone guards by faction hostility so bots avoid
        // areas patrolled by guards of the opposing faction.
        if (cInfo->ExtraFlags & CREATURE_FLAG_EXTRA_GUARD)
        {
            FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->FactionAlliance);
            if (factionTemplate && !factionTemplate->IsContestedGuardFaction() &&
                !(factionTemplate->hostileMask & FACTION_MASK_PLAYER))
            {
                if (factionTemplate->hostileMask & FACTION_MASK_HORDE)
                {
                    m_allianceGuardAreas.insert(areaId);
                }
                if (factionTemplate->hostileMask & FACTION_MASK_ALLIANCE)
                {
                    m_hordeGuardAreas.insert(areaId);
                }
            }
            continue;
        }

        // Skip questgivers, vendors, and non-attackable creatures for level stats.
        if (cInfo->NpcFlags != 0 || cInfo->UnitFlags & UNIT_FLAG_NON_ATTACKABLE)
        {
            continue;
        }

        uint8 avgLevel = (cInfo->MinLevel + cInfo->MaxLevel) / 2;
        areaLevels[areaId].push_back(avgLevel);
    }

    uint32 statsCount = 0;
    for (std::map<uint32, std::vector<uint8>>::iterator itr = areaLevels.begin(); itr != areaLevels.end(); ++itr)
    {
        std::vector<uint8>& levels = itr->second;
        if (levels.size() < 10) // need at least 10 creatures to have meaningful statistics
        {
            continue;
        }

        std::sort(levels.begin(), levels.end());

        // to avoid outliers, use 25th and 75th percentiles
        size_t p25 = levels.size() / 4;
        size_t p75 = (levels.size() * 3) / 4;

        AreaCreatureStats& stats = m_areaCreatureStatsMap[itr->first];
        stats.minLevel = levels[p25];
        stats.maxLevel = levels[p75];
        stats.creatureCount = levels.size();
        ++statsCount;
    }

    m_areaCreatureStatsComputed = true;
    sLog.outString(">> [Playerbots] Calculated spawn stats for %u areas (%u creatures, %u area lookups)",
        statsCount, totalCreatures, getAreaIdCalls);
}

bool RandomPlayerbotMgr::HandlePlayerbotConsoleCommand(ChatHandler* handler, char const* args)
{
    if (!sPlayerbotAIConfig.enabled)
    {
        sLog.outError("Playerbot system is currently disabled!");
        return false;
    }

    if (!args || !*args)
    {
        sLog.outError("Usage: rndbot stats/update/reset/init/refresh/add/remove");
        return false;
    }

    string cmd = args;

    if (cmd == "reset")
    {
        // Reset all random bots
        CharacterDatabase.PExecute("DELETE FROM `ai_playerbot_random_bots`");
        sLog.outString("Random bots were reset for all players. Please restart the Server.");
        return true;
    }
    else if (cmd == "stats")
    {
        // Print statistics of random bots
        sRandomPlayerbotMgr.PrintStats();
        return true;
    }
    else if (cmd == "update")
    {
        // Update the AI of random bots
        sRandomPlayerbotMgr.UpdateAIInternal(0);
        return true;
    }
    else if (cmd == "init" || cmd == "refresh" || cmd == "teleport" || cmd == "revive")
    {
        sLog.outString("Randomizing bots for %d accounts", sPlayerbotAIConfig.randomBotAccounts.size());
        list<uint32> botIds;
        for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); ++i)
        {
            uint32 account = *i;
            if (QueryResult *results = CharacterDatabase.PQuery("SELECT `guid` FROM `characters` where `account` = '%u'", account))
            {
                do
                {
                    Field* fields = results->Fetch();

                    uint32 botId = fields[0].GetUInt32();
                    ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, botId);
                    Player* bot = sObjectMgr.GetPlayer(guid);
                    if (!bot)
                    {
                        continue;
                    }

                    botIds.push_back(botId);
                } while (results->NextRow());
                delete results;
            }
        }

        int processed = 0;
        for (list<uint32>::iterator i = botIds.begin(); i != botIds.end(); ++i)
        {
            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, *i);
            Player* bot = sObjectMgr.GetPlayer(guid);
            if (!bot)
            {
                continue;
            }

            sLog.outString("[%u/%u] Processing command '%s' for bot '%s'",
                processed++, botIds.size(), cmd.c_str(), bot->GetName());

            if (cmd == "init")
            {
                sRandomPlayerbotMgr.RandomizeFirst(bot);
            }
            else if (cmd == "teleport")
            {
                sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
            }
            else if (cmd == "revive")
            {
                sRandomPlayerbotMgr.Revive(bot);
            }
            else
            {
                bot->SetLevel(bot->getLevel() - 1);
                sRandomPlayerbotMgr.IncreaseLevel(bot);
            }
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
            CharacterDatabase.PExecute("UPDATE `ai_playerbot_random_bots` SET `validIn` = '%u' WHERE `event` = 'randomize' AND `bot` = '%u'",
                randomTime, bot->GetGUIDLow());
            CharacterDatabase.PExecute("UPDATE `ai_playerbot_random_bots` SET `validIn` = '%u' WHERE `event` = 'logout' AND `bot` = '%u'",
                sPlayerbotAIConfig.maxRandomBotInWorldTime, bot->GetGUIDLow());
        }
        return true;
    }
    else
    {
        // Handle other playerbot commands
        list<string> messages = sRandomPlayerbotMgr.HandlePlayerbotCommand(args, NULL);
        for (list<string>::iterator i = messages.begin(); i != messages.end(); ++i)
        {
            sLog.outString(i->c_str());
        }
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::HandleCommand(uint32 type, const string& text, Player& fromPlayer)
{
    // Handle commands for all player bots
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        bot->GetPlayerbotAI()->HandleCommand(type, text, fromPlayer);
    }
}

void RandomPlayerbotMgr::OnPlayerLogout(Player* player)
{
    // Handle player logout for all player bots
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (player == ai->GetMaster())
        {
            ai->SetMaster(NULL);
            ai->ResetStrategies();
        }
    }

    if (!player->GetPlayerbotAI())
    {
        vector<Player*>::iterator i = find(players.begin(), players.end(), player);
        if (i != players.end())
        {
            players.erase(i);
        }
    }
}

void RandomPlayerbotMgr::OnPlayerLogin(Player* player)
{
    // Handle player login for all player bots
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (player == bot || player->GetPlayerbotAI())
        {
            continue;
        }

        Group* group = bot->GetGroup();
        if (!group)
        {
            continue;
        }

        for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->getSource();
            PlayerbotAI* ai = bot->GetPlayerbotAI();
            if (member == player && (!ai->GetMaster() || ai->GetMaster()->GetPlayerbotAI()))
            {
                ai->SetMaster(player);
                ai->ResetStrategies();
                ai->TellMaster("Hello");
                break;
            }
        }
    }

    if (!IsRandomBot(player))
    {
        players.push_back(player);
        sLog.outDebug("Including non-random bot player %s into random bot update", player->GetName());
    }
}

Player* RandomPlayerbotMgr::GetRandomPlayer()
{
    // Get a random player from the list of players
    if (players.empty())
    {
        return NULL;
    }

    uint32 index = urand(0, players.size() - 1);
    return players[index];
}

void RandomPlayerbotMgr::PrintStats()
{
    sLog.outString("%d Random Bots online", playerBots.size());

    map<uint32, int> alliance, horde;
    for (uint32 i = 0; i < 10; ++i)
    {
        alliance[i] = 0;
        horde[i] = 0;
    }

    map<uint8, int> perRace, perClass;
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        perRace[race] = 0;
    }
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        perClass[cls] = 0;
    }

    int dps = 0, heal = 0, tank = 0, active = 0;
    for (PlayerBotMap::iterator i = playerBots.begin(); i != playerBots.end(); ++i)
    {
        Player* bot = i->second;
        if (IsAlliance(bot->getRace()))
        {
            alliance[bot->getLevel() / 10]++;
        }
        else
        {
            horde[bot->getLevel() / 10]++;
        }

        perRace[bot->getRace()]++;
        perClass[bot->getClass()]++;

        if (bot->GetPlayerbotAI()->IsActive())
        {
            active++;
        }

        int spec = AiFactory::GetPlayerSpecTab(bot);
        switch (bot->getClass())
        {
        case CLASS_DRUID:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PALADIN:
            if (spec == 1)
            {
                tank++;
            }
            else if (spec == 0)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_PRIEST:
            if (spec != 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
            {
                heal++;
            }
            else
            {
                dps++;
            }
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
            {
                tank++;
            }
            else
            {
                dps++;
            }
            break;
        default:
            dps++;
            break;
        }
    }

    sLog.outString("Per level:");
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    for (uint32 i = 0; i < 10; ++i)
    {
        if (!alliance[i] && !horde[i])
        {
            continue;
        }

        uint32 from = i * 10;
        uint32 to = min(from + 9, maxLevel);
        if (!from)
        {
            from = 1;
        }
        sLog.outString("    %d..%d: %d alliance, %d horde", from, to, alliance[i], horde[i]);
    }
    sLog.outString("Per race:");
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        if (perRace[race])
        {
            sLog.outString("    %s: %d", ChatHelper::formatRace(race).c_str(), perRace[race]);
        }
    }
    sLog.outString("Per class:");
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (perClass[cls])
        {
            sLog.outString("    %s: %d", ChatHelper::formatClass(cls).c_str(), perClass[cls]);
        }
    }
    sLog.outString("Per role:");
    sLog.outString("    tank: %d", tank);
    sLog.outString("    heal: %d", heal);
    sLog.outString("    dps: %d", dps);

    sLog.outString("Active bots: %d", active);
}

double RandomPlayerbotMgr::GetBuyMultiplier(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    uint32 value = GetEventValue(id, "buymultiplier");
    if (!value)
    {
        value = urand(1, 120);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "buymultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

double RandomPlayerbotMgr::GetSellMultiplier(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    uint32 value = GetEventValue(id, "sellmultiplier");
    if (!value)
    {
        value = urand(80, 250);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "sellmultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

uint32 RandomPlayerbotMgr::GetLootAmount(Player* bot)
{
    uint32 id = bot->GetGUIDLow();
    return GetEventValue(id, "lootamount");
}

void RandomPlayerbotMgr::SetLootAmount(Player* bot, uint32 value)
{
    uint32 id = bot->GetGUIDLow();
    SetEventValue(id, "lootamount", value, 24 * 3600);
}

uint32 RandomPlayerbotMgr::GetTradeDiscount(Player* bot)
{
    Group* group = bot->GetGroup();
    return GetLootAmount(bot) / (group ? group->GetMembersCount() : 10);
}

string RandomPlayerbotMgr::HandleRemoteCommand(string request)
{
    string::iterator pos = find(request.begin(), request.end(), ',');
    if (pos == request.end())
    {
        ostringstream out; out << "invalid request: " << request;
        return out.str();
    }

    string command = string(request.begin(), pos);
    uint64 guid = atoi(string(pos + 1, request.end()).c_str());
    Player* bot = GetPlayerBot(guid);
    if (!bot)
    {
        return "invalid guid";
    }

    PlayerbotAI *ai = bot->GetPlayerbotAI();
    if (!ai)
    {
        return "invalid guid";
    }

    return ai->HandleRemoteCommand(command);
}
