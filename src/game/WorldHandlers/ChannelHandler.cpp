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

/**
 * @file ChannelHandler.cpp
 * @brief Channel-related opcode handlers
 *
 * This file handles channel-related opcodes including:
 * - CMSG_JOIN_CHANNEL: Join a channel
 * - CMSG_LEAVE_CHANNEL: Leave a channel
 * - CMSG_CHANNEL_LIST: List channels
 * - CMSG_CHANNEL_PASSWORD: Set channel password
 * - CMSG_CHANNEL_OWNER: Set channel owner
 * - CMSG_CHANNEL_MODERATOR: Set channel moderator
 * - CMSG_CHANNEL_MUTE: Mute channel member
 * - CMSG_CHANNEL_UNMUTE: Unmute channel member
 * - CMSG_CHANNEL_INVITE: Invite to channel
 * - CMSG_CHANNEL_KICK: Kick from channel
 * - CMSG_CHANNEL_BAN: Ban from channel
 * - CMSG_CHANNEL_UNBAN: Unban from channel
 */

#include "ObjectMgr.h"                                      // for normalizePlayerName
#include "ChannelMgr.h"

/**
 * @brief Handles a client's request to join a chat channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleJoinChannelOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());

    uint32 channel_id;
    std::string channelname, pass;

    recvPacket >> channel_id;
    recvPacket.ReadBit();       // has voice
    recvPacket.ReadBit();       // zone update

    uint8 channelLength = recvPacket.ReadBits(8);
    uint8 passwordLength = recvPacket.ReadBits(8);
    channelname = recvPacket.ReadString(channelLength);
    pass = recvPacket.ReadString(passwordLength);

    if (channelname.empty())
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetJoinChannel(channelname, channel_id))
        {
            chn->Join(_player, pass.c_str());
        }
}

/**
 * @brief Handles a client's request to leave a chat channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleLeaveChannelOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();

    uint32 unk;
    std::string channelname;
    recvPacket >> unk;                                      // channel id?
    channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (channelname.empty())
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Leave(_player, true);
        }
        cMgr->LeftChannel(channelname);
    }
}

/**
 * @brief Sends the member list for a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelListOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->List(_player);
        }
}

/**
 * @brief Changes the password for a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelPasswordOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 nameLen, passLen;
    std::string channelname, pass;

    nameLen = recvPacket.ReadBits(8);
    passLen = recvPacket.ReadBits(7);
    channelname = recvPacket.ReadString(nameLen);
    pass = recvPacket.ReadString(passLen);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Password(_player, pass.c_str());
        }
}

/**
 * @brief Sets a new owner for a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelSetOwnerOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();

    uint32 channelLen, nameLen;
    std::string channelname, newp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    newp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(newp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->SetOwner(_player->GetObjectGuid(), newp.c_str());
        }
}

/**
 * @brief Requests the current owner of a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelOwnerOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->SendWhoOwner(_player);
        }
}

/**
 * @brief Grants moderator privileges to a channel member.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelModeratorOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->SetModerator(_player, otp.c_str());
        }
}

/**
 * @brief Removes moderator privileges from a channel member.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelUnmoderatorOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->UnsetMute(_player, otp.c_str());
        }
}

/**
 * @brief Mutes a member in a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelMuteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->SetMute(_player, otp.c_str());
        }
}

/**
 * @brief Unmutes a member in a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelUnmuteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->UnsetMute(_player, otp.c_str());
        }
}

/**
 * @brief Invites another player to a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelInviteOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Invite(_player, otp.c_str());
        }
}

/**
 * @brief Kicks a member from a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelKickOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    channelname = recvPacket.ReadString(channelLen);
    otp = recvPacket.ReadString(nameLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Kick(_player, otp.c_str());
        }
}

/**
 * @brief Bans a member from a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelBanOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    channelLen = recvPacket.ReadBits(8);
    nameLen = recvPacket.ReadBits(7);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Ban(_player, otp.c_str());
        }
}

/**
 * @brief Removes a ban for a member in a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelUnbanOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    uint32 channelLen, nameLen;
    std::string channelname, otp;

    nameLen = recvPacket.ReadBits(7);
    channelLen = recvPacket.ReadBits(8);
    otp = recvPacket.ReadString(nameLen);
    channelname = recvPacket.ReadString(channelLen);

    if (!normalizePlayerName(otp))
    {
        return;
    }

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->UnBan(_player, otp.c_str());
        }
}

/**
 * @brief Toggles channel join and leave announcements.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelAnnouncementsOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    //recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Announce(_player);
        }
}

/**
 * @brief Toggles moderated mode for a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelModerateOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->Moderate(_player);
        }
}

/**
 * @brief Handles a channel display list query.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleChannelDisplayListQueryOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname = recvPacket.ReadString(recvPacket.ReadBits(8));
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            chn->List(_player);
        }
}

/**
 * @brief Sends the current member count for a channel.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleGetChannelMemberCountOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            WorldPacket data(SMSG_CHANNEL_MEMBER_COUNT, chn->GetName().size() + 1 + 1 + 4);
            data << chn->GetName();
            data << uint8(chn->GetFlags());
            data << uint32(chn->GetNumPlayers());
            SendPacket(&data);
        }
    }
}

/**
 * @brief Handles a channel watch request from the client.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleSetChannelWatchOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: Received opcode %s (%u, 0x%X)", recvPacket.GetOpcodeName(), recvPacket.GetOpcode(), recvPacket.GetOpcode());
    // recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    /*if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
        {
            chn->JoinNotify(_player->GetGUID());
        }*/
}
