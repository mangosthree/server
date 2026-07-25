/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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

#include "Master.h"

#include "AntiFreezeService.h"
#include "CliService.h"
#include "RASession.h"

#include "Config/Config.h"
#include "DBCStores.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "MapManager.h"
#include "MassMailMgr.h"
#include "Server/WorldNetwork.h"
#include "Timer.h"
#include "World.h"

#ifdef ENABLE_SOAP
#include "SOAP/SoapService.h"
#endif

#ifdef _WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#else
#include "PosixDaemon.h"
#endif

#include <chrono>
#include <string>
#include <thread>
#include <vector>

/// Shortest interval between two world ticks, in milliseconds.
#ifndef WORLD_SLEEP_CONST
#define WORLD_SLEEP_CONST 50
#endif

extern uint32 realmID;

Master::Master()
{
}

Master::~Master()
{
}

bool Master::StartDatabases()
{
    ///- Get world database info from configuration file
    std::string dbstring = sConfig.GetStringDefault("WorldDatabaseInfo", "");
    int nConnections = sConfig.GetIntDefault("WorldDatabaseConnections", 1);
    if (dbstring.empty())
    {
        sLog.outError("Database not specified in configuration file");
        return false;
    }
    sLog.outString("World Database total connections: %i", nConnections + 1);

    ///- Initialise the world database
    if (!WorldDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Can not connect to world database %s", dbstring.c_str());
        return false;
    }

    ///- Check the World database version
    if (!WorldDatabase.CheckDatabaseVersion(DATABASE_WORLD))
    {
        WorldDatabase.HaltDelayThread();
        return false;
    }

    dbstring = sConfig.GetStringDefault("CharacterDatabaseInfo", "");
    nConnections = sConfig.GetIntDefault("CharacterDatabaseConnections", 1);
    if (dbstring.empty())
    {
        sLog.outError("Character Database not specified in configuration file");
        WorldDatabase.HaltDelayThread();
        return false;
    }
    sLog.outString("Character Database total connections: %i", nConnections + 1);

    ///- Initialise the Character database
    if (!CharacterDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Can not connect to Character database %s", dbstring.c_str());
        WorldDatabase.HaltDelayThread();
        return false;
    }

    ///- Check the Character database version
    if (!CharacterDatabase.CheckDatabaseVersion(DATABASE_CHARACTER))
    {
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    ///- Get login database info from configuration file
    dbstring = sConfig.GetStringDefault("LoginDatabaseInfo", "");
    nConnections = sConfig.GetIntDefault("LoginDatabaseConnections", 1);
    if (dbstring.empty())
    {
        sLog.outError("Login database not specified in configuration file");
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    ///- Initialise the login database
    sLog.outString("Login Database total connections: %i", nConnections + 1);
    if (!LoginDatabase.Initialize(dbstring.c_str(), nConnections))
    {
        sLog.outError("Can not connect to login database %s", dbstring.c_str());
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        return false;
    }

    ///- Check the Realm database version
    if (!LoginDatabase.CheckDatabaseVersion(DATABASE_REALMD))
    {
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        LoginDatabase.HaltDelayThread();
        return false;
    }

    sLog.outString();

    ///- Get the realm Id from the configuration file
    realmID = sConfig.GetIntDefault("RealmID", 0);
    if (!realmID)
    {
        sLog.outError("Realm ID not defined in configuration file");
        WorldDatabase.HaltDelayThread();
        CharacterDatabase.HaltDelayThread();
        LoginDatabase.HaltDelayThread();
        return false;
    }

    sLog.outString("Realm running as realm ID %d", realmID);
    sLog.outString();

    sWorld.LoadDBVersion();

    sLog.outString("Using World DB: %s", sWorld.GetDBVersion());
    sLog.outString();
    return true;
}

void Master::StopDatabases()
{
    CharacterDatabase.HaltDelayThread();
    WorldDatabase.HaltDelayThread();
    LoginDatabase.HaltDelayThread();
}

void Master::ClearOnlineAccounts()
{
    // Cleanup online status for characters hosted at current realm
    /// \todo Only accounts with characters logged on *this* realm should have online status reset. Move the online column from 'account' to 'realmcharacters'?
    LoginDatabase.PExecute("UPDATE `account` SET `active_realm_id` = 0, `os` = ''  WHERE `active_realm_id` = '%u'", realmID);

    CharacterDatabase.Execute("UPDATE `characters` SET `online` = 0 WHERE `online`<>0");

    // Battleground instance ids reset at server restart
    CharacterDatabase.Execute("UPDATE `character_battleground_data` SET `instance_id` = 0");
}

void Master::StartServices()
{
    // Remote administration, over the same networking engine the world uses.
    if (sConfig.GetBoolDefault("Ra.Enable", false))
    {
        m_services.push_back(std::unique_ptr<IService>(new RaService(
            uint16(sConfig.GetIntDefault("Ra.Port", 3443)),
            sConfig.GetStringDefault("Ra.IP", "0.0.0.0"))));
    }

#ifdef ENABLE_SOAP
    if (sConfig.GetBoolDefault("SOAP.Enabled", false))
    {
        m_services.push_back(std::unique_ptr<IService>(new SoapService(
            sConfig.GetStringDefault("SOAP.IP", "127.0.0.1"),
            uint16(sConfig.GetIntDefault("SOAP.Port", 7878)))));
    }
#else
    if (sConfig.GetBoolDefault("SOAP.Enabled", false))
    {
        sLog.outError("SOAP is enabled but wasn't included during compilation, not activating it.");
    }
#endif

    // Watchdog. Disabled unless MaxCoreStuckTime is set.
    m_services.push_back(std::unique_ptr<IService>(new AntiFreezeService(
        1000 * uint32(sConfig.GetIntDefault("MaxCoreStuckTime", 0)))));

    // Console last, so its prompt lands after every other start-up line.
#ifdef _WIN32
    const bool consoleWanted = sConfig.GetBoolDefault("Console.Enable", true)
                            && m_ServiceStatus == -1;   // no console in service mode
#else
    const bool consoleWanted = sConfig.GetBoolDefault("Console.Enable", true);
#endif
    if (consoleWanted)
    {
        m_services.push_back(std::unique_ptr<IService>(
            new CliService(sConfig.GetBoolDefault("BeepAtStart", true))));
    }

    for (auto& service : m_services)
    {
        service->Start();
    }
}

void Master::StopServices()
{
    // Ask everything to wind down first, then wait. Doing this in one pass per
    // service would serialise the timeouts: the total would be the sum of every
    // service's shutdown rather than the longest one.
    for (auto& service : m_services)
    {
        service->RequestStop();
    }

    for (auto itr = m_services.rbegin(); itr != m_services.rend(); ++itr)
    {
        sLog.outString("[shutdown] stopping %s", (*itr)->Name());
        (*itr)->Join();
    }

    m_services.clear();
}

void Master::WorldLoop()
{
    sLog.outString("World Updater Thread started (%dms min update interval)", WORLD_SLEEP_CONST);

    uint32 realPrevTime = getMSTime();

    while (!World::IsStopped())
    {
        ++World::m_worldLoopCounter;

        const uint32 realCurrTime = getMSTime();
        const uint32 diff = getMSTimeDiff(realPrevTime, realCurrTime);

        sWorld.Update(diff);
        realPrevTime = realCurrTime;

        const uint32 executionTimeDiff = getMSTimeDiff(realCurrTime, getMSTime());

        if (executionTimeDiff < WORLD_SLEEP_CONST)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(WORLD_SLEEP_CONST - executionTimeDiff));
        }

#ifdef _WIN32
        if (m_ServiceStatus == 0) // service stopped
        {
            World::StopNow(SHUTDOWN_EXIT_CODE);
        }

        while (m_ServiceStatus == 2) // service paused
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
#endif
    }

    sLog.outString("World Updater Thread stopped");
}

