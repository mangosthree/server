/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "Common/Locales.h"
#include <cmath>
#include <utility>
#include <memory>
#include <mutex>
#include <algorithm>
#include "WorldGateway.h"

#include "DBCStores.h"
#include "Database/DatabaseEnv.h"
#include "Log/Log.h"
#include "OpcodeTable.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif

#include <string>

namespace
{
    /**
     * @brief The account row, read once during LookupAccount and reused by Attach.
     *
     * This is exactly the set of columns the old WorldSocket pulled out of the
     * login database and then had to carry, as loose locals, through two hundred
     * lines of interleaved policy (WorldSocket.cpp:1036-1250, now deleted).
     * Naming it makes the hand-off explicit and stops the row being fetched twice.
     */
    struct AccountRow : public proto::AuthContext
    {
        uint32         id        = 0;
        AccountTypes   security  = SEC_PLAYER;
        uint8          expansion = 0;
        time_t         muteTime  = 0;
        LocaleConstant locale    = LOCALE_enUS;
        std::string    os;
        BigNumber      sessionKey; ///< `sessionkey` column (K) -- arms proto's cipher and its SHA-1 proof.

        /// `s` column (SRP6 salt) -- WorldSocket kept this in a field it called
        /// m_s and exposed as GetSessionKey(), which SendRedirectClient() uses
        /// as an HMAC seed. That naming predates this refactor and is preserved
        /// here, not corrected: this is a byte-for-byte port of what the socket
        /// already sent, not a cryptography review.
        BigNumber      sessionSalt;
    };

    /**
     * @brief Register the calling thread with the database client library.
     *
     * The protocol layer calls LookupAccount()/Attach() on one of the network
     * engine's worker threads, and those threads query LoginDatabase. The MySQL
     * client keeps per-thread state that every such thread has to set up and
     * tear down, or it corrupts and leaks it -- a failure that shows up far from
     * its cause and only under load.
     *
     * The registration lives here rather than in the network engine on purpose:
     * net must not know a database exists. A function-local thread_local runs the
     * guard's constructor the first time a given worker thread reaches this code
     * and its destructor when that thread exits, so the transport stays oblivious
     * while every thread that touches the database is still accounted for.
     */
    void EnsureDbThreadRegistered()
    {
        static thread_local DbThreadGuard guard(&LoginDatabase);
        (void)guard;
    }
}

WorldGateway::WorldGateway()
    : m_nextId(1)
{
}

WorldGateway::~WorldGateway()
{
}

