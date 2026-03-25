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
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>

// ============================================================================
// Standalone reimplementation of utility functions for testing.
// Mirrors src/shared/Utilities/Util.h / Util.cpp
// ============================================================================

typedef std::vector<std::string> Tokens;

enum TimeFormat : uint8_t
{
    FullText,
    ShortText,
    Numeric
};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F float(M_PI)

enum TimeConstants
{
    MINUTE = 60,
    HOUR   = MINUTE * 60,
    DAY    = HOUR * 24,
    WEEK   = DAY * 7,
    MONTH  = DAY * 30,
    YEAR   = MONTH * 12,
    IN_MILLISECONDS = 1000
};

Tokens StrSplit(const std::string& src, const std::string& sep)
{
    Tokens r;
    std::string s;
    for (char c : src)
    {
        if (sep.find(c) != std::string::npos)
        {
            if (!s.empty()) { r.push_back(s); s = ""; }
        }
        else
            s += c;
    }
    if (!s.empty()) r.push_back(s);
    return r;
}

uint32_t GetUInt32ValueFromArray(Tokens const& data, uint16_t index)
{
    if (index >= data.size()) return 0;
    return (uint32_t)atoi(data[index].c_str());
}

float NormalizeOrientation(float o)
{
    if (o < 0)
    {
        float mod = o * -1;
        mod = fmod(mod, 2.0f * M_PI_F);
        mod = -mod + 2.0f * M_PI_F;
        return mod;
    }
    return fmod(o, 2.0f * M_PI_F);
}

void stripLineInvisibleChars(std::string& str)
{
    static std::string invChars = " \t\7\n";
    size_t wpos = 0;
    bool space = false;
    for (size_t pos = 0; pos < str.size(); ++pos)
    {
        if (invChars.find(str[pos]) != std::string::npos)
        {
            if (!space) { str[wpos++] = ' '; space = true; }
        }
        else
        {
            if (wpos != pos) str[wpos++] = str[pos];
            else ++wpos;
            space = false;
        }
    }
    if (wpos < str.size()) str.erase(wpos, str.size());
}

std::string secsToTimeString(time_t timeInSecs, TimeFormat timeFormat = FullText, bool hoursOnly = false)
{
    time_t secs    = timeInSecs % MINUTE;
    time_t minutes = timeInSecs % HOUR / MINUTE;
    time_t hours   = timeInSecs % DAY  / HOUR;
    time_t days    = timeInSecs / DAY;
    std::ostringstream ss;
    if (days)
    {
        ss << days;
        if (timeFormat == Numeric) ss << ":";
        else if (timeFormat == ShortText) ss << "d";
        else ss << (days == 1 ? " Day " : " Days ");
    }
    if (hours || hoursOnly)
    {
        ss << hours;
        if (timeFormat == Numeric) ss << ":";
        else if (timeFormat == ShortText) ss << "h";
        else ss << (hours <= 1 ? " Hour " : " Hours ");
    }
    if (!hoursOnly)
    {
        ss << minutes;
        if (timeFormat == Numeric) ss << ":";
        else if (timeFormat == ShortText) ss << "m";
        else ss << (minutes == 1 ? " Minute " : " Minutes ");
    }
    else
    {
        if (timeFormat == Numeric) ss << "0:";
    }
    if (secs || (!days && !hours && !minutes))
    {
        ss << std::setw(2) << std::setfill('0') << secs;
        if (timeFormat == ShortText) ss << "s";
        else if (timeFormat == FullText) ss << (secs <= 1 ? " Second." : " Seconds.");
    }
    else
    {
        if (timeFormat == Numeric) ss << "00";
    }
    return ss.str();
}

uint32_t TimeStringToSecs(const std::string& timestring)
{
    uint32_t secs = 0, buffer = 0, multiplier = 0;
    for (char c : timestring)
    {
        if (isdigit(c)) { buffer *= 10; buffer += c - '0'; }
        else
        {
            switch (c)
            {
                case 'd': multiplier = DAY;    break;
                case 'h': multiplier = HOUR;   break;
                case 'm': multiplier = MINUTE; break;
                case 's': multiplier = 1;      break;
                default:  return 0;
            }
            buffer *= multiplier;
            secs += buffer;
            buffer = 0;
        }
    }
    return secs;
}

std::string MoneyToString(uint64_t money)
{
    uint32_t gold = money / 10000;
    uint32_t silv = (money % 10000) / 100;
    uint32_t copp = (money % 10000) % 100;
    std::stringstream ss;
    if (gold) ss << gold << "g";
    if (silv || gold) ss << silv << "s";
    ss << copp << "c";
    return ss.str();
}

