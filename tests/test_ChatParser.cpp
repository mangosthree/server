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
 */

#include "TestFramework.h"

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>

// ============================================================================
// Standalone reimplementation of Chat command parsing patterns
// from src/game/ChatCommands/
// ============================================================================

namespace ChatTest
{

enum ChatMsg
{
    CHAT_MSG_SAY            = 0x00,
    CHAT_MSG_PARTY          = 0x01,
    CHAT_MSG_RAID           = 0x02,
    CHAT_MSG_GUILD          = 0x03,
    CHAT_MSG_OFFICER        = 0x04,
    CHAT_MSG_YELL           = 0x05,
    CHAT_MSG_WHISPER        = 0x06,
    CHAT_MSG_WHISPER_INFORM = 0x07,
    CHAT_MSG_EMOTE          = 0x08,
    CHAT_MSG_CHANNEL        = 0x11,
    CHAT_MSG_SYSTEM         = 0x40,
};

enum AccountTypes
{
    SEC_PLAYER        = 0,
    SEC_MODERATOR     = 1,
    SEC_GAMEMASTER    = 2,
    SEC_ADMINISTRATOR = 3,
    SEC_CONSOLE       = 4
};

struct ChatCommand
{
    const char* Name;
    int32_t SecurityLevel;
    bool AllowConsole;
};

// Command parsing: extract command name from input
std::string ExtractCommand(const std::string& input)
{
    if (input.empty() || input[0] != '.') return "";
    size_t end = input.find(' ', 1);
    if (end == std::string::npos) end = input.size();
    return input.substr(1, end - 1);
}

// Extract arguments after command
std::string ExtractArgs(const std::string& input)
{
    size_t pos = input.find(' ');
    if (pos == std::string::npos) return "";
    pos++; // skip the space
    while (pos < input.size() && input[pos] == ' ') ++pos; // skip extra spaces
    return input.substr(pos);
}

// Parse a player name (first letter uppercase, rest lowercase)
std::string NormalizePlayerName(const std::string& name)
{
    if (name.empty()) return "";
    std::string result = name;
    result[0] = toupper(result[0]);
    for (size_t i = 1; i < result.size(); ++i)
        result[i] = tolower(result[i]);
    return result;
}

// Validate player name (2-12 chars, only letters)
bool IsValidPlayerName(const std::string& name)
{
    if (name.size() < 2 || name.size() > 12) return false;
    for (char c : name)
        if (!isalpha(c)) return false;
    return true;
}

// Parse link format: |cFFFFFFFF|Hitem:12345:0:0:0|h[Item Name]|h|r
bool ParseItemLink(const std::string& link, uint32_t& itemId)
{
    size_t pos = link.find("|Hitem:");
    if (pos == std::string::npos) return false;
    pos += 7; // skip "|Hitem:"
    size_t end = link.find(':', pos);
    if (end == std::string::npos) return false;
    itemId = uint32_t(atoi(link.substr(pos, end - pos).c_str()));
    return itemId > 0;
}

// Security check
bool HasPermission(int32_t playerSec, int32_t requiredSec)
{
    return playerSec >= requiredSec;
}

// Chat message length validation
bool IsValidChatMessage(const std::string& msg)
{
    if (msg.empty()) return false;
    if (msg.size() > 255) return false;
    return true;
}

// Format GM announcement
std::string FormatGMAnnouncement(const std::string& gmName, const std::string& message)
{
    return "[" + gmName + "]: " + message;
}

// Chat channel name validation
bool IsValidChannelName(const std::string& name)
{
    if (name.empty() || name.size() > 100) return false;
    for (char c : name)
        if (!isalnum(c) && c != ' ' && c != '_') return false;
    return true;
}

// Simple word filter check
bool ContainsBannedWord(const std::string& msg, const std::vector<std::string>& bannedWords)
{
    std::string lower = msg;
    for (char& c : lower) c = tolower(c);
    for (const auto& word : bannedWords)
    {
        std::string lowerWord = word;
        for (char& c : lowerWord) c = tolower(c);
        if (lower.find(lowerWord) != std::string::npos) return true;
    }
    return false;
}

} // namespace ChatTest

using namespace ChatTest;

// ============================================================================
// Chat Message Type Tests
// ============================================================================

TEST(ChatMsg, SayIsZero)
{
    EXPECT_EQ(0x00, CHAT_MSG_SAY);
}

TEST(ChatMsg, DistinctValues)
{
    EXPECT_NE(CHAT_MSG_SAY, CHAT_MSG_PARTY);
    EXPECT_NE(CHAT_MSG_PARTY, CHAT_MSG_RAID);
    EXPECT_NE(CHAT_MSG_WHISPER, CHAT_MSG_YELL);
}