proto::AuthLookup WorldGateway::LookupAccount(const proto::AuthRequest& request)
{
    EnsureDbThreadRegistered();

    proto::AuthLookup result;

    // ---- Client build ------------------------------------------------------
    // WorldSocket.cpp:1036.
    if (!IsAcceptableClientBuild(request.build))
    {
        result.status = proto::AuthStatus::VersionMismatch;
        return result;
    }

    // ---- Account row --------------------------------------------------------
    // WorldSocket.cpp:1054-1069. `v` (SRP6 verifier) is dropped: the original
    // code fetched it only to print an unused debug line (N and g were computed
    // and never even printed) -- no login decision ever read it.
    std::string safeAccount = request.account;
    LoginDatabase.escape_string(safeAccount);

    QueryResult* queryResult =
        LoginDatabase.PQuery("SELECT "
                             "`id`, "          // 0
                             "`gmlevel`, "     // 1
                             "`sessionkey`, "  // 2
                             "`last_ip`, "     // 3
                             "`locked`, "      // 4
                             "`s`, "           // 5
                             "`expansion`, "   // 6
                             "`mutetime`, "    // 7
                             "`locale`, "      // 8
                             "`os` "           // 9
                             "FROM `account` WHERE `username` = '%s'",
                             safeAccount.c_str());

    if (!queryResult)
    {
        result.status = proto::AuthStatus::UnknownAccount;
        return result;
    }

    const Field* fields = queryResult->Fetch();

    std::shared_ptr<AccountRow> row = std::make_shared<AccountRow>();

    row->id = fields[0].GetUInt32();

    // Clamp rather than trust: a bad gmlevel in the database must not hand out
    // more authority than the server has levels for. WorldSocket.cpp:1125-1128.
    uint32 security = fields[1].GetUInt16();
    if (security > SEC_ADMINISTRATOR)
    {
        security = SEC_ADMINISTRATOR;
    }
    row->security = AccountTypes(security);

    row->sessionKey.SetHexStr(fields[2].GetString());

    const std::string lastIp = fields[3].GetString();
    const bool        locked = fields[4].GetUInt8() == 1;

    row->sessionSalt.SetHexStr(fields[5].GetString());

    row->expansion = uint8(std::min<uint32>(sWorld.getConfig(CONFIG_UINT32_EXPANSION),
                                            fields[6].GetUInt8()));
    row->muteTime  = time_t(fields[7].GetUInt64());

    const uint8 rawLocale = fields[8].GetUInt8();
    row->locale = rawLocale >= MAX_LOCALE ? LOCALE_enUS : LocaleConstant(rawLocale);

    row->os = fields[9].GetString();

    delete queryResult;

    // ---- IP lock -------------------------------------------------------------
    // WorldSocket.cpp:1107-1121 ("re-check ip locking, same check as in realmd").
    if (locked && lastIp != request.peerAddress)
    {
        BASIC_LOG("WorldGateway: account '%s' is IP locked to %s but connected from %s",
                  request.account.c_str(), lastIp.c_str(),
                  request.peerAddress.c_str());
        result.status = proto::AuthStatus::Failed;
        return result;
    }

    // ---- Bans ------------------------------------------------------------
    // WorldSocket.cpp:1145-1161 ("re-check account ban, same check as in realmd").
    QueryResult* banResult =
        LoginDatabase.PQuery("SELECT 1 FROM `account_banned` WHERE `id` = %u AND `active` = 1 "
                             "AND (`unbandate` > UNIX_TIMESTAMP() OR `unbandate` = `bandate`) "
                             "UNION "
                             "SELECT 1 FROM `ip_banned` WHERE (`unbandate` = `bandate` OR "
                             "`unbandate` > UNIX_TIMESTAMP()) AND `ip` = '%s'",
                             row->id, request.peerAddress.c_str());

    if (banResult)
    {
        delete banResult;
        sLog.outError("WorldGateway: banned account '%s' tried to connect from %s",
                      request.account.c_str(), request.peerAddress.c_str());
        result.status = proto::AuthStatus::Banned;
        return result;
    }

    // ---- Security floor (server closed to ordinary players) ------------------
    // WorldSocket.cpp:1166-1179.
    const AccountTypes allowed = sWorld.GetPlayerSecurityLimit();
    if (allowed > SEC_PLAYER && row->security < allowed)
    {
        BASIC_LOG("WorldGateway: account '%s' tried to login but its security "
                  "level is not enough", request.account.c_str());
        result.status = proto::AuthStatus::Unavailable;
        return result;
    }

    // ---- Warden's client OS rule ----------------------------------------------
    // WorldSocket.cpp:1181-1191.
    const bool wardenActive = sWorld.getConfig(CONFIG_BOOL_WARDEN_WIN_ENABLED)
                           || sWorld.getConfig(CONFIG_BOOL_WARDEN_OSX_ENABLED);

    if (wardenActive && row->os != "Win" && row->os != "OSX")
    {
        BASIC_LOG("WorldGateway: client %s attempted to log in using invalid "
                  "client OS (%s)", request.peerAddress.c_str(), row->os.c_str());
        result.status = proto::AuthStatus::Reject;
        return result;
    }

    result.status     = proto::AuthStatus::Ok;
    result.sessionKey = row->sessionKey;
    result.context    = row;
    return result;
}