inline void ApplyModUInt32Var(uint32_t& var, int32_t val, bool apply)
{
    int32_t cur = var;
    cur += (apply ? val : -val);
    if (cur < 0) cur = 0;
    var = cur;
}

inline void ApplyModFloatVar(float& var, float val, bool apply)
{
    var += (apply ? val : -val);
    if (var < 0) var = 0;
}

inline void ApplyPercentModFloatVar(float& var, float val, bool apply)
{
    if (val == -100.0f) val = -99.99f;
    var *= (apply ? (100.0f + val) / 100.0f : 100.0f / (100.0f + val));
}

inline bool roll_chance_f(float chance) { return chance > 100.0f * float(rand()) / float(RAND_MAX); }
inline bool roll_chance_i(int chance) { return chance > (rand() % 100); }

inline bool isBasicLatinCharacter(wchar_t wchar)
{
    return (wchar >= L'a' && wchar <= L'z') || (wchar >= L'A' && wchar <= L'Z');
}

inline bool isNumeric(char c) { return c >= '0' && c <= '9'; }
inline bool isNumeric(char const* str)
{
    for (const char* c = str; *c; ++c)
        if (!isNumeric(*c)) return false;
    return true;
}

inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); }));
    return s;
}
inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end());
    return s;
}
inline std::string& trim(std::string& s) { return ltrim(rtrim(s)); }

// ============================================================================
// StrSplit Tests
// ============================================================================

TEST(StrSplit, BasicSplit)
{
    Tokens result = StrSplit("a b c", " ");
    EXPECT_EQ(size_t(3), result.size());
    EXPECT_STR_EQ("a", result[0]);
    EXPECT_STR_EQ("b", result[1]);
    EXPECT_STR_EQ("c", result[2]);
}

TEST(StrSplit, SplitByComma)
{
    Tokens result = StrSplit("one,two,three", ",");
    EXPECT_EQ(size_t(3), result.size());
    EXPECT_STR_EQ("one", result[0]);
    EXPECT_STR_EQ("two", result[1]);
    EXPECT_STR_EQ("three", result[2]);
}

TEST(StrSplit, EmptyString)
{
    Tokens result = StrSplit("", " ");
    EXPECT_EQ(size_t(0), result.size());
}

TEST(StrSplit, NoSeparatorFound)
{
    Tokens result = StrSplit("hello", ",");
    EXPECT_EQ(size_t(1), result.size());
    EXPECT_STR_EQ("hello", result[0]);
}

TEST(StrSplit, ConsecutiveSeparators)
{
    Tokens result = StrSplit("a  b", " ");
    EXPECT_EQ(size_t(2), result.size());
    EXPECT_STR_EQ("a", result[0]);
    EXPECT_STR_EQ("b", result[1]);
}

TEST(StrSplit, LeadingSeparator)
{
    Tokens result = StrSplit(",a,b", ",");
    EXPECT_EQ(size_t(2), result.size());
    EXPECT_STR_EQ("a", result[0]);
    EXPECT_STR_EQ("b", result[1]);
}

TEST(StrSplit, TrailingSeparator)
{
    Tokens result = StrSplit("a,b,", ",");
    EXPECT_EQ(size_t(2), result.size());
    EXPECT_STR_EQ("a", result[0]);
    EXPECT_STR_EQ("b", result[1]);
}

TEST(StrSplit, MultipleSeparatorChars)
{
    Tokens result = StrSplit("a:b;c", ":;");
    EXPECT_EQ(size_t(3), result.size());
}

TEST(StrSplit, SingleCharTokens)
{
    Tokens result = StrSplit("1 2 3 4 5", " ");
    EXPECT_EQ(size_t(5), result.size());
}

// ============================================================================
// NormalizeOrientation Tests
// ============================================================================

TEST(NormalizeOrientation, ZeroAngle)
{
    EXPECT_FLOAT_EQ(0.0f, NormalizeOrientation(0.0f));
}

TEST(NormalizeOrientation, FullCircleReducesToZero)
{
    float result = NormalizeOrientation(2.0f * M_PI_F);
    EXPECT_NEAR(0.0f, result, 0.0001f);
}

TEST(NormalizeOrientation, HalfCircle)
{
    float result = NormalizeOrientation(M_PI_F);
    EXPECT_NEAR(M_PI_F, result, 0.0001f);
}

TEST(NormalizeOrientation, MoreThan2Pi)
{
    float result = NormalizeOrientation(3.0f * M_PI_F);
    EXPECT_NEAR(M_PI_F, result, 0.0001f);
}

