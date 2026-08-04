#ifndef _RandomPlayerbotMgr_H
#define _RandomPlayerbotMgr_H

#include "Common.h"
#include "PlayerbotAIBase.h"
#include "PlayerbotMgr.h"
#include <unordered_map>

class WorldPacket;
class Player;
class Unit;
class Object;
class Item;
class QueryResult;

using namespace std;
/**
* \struct AreaCreatureStats
* \brief Entry representing creature levels within an area for playerbot spawning decisions
*/
struct AreaCreatureStats
{
    uint8   minLevel;
    uint8   maxLevel;
    uint16  creatureCount;

    AreaCreatureStats() : minLevel(0), maxLevel(0), creatureCount(0) {}
};

class RandomPlayerbotMgr : public PlayerbotHolder
{
    public:
        /**
         * @brief Constructor for RandomPlayerbotMgr.
         * Initializes the random player bot manager.
         */
        RandomPlayerbotMgr();

        /**
         * @brief Destructor for RandomPlayerbotMgr.
         */
        virtual ~RandomPlayerbotMgr();

        /**
         * @brief Gets the singleton instance of RandomPlayerbotMgr.
         * @return Reference to the singleton instance of RandomPlayerbotMgr.
         */
        static RandomPlayerbotMgr& instance()
        {
            static RandomPlayerbotMgr instance;
            return instance;
        }

        /**
         * @brief Updates the AI internal state.
         * @param elapsed Time elapsed since the last update.
         */
        virtual void UpdateAIInternal(uint32 elapsed);

        /**
         * @brief Handles player bot console commands.
         * @param handler Pointer to the chat handler.
         * @param args The command arguments.
         * @return True if the command is handled successfully, false otherwise.
         */
        static bool HandlePlayerbotConsoleCommand(ChatHandler* handler, char const* args);

        /**
         * @brief Checks if a given player is a random bot.
         * @param bot Pointer to the player.
         * @return True if the player is a random bot, false otherwise.
         */
        bool IsRandomBot(Player* bot);

        /**
         * @brief Checks if a given player ID is a random bot.
         * @param bot The player ID.
         * @return True if the player ID is a random bot, false otherwise.
         */
        bool IsRandomBot(uint32 bot);

        /**
         * @brief Randomizes the given player bot.
         * @param bot Pointer to the player bot.
         */
        void Randomize(Player* bot);

        /**
         * @brief Randomizes the given player bot for the first time.
         * @param bot Pointer to the player bot.
         */
        void RandomizeFirst(Player* bot);

        /**
         * @brief Increases the level of the given player bot.
         * @param bot Pointer to the player bot.
         */
        void IncreaseLevel(Player* bot);

        /**
         * @brief Schedules a teleport for the given player bot.
         * @param bot The player ID.
         */
        void ScheduleTeleport(uint32 bot, uint32 time = 0);

        /**
         * @brief Handles a command from a player.
         * @param type The type of the command.
         * @param text The text of the command.
         * @param fromPlayer The player who sent the command.
         */
        void HandleCommand(uint32 type, const string& text, Player& fromPlayer);

        /**
         * @brief Handles a remote command.
         * @param request The command request.
         * @return The result of the command.
         */
        string HandleRemoteCommand(string request);

        /**
         * @brief Handles player logout.
         * @param player Pointer to the player.
         */
        void OnPlayerLogout(Player* player);

        /**
         * @brief Handles player login.
         * @param player Pointer to the player.
         */
        void OnPlayerLogin(Player* player);

        /**
         * @brief Updates zone-population bookkeeping when a player changes zone.
         * @param player Pointer to the player.
         * @param newZone The zone the player is entering.
         */
        void OnPlayerZoneChange(Player* player, uint32 newZone);

        /// True if at least one real (non-bot) player is currently in the given zone.
        bool HasRealPlayerInZone(uint32 zoneId) const;

        /**
         * @brief Gets a random player.
         * @return Pointer to the random player.
         */
        Player* GetRandomPlayer();

        /**
         * @brief Prints statistics about the random player bots.
         */
        void PrintStats();

        /**
         * @brief Gets the buy multiplier for the given player bot.
         * @param bot Pointer to the player bot.
         * @return The buy multiplier.
         */
        double GetBuyMultiplier(Player* bot);

        /**
         * @brief Gets the sell multiplier for the given player bot.
         * @param bot Pointer to the player bot.
         * @return The sell multiplier.
         */
        double GetSellMultiplier(Player* bot);

        /**
         * @brief Gets the loot amount for the given player bot.
         * @param bot Pointer to the player bot.
         * @return The loot amount.
         */
        uint32 GetLootAmount(Player* bot);

        /**
         * @brief Sets the loot amount for the given player bot.
         * @param bot Pointer to the player bot.
         * @param value The loot amount.
         */
        void SetLootAmount(Player* bot, uint32 value);

        /**
         * @brief Gets the trade discount for the given player bot.
         * @param bot Pointer to the player bot.
         * @return The trade discount.
         */
        uint32 GetTradeDiscount(Player* bot);

        /**
         * @brief Refreshes the given player bot.
         * @param bot Pointer to the player bot.
         */
        void Refresh(Player* bot);

        /**
         * @brief Teleports the given player bot to a random location based on their level.
         * @param bot Pointer to the player bot.
         */
        void RandomTeleportForLevel(Player* bot);