proto::SessionId WorldGateway::Attach(const proto::AuthRequest& request,
                                      const std::shared_ptr<proto::IClientLink>& link,
                                      const std::shared_ptr<proto::AuthContext>& context)
{
    EnsureDbThreadRegistered();

    AccountRow* row = static_cast<AccountRow*>(context.get());
    if (row == nullptr)
    {
        return proto::INVALID_SESSION_ID;
    }

    // The proof has already been verified by this point, so recording the
    // address here is recording an authenticated login rather than an attempt.
    // WorldSocket.cpp:1225-1230.
    static SqlStatementID updAccount;
    SqlStatement stmt = LoginDatabase.CreateStatement(
        updAccount, "UPDATE `account` SET `last_ip` = ? WHERE `username` = ?");
    stmt.PExecute(request.peerAddress.c_str(), request.account.c_str());

    std::shared_ptr<SessionMailbox> mailbox =
        std::make_shared<SessionMailbox>();
    std::unique_ptr<WorldSession> session =
        std::make_unique<WorldSession>(
            row->id, link, mailbox, row->security, row->expansion,
            row->muteTime, row->locale, row->sessionSalt);

    session->LoadGlobalAccountData();
    session->LoadTutorialsData();

    // The addon block was left opaque by the protocol layer precisely so it
    // could be parsed here, where the addon registry lives. Same shape
    // WorldSocket.cpp:1018-1020 built: a plain ByteBuffer holding exactly the
    // addon bytes, read position at zero.
    ByteBuffer addonBuffer;
    if (!request.addonData.empty())
    {
        addonBuffer.append(request.addonData.data(), request.addonData.size());
    }
    session->ReadAddonsInfo(addonBuffer);

    // WorldSocket.cpp:1244-1248.
    const bool wardenActive = sWorld.getConfig(CONFIG_BOOL_WARDEN_WIN_ENABLED)
                           || sWorld.getConfig(CONFIG_BOOL_WARDEN_OSX_ENABLED);
    if (wardenActive)
    {
        session->InitWarden(uint16(request.build), &row->sessionKey, row->os);
    }

    proto::SessionId id;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        do
        {
            id = m_nextId++;
            if (id == proto::INVALID_SESSION_ID)
            {
                id = m_nextId++;
            }
        }
        while (m_routes.find(id) != m_routes.end());
        m_routes.emplace(id, mailbox);
    }

    // AddSession answers the client itself, with either AUTH_OK or a queue
    // position (WorldSession::SendAuthWaitQue). The cipher was armed by proto
    // before we were called (ClientConnection::HandleAuthSession), so that
    // reply goes out encrypted -- the one ordering constraint across this seam.
    // Ownership only leaves this function once AddSession has taken it; if it
    // throws, the route is torn down and the session freed rather than leaked.
    WorldSession* published = session.release();
    try
    {
        sWorld.AddSession(published);
    }
    catch (...)
    {
        session.reset(published);
        Detach(id);
        throw;
    }

    return id;
}

void WorldGateway::Deliver(proto::SessionId session, WorldPacket&& packet)
{
    // WorldSocket.cpp:864-868: opcodes outside the known table were rejected
    // at the transport. proto cannot do that check -- NUM_MSG_TYPES is game
    // knowledge it must not link -- so it happens here instead, on the one
    // path proto can no longer bypass.
    if (uint16(packet.GetOpcode()) >= NUM_MSG_TYPES)
    {
        sLog.outError("WorldGateway: received nonexistent opcode 0x%.4X",
                      packet.GetOpcode());
        return;
    }

    // Dump incoming packet (opt-in via PacketLoggingEnabled; off by default).
    // WorldSocket.cpp:875-879 did this with the ACE socket fd as the "SOCKET:"
    // field; the proto SessionId is the modern equivalent -- a slot assigned
    // once at Attach() and released at Detach(), same lifetime shape as a fd.
    if (sLog.IsPacketLoggingEnabled())
    {
        sLog.outWorldPacketDump(session, packet.GetOpcode(),
                                LookupOpcodeName(packet.GetOpcode()), &packet, true);
    }

    std::shared_ptr<SessionMailbox> mailbox;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        auto route = m_routes.find(session);
        if (route == m_routes.end())
        {
            return;
        }
        mailbox = route->second;
    }

    // The lock is already released: the mailbox holds its own, and a post to a
    // closed one is refused rather than fatal.
    mailbox->Enqueue(std::unique_ptr<WorldPacket>(new WorldPacket(std::move(packet))));
}

void WorldGateway::Detach(proto::SessionId session)
{
    std::shared_ptr<SessionMailbox> mailbox;
    {
        std::lock_guard<std::mutex> lock(m_lock);

        auto route = m_routes.find(session);
        if (route == m_routes.end())
        {
            return;
        }
        mailbox = route->second;
        m_routes.erase(route);
    }

    mailbox->Close();

    // The session is not deleted here, and it is not kicked from this thread
    // either. The transport marked the link closed before calling us;
    // WorldSession::Update observes that on the world thread, logs the player
    // out and returns false, and World::UpdateSessions reaps the session. That
    // is the only thread allowed to save a player and take them off the map.
}

bool WorldGateway::OnAuthPacketReceived(WorldPacket& packet)
{
    // WorldSocket.cpp:896-904. No session exists yet at this point -- the
    // socket-era code passed its own (necessarily still-null) m_Session here
    // too, and Eluna::OnPacketReceive already null-checks it.
#ifdef ENABLE_ELUNA
    if (Eluna* e = sWorld.GetEluna())
    {
        if (!e->OnPacketReceive(NULL, packet))
        {
            return false;
        }
    }
#endif
    return true;
}