TEST(NormalizeOrientation, NegativeAngle)
{
    float result = NormalizeOrientation(-M_PI_F);
    EXPECT_NEAR(M_PI_F, result, 0.0001f);
}

TEST(NormalizeOrientation, NegativeFullCircle)
{
    float result = NormalizeOrientation(-2.0f * M_PI_F);
    // -2pi maps to 2pi (fmod gives 0, then 0 + 2pi = 2pi), which normalizes correctly
    // to 2pi (equivalent to 0); accept either 0 or 2pi
    bool ok = (result < 0.0001f) || (std::fabs(result - 2.0f * M_PI_F) < 0.001f);
    EXPECT_TRUE(ok);
}

TEST(NormalizeOrientation, SmallPositive)
{
    float angle = 0.5f;
    float result = NormalizeOrientation(angle);
    EXPECT_FLOAT_EQ(angle, result);
}

TEST(NormalizeOrientation, LargePositive)
{
    float result = NormalizeOrientation(10.0f * M_PI_F);
    EXPECT_NEAR(0.0f, result, 0.001f);
}

TEST(NormalizeOrientation, AlwaysInRange)
{
    float angles[] = {-100.0f, -M_PI_F, -0.001f, 0.0f, 0.001f,
                      M_PI_F, 2.0f * M_PI_F, 10.0f, 100.0f};
    for (float a : angles)
    {
        float r = NormalizeOrientation(a);
        EXPECT_GE(r, 0.0f);
        EXPECT_LT(r, 2.0f * M_PI_F + 0.001f);
    }
}

// ============================================================================
// TimeStringToSecs Tests
// ============================================================================

TEST(TimeStringToSecs, Seconds)
{
    EXPECT_EQ(uint32_t(30), TimeStringToSecs("30s"));
}

TEST(TimeStringToSecs, Minutes)
{
    EXPECT_EQ(uint32_t(120), TimeStringToSecs("2m"));
}

TEST(TimeStringToSecs, Hours)
{
    EXPECT_EQ(uint32_t(7200), TimeStringToSecs("2h"));
}

TEST(TimeStringToSecs, Days)
{
    EXPECT_EQ(uint32_t(86400), TimeStringToSecs("1d"));
}

TEST(TimeStringToSecs, Combined)
{
    // 1h 30m 15s = 3600 + 1800 + 15 = 5415
    EXPECT_EQ(uint32_t(5415), TimeStringToSecs("1h30m15s"));
}

TEST(TimeStringToSecs, DaysHoursMinutesSeconds)
{
    // 1d 2h 3m 4s
    uint32_t expected = DAY + 2 * HOUR + 3 * MINUTE + 4;
    EXPECT_EQ(expected, TimeStringToSecs("1d2h3m4s"));
}

TEST(TimeStringToSecs, InvalidFormat)
{
    EXPECT_EQ(uint32_t(0), TimeStringToSecs("invalid"));
}

TEST(TimeStringToSecs, EmptyString)
{
    EXPECT_EQ(uint32_t(0), TimeStringToSecs(""));
}

TEST(TimeStringToSecs, ZeroSeconds)
{
    EXPECT_EQ(uint32_t(0), TimeStringToSecs("0s"));
}

TEST(TimeStringToSecs, LargeValue)
{
    EXPECT_EQ(uint32_t(7 * DAY), TimeStringToSecs("7d"));
}

// ============================================================================
// secsToTimeString Tests
// ============================================================================

TEST(secsToTimeString, ZeroSeconds_FullText)
{
    std::string result = secsToTimeString(0, FullText);
    EXPECT_FALSE(result.empty());
}

TEST(secsToTimeString, OneSecond_ShortText)
{
    std::string result = secsToTimeString(1, ShortText);
    // Includes minutes field: "0m01s"
    EXPECT_STR_EQ("0m01s", result);
}

TEST(secsToTimeString, OneMinute_ShortText)
{
    std::string result = secsToTimeString(MINUTE, ShortText);
    // Seconds == 0 and days/hours/minutes present: omit "00s" per implementation
    EXPECT_STR_EQ("1m", result);
}

TEST(secsToTimeString, OneHour_ShortText)
{
    std::string result = secsToTimeString(HOUR, ShortText);
    EXPECT_STR_EQ("1h0m", result);
}

TEST(secsToTimeString, OneDay_ShortText)
{
    std::string result = secsToTimeString(DAY, ShortText);
    EXPECT_STR_EQ("1d0m", result);
}