        /// Queues every bot in a persisted group for login (used when AiPlayerbot.RandomBotKeepGroups is set).
        void EnsureGroupedBotsOnline();
        /// Refreshes m_groupedBots from the character DB.
        void LoadGroupedBots();
        /// Underlying query shared by EnsureGroupedBotsOnline/LoadGroupedBots.
        QueryResult* QueryGroupedBots();

        /**
         * @brief Teleports the given player bot to a random location.
         * @param bot Pointer to the player bot.
         * @param mapId The map ID.
         * @param teleX The X coordinate.
         * @param teleY The Y coordinate.
         * @param teleZ The Z coordinate.
         */
        void RandomTeleport(Player * bot, uint32 mapId, float teleX, float teleY, float teleZ);

        /**
         * @brief Gets the maximum allowed bot count.
         * @return The maximum allowed bot count.
         */
        int GetMaxAllowedBotCount();

        /**
         * @brief Processes the given player bot.
         * @param player Pointer to the player bot.
         * @return True if the bot is processed successfully, false otherwise.
         */
        bool ProcessBot(Player* player);

        /**
         * @brief Revives the given player bot.
         * @param player Pointer to the player bot.
         */
        void Revive(Player* player);

    protected:
        /**
         * @brief Internal handler for bot login.
         * @param bot Pointer to the player bot.
         */
        virtual void OnBotLoginInternal(Player * const bot) {}

    private:
        /**
         * @brief Gets the event value for a given player bot and event.
         * @param bot The player ID.
         * @param event The event name.
         * @return The event value.
         */
        uint32 GetEventValue(uint32 bot, string event);

        /**
         * @brief Sets the event value for a given player bot and event.
         * @param bot The player ID.
         * @param event The event name.
         * @param value The event value.
         * @param validIn The validity duration of the event.
         * @return The event value.
         */
        uint32 SetEventValue(uint32 bot, string event, uint32 value, uint32 validIn);

        /**
         * @brief Gets the list of random player bots.
         * @return The list of random player bots.
         */
        list<uint32> GetBots();

        /**
         * @brief Adds random player bots.
         * @return The number of random player bots added.
         */
        uint32 AddRandomBots();

        /**
         * @brief Processes the given player bot.
         * @param bot The player ID.
         * @return True if the bot is processed successfully, false otherwise.
         */
        bool ProcessBot(uint32 bot);

        /**
         * @brief Schedules randomization for the given player bot.
         * @param bot The player ID.
         * @param time The time to schedule the randomization.
         */
        void ScheduleRandomize(uint32 bot, uint32 time);

        /**
         * @brief Teleports the given player bot to a random location.
         * @param bot Pointer to the player bot.
         * @param locs The list of possible locations.
         */
        void RandomTeleport(Player* bot, vector<WorldLocation> &locs);

        /**
         * @brief Gets the level of a zone based on its coordinates.
         * @param mapId The map ID.
         * @param teleX The X coordinate.
         * @param teleY The Y coordinate.
         * @param teleZ The Z coordinate.
         * @return The level of the zone.
         */
        uint32 GetZoneLevel(uint16 mapId, float teleX, float teleY, float teleZ);
        bool IsZoneSafeForBot(Player* bot, uint32 mapId, float x, float y, float z, uint32 useLevel = 0);
        void CalculateAreaCreatureStats();

        /// Rejects void/off-mesh/deep-water destinations; snaps z to ground on success
        bool IsSafeTeleportPosition(uint32 mapId, float x, float y, float& z);

    private:
        vector<Player*> players; ///< List of players.
        int processTicks; ///< Number of process ticks.
        uint32 m_processBotCursor; ///< GUID of the last bot examined; the next pass resumes after it so a budget/cap-limited pass eventually covers every bot.
        std::map<uint32, AreaCreatureStats> m_areaCreatureStatsMap;
        std::map<std::pair<uint32, uint32>, uint32> m_cellToAreaCache;
        bool m_areaCreatureStatsComputed = false; ///< Guards the one-time area-stats scan so an empty result is not recomputed every call.
        std::unordered_map<uint32, bool> m_randomBotCache; ///< Caches IsRandomBot("add") lookups to avoid a DB query per call; kept coherent in SetEventValue.
        std::unordered_map<uint32, uint32> m_playerZoneCounts; ///< zone_id -> real player count, for O(1) bot tick gating.
        std::set<uint32> m_groupedBots; ///< Cached set of bot GUIDs currently in a group, refreshed each update cycle.

        /// Cached mirror of one `ai_playerbot_random_bots` row.
        struct EventValueEntry
        {
            uint32 value;
            uint32 lastChangeTime;
            uint32 validIn;
        };
        std::map<std::pair<uint32, std::string>, EventValueEntry> m_eventValueCache; ///< (bot, event) -> cached value, avoids a DB query per GetEventValue call.
        std::set<uint32> m_allianceGuardAreas; ///< Contested areas whose guards are hostile to Horde; Horde bots are kept out.
        std::set<uint32> m_hordeGuardAreas;    ///< Contested areas whose guards are hostile to Alliance; Alliance bots are kept out.
};

#define sRandomPlayerbotMgr RandomPlayerbotMgr::instance()

#endif