TEST(ChatMsg, SystemHigherThanOthers)
{
    EXPECT_GT(CHAT_MSG_SYSTEM, CHAT_MSG_SAY);
    EXPECT_GT(CHAT_MSG_SYSTEM, CHAT_MSG_CHANNEL);
}

// ============================================================================
// Command Parsing Tests
// ============================================================================

TEST(CommandParser, BasicCommand)
{
    EXPECT_STR_EQ("help", ExtractCommand(".help"));
}

TEST(CommandParser, CommandWithArgs)
{
    EXPECT_STR_EQ("gm", ExtractCommand(".gm on"));
}

TEST(CommandParser, NoPrefix)
{
    EXPECT_STR_EQ("", ExtractCommand("help"));
}

TEST(CommandParser, EmptyInput)
{
    EXPECT_STR_EQ("", ExtractCommand(""));
}

TEST(CommandParser, JustDot)
{
    EXPECT_STR_EQ("", ExtractCommand("."));
}

TEST(CommandParser, MultiWordCommand)
{
    EXPECT_STR_EQ("account", ExtractCommand(".account set password"));
}

// ============================================================================
// Argument Extraction Tests
// ============================================================================

TEST(ArgParser, BasicArgs)
{
    EXPECT_STR_EQ("on", ExtractArgs(".gm on"));
}

TEST(ArgParser, MultipleArgs)
{
    EXPECT_STR_EQ("set password newpass", ExtractArgs(".account set password newpass"));
}

TEST(ArgParser, NoArgs)
{
    EXPECT_STR_EQ("", ExtractArgs(".help"));
}

TEST(ArgParser, ExtraSpaces)
{
    EXPECT_STR_EQ("on", ExtractArgs(".gm   on"));
}

// ============================================================================
// Player Name Normalization Tests
// ============================================================================

TEST(PlayerName, NormalizeAllLower)
{
    EXPECT_STR_EQ("Arthas", NormalizePlayerName("arthas"));
}

TEST(PlayerName, NormalizeAllUpper)
{
    EXPECT_STR_EQ("Arthas", NormalizePlayerName("ARTHAS"));
}

TEST(PlayerName, NormalizeMixed)
{
    EXPECT_STR_EQ("Arthas", NormalizePlayerName("aRTHAS"));
}

TEST(PlayerName, NormalizeSingleChar)
{
    EXPECT_STR_EQ("A", NormalizePlayerName("a"));
}

TEST(PlayerName, NormalizeEmpty)
{
    EXPECT_STR_EQ("", NormalizePlayerName(""));
}

// ============================================================================
// Player Name Validation Tests
// ============================================================================

TEST(PlayerNameValidation, ValidName)
{
    EXPECT_TRUE(IsValidPlayerName("Arthas"));
}

TEST(PlayerNameValidation, TooShort)
{
    EXPECT_FALSE(IsValidPlayerName("A"));
}

TEST(PlayerNameValidation, TooLong)
{
    EXPECT_FALSE(IsValidPlayerName("Abcdefghijklm"));
}

TEST(PlayerNameValidation, HasNumbers)
{
    EXPECT_FALSE(IsValidPlayerName("Player123"));
}

TEST(PlayerNameValidation, HasSpaces)
{
    EXPECT_FALSE(IsValidPlayerName("Art Has"));
}

TEST(PlayerNameValidation, Empty)
{
    EXPECT_FALSE(IsValidPlayerName(""));
}

TEST(PlayerNameValidation, MinLength)
{
    EXPECT_TRUE(IsValidPlayerName("Ab"));
}

TEST(PlayerNameValidation, MaxLength)
{
    EXPECT_TRUE(IsValidPlayerName("Abcdefghijkl")); // 12 chars
}

// ============================================================================
// Item Link Parsing Tests
// ============================================================================

TEST(ItemLink, BasicParse)
{
    uint32_t itemId = 0;
    EXPECT_TRUE(ParseItemLink("|cFFFFFFFF|Hitem:12345:0:0:0|h[Sword]|h|r", itemId));
    EXPECT_EQ(uint32_t(12345), itemId);
}

TEST(ItemLink, InvalidLink)
{
    uint32_t itemId = 0;
    EXPECT_FALSE(ParseItemLink("Not a link", itemId));
}

TEST(ItemLink, ZeroItemId)
{
    uint32_t itemId = 0;
    EXPECT_FALSE(ParseItemLink("|Hitem:0:0:0:0|h[Nothing]|h|r", itemId));
}