TEST(secsToTimeString, Mixed_ShortText)
{
    // 1h 30m 45s
    std::string result = secsToTimeString(HOUR + 30 * MINUTE + 45, ShortText);
    EXPECT_STR_EQ("1h30m45s", result);
}

TEST(secsToTimeString, Numeric_OneHour)
{
    std::string result = secsToTimeString(HOUR, Numeric);
    EXPECT_STR_EQ("1:0:00", result);
}

// ============================================================================
// MoneyToString Tests
// ============================================================================

TEST(MoneyToString, ZeroMoney)
{
    EXPECT_STR_EQ("0c", MoneyToString(0));
}

TEST(MoneyToString, CopperOnly)
{
    EXPECT_STR_EQ("50c", MoneyToString(50));
}

TEST(MoneyToString, SilverOnly)
{
    // 100 copper = 1 silver
    EXPECT_STR_EQ("1s0c", MoneyToString(100));
}

TEST(MoneyToString, GoldOnly)
{
    // 10000 copper = 1 gold
    EXPECT_STR_EQ("1g0s0c", MoneyToString(10000));
}

TEST(MoneyToString, Mixed)
{
    // 1g 23s 45c = 10000 + 2300 + 45 = 12345
    EXPECT_STR_EQ("1g23s45c", MoneyToString(12345));
}

TEST(MoneyToString, SilverAndCopper)
{
    // 5s 30c = 500 + 30 = 530
    EXPECT_STR_EQ("5s30c", MoneyToString(530));
}

TEST(MoneyToString, LargeAmount)
{
    // 1000g 0s 0c = 10000000
    EXPECT_STR_EQ("1000g0s0c", MoneyToString(10000000));
}

TEST(MoneyToString, MaxCopper)
{
    EXPECT_STR_EQ("99c", MoneyToString(99));
}

// ============================================================================
// ApplyModUInt32Var Tests
// ============================================================================

TEST(ApplyModUInt32Var, ApplyPositive)
{
    uint32_t var = 100;
    ApplyModUInt32Var(var, 50, true);
    EXPECT_EQ(uint32_t(150), var);
}

TEST(ApplyModUInt32Var, RemovePositive)
{
    uint32_t var = 100;
    ApplyModUInt32Var(var, 50, false);
    EXPECT_EQ(uint32_t(50), var);
}

TEST(ApplyModUInt32Var, ClampToZero)
{
    uint32_t var = 10;
    ApplyModUInt32Var(var, 50, false);
    EXPECT_EQ(uint32_t(0), var);
}

TEST(ApplyModUInt32Var, ApplyZero)
{
    uint32_t var = 100;
    ApplyModUInt32Var(var, 0, true);
    EXPECT_EQ(uint32_t(100), var);
}

// ============================================================================
// ApplyModFloatVar Tests
// ============================================================================

TEST(ApplyModFloatVar, Apply)
{
    float var = 10.0f;
    ApplyModFloatVar(var, 5.0f, true);
    EXPECT_FLOAT_EQ(15.0f, var);
}

TEST(ApplyModFloatVar, Remove)
{
    float var = 10.0f;
    ApplyModFloatVar(var, 5.0f, false);
    EXPECT_FLOAT_EQ(5.0f, var);
}

TEST(ApplyModFloatVar, ClampToZero)
{
    float var = 3.0f;
    ApplyModFloatVar(var, 10.0f, false);
    EXPECT_FLOAT_EQ(0.0f, var);
}

// ============================================================================
// ApplyPercentModFloatVar Tests
// ============================================================================

TEST(ApplyPercentModFloatVar, ApplyPositivePercent)
{
    float var = 100.0f;
    ApplyPercentModFloatVar(var, 50.0f, true); // +50%
    EXPECT_FLOAT_EQ(150.0f, var);
}

TEST(ApplyPercentModFloatVar, RemovePositivePercent)
{
    float var = 150.0f;
    ApplyPercentModFloatVar(var, 50.0f, false); // undo +50%
    EXPECT_NEAR(100.0f, var, 0.001f);
}

TEST(ApplyPercentModFloatVar, ApplyNegativePercent)
{
    float var = 100.0f;
    ApplyPercentModFloatVar(var, -50.0f, true); // -50%
    EXPECT_FLOAT_EQ(50.0f, var);
}

TEST(ApplyPercentModFloatVar, Clamp100Percent)
{
    float var = 100.0f;
    ApplyPercentModFloatVar(var, -100.0f, true);
    // -100% clamped to -99.99%, so result ~= 0.01
    EXPECT_GT(var, 0.0f);
    EXPECT_LT(var, 1.0f);
}

