#pragma once

#include "Config.h"

class Player;
class PlayerbotMgr;
class ChatHandler;

/**
 * @brief Configuration class for Playerbot AI.
 * This class holds all the configuration parameters for the Playerbot AI and provides methods to initialize and access these parameters.
 */
class PlayerbotAIConfig
{
public:
    /**
     * @brief Constructor for PlayerbotAIConfig.
     * Initializes all configuration parameters with default values.
     */
    PlayerbotAIConfig();

    /**
     * @brief Gets the singleton instance of PlayerbotAIConfig.
     * @return Reference to the singleton instance of PlayerbotAIConfig.
     */
    static PlayerbotAIConfig& instance()
    {
        static PlayerbotAIConfig instance;
        return instance;
    }

public:
    /**
     * @brief Initializes the Playerbot AI configuration by reading from the configuration file.
     * @return True if initialization is successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Checks if a given account ID is in the random bot account list.
     * @param id The account ID to check.
     * @return True if the account ID is in the list, false otherwise.
     */
    bool IsInRandomAccountList(uint32 id);

    /**
     * @brief Checks if a given item ID is in the random bot quest item list.
     * @param id The item ID to check.
     * @return True if the item ID is in the list, false otherwise.
     */
    bool IsInRandomQuestItemList(uint32 id);

    bool enabled; ///< Indicates if the Playerbot AI is enabled.
    bool allowGuildBots; ///< Indicates if guild bots are allowed.
    uint32 globalCoolDown, reactDelay, maxWaitForMove, expireActionTime, dispelAuraDuration, passiveDelay, repeatDelay;
    float sightDistance, spellDistance, reactDistance, grindDistance, lootDistance, shootDistance,
        fleeDistance, tooCloseDistance, meleeDistance, followDistance, whisperDistance, contactDistance,
        aoeRadius;
    bool whisperToZoneOnly; ///< Restricts suggestion whispers to players in the bot's zone.
    uint32 criticalHealth, lowHealth, mediumHealth, almostFullHealth, hungryHealth;
    uint32 lowMana, mediumMana, thirstyMana;

    uint32 openGoSpell; ///< The spell ID for opening game objects.
    bool randomBotAutologin; ///< Indicates if random bots should auto-login.
    bool botAutologin; ///< Indicates if bots should auto-login.
    std::string randomBotMapsAsString; ///< Comma-separated string of random bot maps.
    std::vector<uint32> randomBotMaps; ///< List of random bot maps.
    std::list<uint32> randomBotQuestItems; ///< List of random bot quest items.
    std::list<uint32> randomBotAccounts; ///< List of random bot accounts.
    std::list<uint32> randomBotSpellIds; ///< List of random bot spell IDs.
    uint32 randomBotTeleportDistance; ///< The teleport distance for random bots.
    float randomGearLoweringChance; ///< The chance of lowering gear for random bots.
    float randomBotMaxLevelChance; ///< The chance of random bots reaching max level.
    uint32 minRandomBots, maxRandomBots;
    uint32 randomBotUpdateInterval, randomBotCountChangeMinInterval, randomBotCountChangeMaxInterval;
    uint32 minRandomBotInWorldTime, maxRandomBotInWorldTime;
    uint32 minRandomBotRandomizeTime, maxRandomBotRandomizeTime;
    uint32 minRandomBotReviveTime, maxRandomBotReviveTime;
    uint32 minRandomBotPvpTime, maxRandomBotPvpTime;
    uint32 minRandomBotsPerInterval, maxRandomBotsPerInterval;
    uint32 randomBotProcessBudgetMs; ///< Wall-clock budget (ms) for one random-bot processing pass; 0 disables pacing.
    uint32 randomBotCatchupInterval; ///< Seconds until the next pass when a pass stops on its time budget.
    uint32 minRandomBotsPriceChangeInterval, maxRandomBotsPriceChangeInterval;
    bool randomBotJoinLfg; ///< Indicates if random bots should join Looking For Group.
    bool randomBotLoginAtStartup; ///< Indicates if random bots should login at startup.
    bool randomBotActiveZoneOnly; ///< If true, ungrouped random bots only tick when a real player is in their zone.
    uint32 randomBotTeleLevel; ///< The teleport level for random bots.
    bool logInGroupOnly, logValuesPerTick;
    bool fleeingEnabled; ///< Indicates if fleeing is enabled for bots.
    bool cautiousDefault; ///< If true, all bots get the "cautious" strategy (avoid pulling aggro while moving) by default.
    bool spawnZoneStats;  ///< If true, scan all creatures once to level-band bot spawn zones. Expensive (multi-second world-thread scan); off by default.
    std::string combatStrategies, nonCombatStrategies;
    std::string randomBotCombatStrategies, randomBotNonCombatStrategies;
    uint32 randomBotMinLevel, randomBotMaxLevel;
    float randomChangeMultiplier;
    uint32 specProbability[MAX_CLASSES][3]; ///< Probability of class specs for random bots.
    std::string commandPrefix; ///< Prefix for bot commands.
    std::string randomBotAccountPrefix; ///< Prefix for random bot accounts.
    uint32 randomBotAccountCount; ///< Number of random bot accounts.
    bool deleteRandomBotAccounts; ///< Indicates if random bot accounts should be deleted.
    uint32 randomBotGuildCount; ///< Number of random bot guilds.
    bool deleteRandomBotGuilds; ///< Indicates if random bot guilds should be deleted.
    std::list<uint32> randomBotGuilds; ///< List of random bot guilds.
    bool randomBotShowHelmet; ///< Indicates if random bots should show helmets.
    bool randomBotShowCloak; ///< Indicates if random bots should show cloaks.
    bool enableGreet; ///< Indicates if greeting is enabled for bots.
    bool autoStockFood; ///< Indicates if bots with empty bags should be auto-stocked with food/drink on login.

    bool useCuratedGear; ///< Phase A: equip class/spec/tier curated best-in-slot gear (ai_playerbot_gear) instead of gear-score guessing.
    std::string gearTier; ///< Force a curated gear tier ("normal"/"heroic"/"raider"); empty = balanced split by bot GUID.
    bool useCuratedGearEnhancements; ///< Phase B/C/D: apply curated enchants/gems/glyphs on top of curated gear (ai_playerbot_gear_enchant/_gem/_glyph). Requires useCuratedGear.
    uint32 feralBearTankChance; ///< Percent (0-100) of Feral druid bots routed to the bear tank build (AiFactory::IsFeralBearTank); the rest are cat DPS.

    bool guildTaskEnabled; ///< Indicates if guild tasks are enabled.
    uint32 minGuildTaskChangeTime, maxGuildTaskChangeTime;
    uint32 minGuildTaskAdvertisementTime, maxGuildTaskAdvertisementTime;
    uint32 minGuildTaskRewardTime, maxGuildTaskRewardTime;
    uint32 guildTaskAdvertCleanupTime;

    uint32 iterationsPerTick; ///< Number of iterations per tick.

    int commandServerPort; ///< Port for the command server.

    /**
     * @brief Gets the value of a configuration parameter by name.
     * @param name The name of the configuration parameter.
     * @return The value of the configuration parameter as a string.
     */
    std::string GetValue(std::string name) const;

    /**
     * @brief Sets the value of a configuration parameter by name.
     * @param name The name of the configuration parameter.
     * @param value The value to set the configuration parameter to.
     */
    void SetValue(std::string &name, std::string value);

private:
    Config config; ///< Configuration object for reading from the configuration file.
};

#define sPlayerbotAIConfig MaNGOS::Singleton<PlayerbotAIConfig>::Instance()