void Master::ShutdownWorld()
{
    // Strict order, and it matters: players must be saved before their sessions
    // are drained, sessions must be drained before the listener goes away, and
    // the listener must be gone before the maps they live on are unloaded.
    sLog.outString("[shutdown] world loop stopped; entering world-thread shutdown tail");

    sLog.outString("[shutdown] KickAll: saving + kicking players...");
    sWorld.KickAll();
    sLog.outString("[shutdown] KickAll done");

    sLog.outString("[shutdown] final UpdateSessions...");
    sWorld.UpdateSessions(1);
    sLog.outString("[shutdown] final UpdateSessions done");

    sLog.outString("[shutdown] StopNetwork: ending the world listener...");
    sWorldNetwork.Stop();
    sLog.outString("[shutdown] StopNetwork done");

    sLog.outString("[shutdown] UnloadAll: unloading maps + MapUpdater teardown...");
    sMapMgr.UnloadAll();
    sLog.outString("[shutdown] UnloadAll returned; world thread exiting");
}

int Master::Run()
{
    if (!StartDatabases())
    {
        return 1;
    }

    ClearOnlineAccounts();

    sWorld.SetInitialWorldSettings();

#ifndef _WIN32
    detachDaemon();
#endif

    // Publish this realm's flags and the client builds it accepts.
    const uint8 recommendedOrNew =
        sWorld.getConfig(CONFIG_BOOL_REALM_RECOMMENDED_OR_NEW)
            ? REALM_FLAG_NEW_PLAYERS : REALM_FLAG_RECOMMENDED;
    const uint8 realmStatus =
        sWorld.getConfig(CONFIG_BOOL_REALM_RECOMMENDED_OR_NEW_ENABLED)
            ? recommendedOrNew : uint8(REALM_FLAG_NONE);

    std::string builds = AcceptableClientBuildsListStr();
    LoginDatabase.escape_string(builds);
    LoginDatabase.DirectPExecute(
        "UPDATE `realmlist` SET `realmflags` = %u, `population` = 0, "
        "`realmbuilds` = '%s' WHERE `id` = '%u'",
        realmStatus, builds.c_str(), realmID);

    // Async transactions are forbidden during start-up; enable them only now
    // that the world is fully loaded.
    WorldDatabase.ThreadStart();
    CharacterDatabase.AllowAsyncTransactions();
    WorldDatabase.AllowAsyncTransactions();
    LoginDatabase.AllowAsyncTransactions();

    if (!sWorldNetwork.Start(uint16(sWorld.getConfig(CONFIG_UINT32_PORT_WORLD)),
                             sConfig.GetStringDefault("BindIP", "0.0.0.0")))
    {
        StopDatabases();
        return 1;
    }

    StartServices();

    WorldLoop();

    ShutdownWorld();
    StopServices();

    ClearOnlineAccounts();

    // send all still queued mass mails (before DB connections shutdown)
    sMassMailMgr.Update(true);

    sLog.outString("[shutdown] halting DB delay threads (Character/World/Login)...");
    StopDatabases();
    sLog.outString("[shutdown] DB delay threads halted");

    WorldDatabase.ThreadEnd();

    return World::GetExitCode();
}