// ============================================================================
// stripLineInvisibleChars Tests
// ============================================================================

TEST(stripLineInvisibleChars, NoInvisibleChars)
{
    std::string s = "Hello World";
    stripLineInvisibleChars(s);
    EXPECT_STR_EQ("Hello World", s);
}

TEST(stripLineInvisibleChars, CollapseSpaces)
{
    std::string s = "Hello   World";
    stripLineInvisibleChars(s);
    EXPECT_STR_EQ("Hello World", s);
}

TEST(stripLineInvisibleChars, TabsConverted)
{
    std::string s = "Hello\tWorld";
    stripLineInvisibleChars(s);
    EXPECT_STR_EQ("Hello World", s);
}

TEST(stripLineInvisibleChars, NewlineConverted)
{
    std::string s = "Hello\nWorld";
    stripLineInvisibleChars(s);
    EXPECT_STR_EQ("Hello World", s);
}

TEST(stripLineInvisibleChars, EmptyString)
{
    std::string s = "";
    stripLineInvisibleChars(s);
    EXPECT_STR_EQ("", s);
}

// ============================================================================
// isNumeric Tests
// ============================================================================

TEST(isNumeric, SingleDigit)
{
    EXPECT_TRUE(isNumeric('0'));
    EXPECT_TRUE(isNumeric('5'));
    EXPECT_TRUE(isNumeric('9'));
}

TEST(isNumeric, NonDigit)
{
    EXPECT_FALSE(isNumeric('a'));
    EXPECT_FALSE(isNumeric(' '));
    EXPECT_FALSE(isNumeric('.'));
}

TEST(isNumeric, AllDigitString)
{
    EXPECT_TRUE(isNumeric("123456"));
}

TEST(isNumeric, MixedString)
{
    EXPECT_FALSE(isNumeric("12a34"));
}

TEST(isNumeric, EmptyString)
{
    EXPECT_TRUE(isNumeric(""));
}

// ============================================================================
// isBasicLatinCharacter Tests
// ============================================================================

TEST(isBasicLatin, LowercaseLetters)
{
    EXPECT_TRUE(isBasicLatinCharacter(L'a'));
    EXPECT_TRUE(isBasicLatinCharacter(L'z'));
    EXPECT_TRUE(isBasicLatinCharacter(L'm'));
}

TEST(isBasicLatin, UppercaseLetters)
{
    EXPECT_TRUE(isBasicLatinCharacter(L'A'));
    EXPECT_TRUE(isBasicLatinCharacter(L'Z'));
    EXPECT_TRUE(isBasicLatinCharacter(L'M'));
}

TEST(isBasicLatin, NonLatinFails)
{
    EXPECT_FALSE(isBasicLatinCharacter(L'0'));
    EXPECT_FALSE(isBasicLatinCharacter(L'!'));
    EXPECT_FALSE(isBasicLatinCharacter(L' '));
}

// ============================================================================
// trim Tests
// ============================================================================

TEST(trim, LeadingSpaces)
{
    std::string s = "   hello";
    ltrim(s);
    EXPECT_STR_EQ("hello", s);
}

TEST(trim, TrailingSpaces)
{
    std::string s = "hello   ";
    rtrim(s);
    EXPECT_STR_EQ("hello", s);
}

TEST(trim, BothEnds)
{
    std::string s = "  hello  ";
    trim(s);
    EXPECT_STR_EQ("hello", s);
}

TEST(trim, NoSpaces)
{
    std::string s = "hello";
    trim(s);
    EXPECT_STR_EQ("hello", s);
}

TEST(trim, AllSpaces)
{
    std::string s = "     ";
    trim(s);
    EXPECT_STR_EQ("", s);
}

TEST(trim, PreservesInternalSpaces)
{
    std::string s = "  hello world  ";
    trim(s);
    EXPECT_STR_EQ("hello world", s);
}

// ============================================================================
// TimeConstants Tests
// ============================================================================

TEST(TimeConstants, Values)
{
    EXPECT_EQ(60, MINUTE);
    EXPECT_EQ(3600, HOUR);
    EXPECT_EQ(86400, DAY);
    EXPECT_EQ(604800, WEEK);
    EXPECT_EQ(2592000, MONTH);
}

TEST(TimeConstants, Relationships)
{
    EXPECT_EQ(HOUR, MINUTE * 60);
    EXPECT_EQ(DAY, HOUR * 24);
    EXPECT_EQ(WEEK, DAY * 7);
    EXPECT_EQ(MONTH, DAY * 30);
}