TEST(ItemLink, LargeItemId)
{
    uint32_t itemId = 0;
    EXPECT_TRUE(ParseItemLink("|Hitem:99999:0:0:0|h[Epic Item]|h|r", itemId));
    EXPECT_EQ(uint32_t(99999), itemId);
}

// ============================================================================
// Permission Tests
// ============================================================================

TEST(Permission, PlayerCanUsePlayerCommands)
{
    EXPECT_TRUE(HasPermission(SEC_PLAYER, SEC_PLAYER));
}

TEST(Permission, PlayerCannotUseGMCommands)
{
    EXPECT_FALSE(HasPermission(SEC_PLAYER, SEC_GAMEMASTER));
}

TEST(Permission, GMCanUsePlayerCommands)
{
    EXPECT_TRUE(HasPermission(SEC_GAMEMASTER, SEC_PLAYER));
}

TEST(Permission, AdminCanUseAll)
{
    EXPECT_TRUE(HasPermission(SEC_ADMINISTRATOR, SEC_PLAYER));
    EXPECT_TRUE(HasPermission(SEC_ADMINISTRATOR, SEC_MODERATOR));
    EXPECT_TRUE(HasPermission(SEC_ADMINISTRATOR, SEC_GAMEMASTER));
    EXPECT_TRUE(HasPermission(SEC_ADMINISTRATOR, SEC_ADMINISTRATOR));
}

TEST(Permission, ConsoleIsHighest)
{
    EXPECT_TRUE(HasPermission(SEC_CONSOLE, SEC_ADMINISTRATOR));
}

// ============================================================================
// Chat Message Validation Tests
// ============================================================================

TEST(ChatValidation, ValidMessage)
{
    EXPECT_TRUE(IsValidChatMessage("Hello World!"));
}

TEST(ChatValidation, EmptyMessage)
{
    EXPECT_FALSE(IsValidChatMessage(""));
}

TEST(ChatValidation, TooLong)
{
    std::string longMsg(256, 'A');
    EXPECT_FALSE(IsValidChatMessage(longMsg));
}

TEST(ChatValidation, MaxLength)
{
    std::string maxMsg(255, 'A');
    EXPECT_TRUE(IsValidChatMessage(maxMsg));
}

// ============================================================================
// GM Announcement Tests
// ============================================================================

TEST(GMAnnouncement, BasicFormat)
{
    EXPECT_STR_EQ("[Admin]: Server restart in 5 minutes",
                  FormatGMAnnouncement("Admin", "Server restart in 5 minutes"));
}

TEST(GMAnnouncement, EmptyMessage)
{
    EXPECT_STR_EQ("[Admin]: ", FormatGMAnnouncement("Admin", ""));
}

// ============================================================================
// Channel Name Validation Tests
// ============================================================================

TEST(ChannelName, ValidName)
{
    EXPECT_TRUE(IsValidChannelName("General"));
}

TEST(ChannelName, ValidWithSpaces)
{
    EXPECT_TRUE(IsValidChannelName("Looking For Group"));
}

TEST(ChannelName, ValidWithUnderscore)
{
    EXPECT_TRUE(IsValidChannelName("Custom_Channel"));
}

TEST(ChannelName, Empty)
{
    EXPECT_FALSE(IsValidChannelName(""));
}

TEST(ChannelName, InvalidChars)
{
    EXPECT_FALSE(IsValidChannelName("Chat!@#$"));
}

TEST(ChannelName, TooLong)
{
    std::string longName(101, 'A');
    EXPECT_FALSE(IsValidChannelName(longName));
}

// ============================================================================
// Word Filter Tests
// ============================================================================

TEST(WordFilter, NoBannedWords)
{
    std::vector<std::string> banned = {"badword"};
    EXPECT_FALSE(ContainsBannedWord("Hello world", banned));
}

TEST(WordFilter, ContainsBannedWord)
{
    std::vector<std::string> banned = {"badword"};
    EXPECT_TRUE(ContainsBannedWord("This has badword in it", banned));
}

TEST(WordFilter, CaseInsensitive)
{
    std::vector<std::string> banned = {"BadWord"};
    EXPECT_TRUE(ContainsBannedWord("this has BADWORD in it", banned));
}

TEST(WordFilter, EmptyMessage)
{
    std::vector<std::string> banned = {"bad"};
    EXPECT_FALSE(ContainsBannedWord("", banned));
}

TEST(WordFilter, EmptyBannedList)
{
    std::vector<std::string> banned;
    EXPECT_FALSE(ContainsBannedWord("anything goes", banned));
}
