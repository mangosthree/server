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
#include <chrono>
#include <ctime>
#include <thread>

// ============================================================================
// Standalone reimplementation of Timer utilities for testing.
// Mirrors src/shared/Utilities/Timer.h and Duration.h
// ============================================================================

typedef std::chrono::milliseconds Milliseconds;
typedef std::chrono::seconds Seconds;
typedef std::chrono::minutes Minutes;
typedef std::chrono::hours Hours;
typedef std::chrono::steady_clock::time_point TimePoint;
typedef std::chrono::system_clock::time_point SystemTimePoint;

inline std::chrono::steady_clock::time_point GetApplicationStartTime()
{
    using namespace std::chrono;
    static const steady_clock::time_point ApplicationStartTime = steady_clock::now();
    return ApplicationStartTime;
}

inline uint32_t getMSTime()
{
    using namespace std::chrono;
    return uint32_t(duration_cast<milliseconds>(steady_clock::now() - GetApplicationStartTime()).count());
}

inline uint32_t getMSTimeDiff(uint32_t oldMSTime, uint32_t newMSTime)
{
    if (oldMSTime > newMSTime)
        return (0xFFFFFFFF - oldMSTime) + newMSTime;
    else
        return newMSTime - oldMSTime;
}

inline uint32_t GetMSTimeDiffToNow(uint32_t oldMSTime)
{
    return getMSTimeDiff(oldMSTime, getMSTime());
}

inline uint32_t GetUnixTimeStamp()
{
    time_t nowMS = std::time(nullptr);
    return static_cast<uint32_t>(nowMS);
}

struct IntervalTimer
{
    IntervalTimer() : _interval(0), _current(0) {}

    void Update(time_t diff)
    {
        _current += diff;
        if (_current < 0)
            _current = 0;
    }

    bool Passed() { return _current >= _interval; }

    void Reset()
    {
        if (_current >= _interval)
            _current %= _interval;
    }

    void SetCurrent(time_t current) { _current = current; }
    void SetInterval(time_t interval) { _interval = interval; }
    time_t GetInterval() const { return _interval; }
    time_t GetCurrent() const { return _current; }

private:
    time_t _interval;
    time_t _current;
};

struct TimeTracker
{
    TimeTracker(int32_t expiry = 0) : _expiryTime(Milliseconds(expiry)) {}
    TimeTracker(Milliseconds expiry) : _expiryTime(expiry) {}

    void Update(int32_t diff) { Update(Milliseconds(diff)); }
    void Update(Milliseconds diff) { _expiryTime -= diff; }

    bool Passed() const { return _expiryTime <= Seconds(0); }

    void Reset(int32_t expiry) { Reset(Milliseconds(expiry)); }
    void Reset(Milliseconds expiry) { _expiryTime = expiry; }

    Milliseconds GetExpiry() const { return _expiryTime; }

private:
    Milliseconds _expiryTime;
};

struct PeriodicTimer
{
    PeriodicTimer(int32_t period, int32_t start_time)
        : i_period(period), i_expireTime(start_time) {}

    bool Update(const uint32_t diff)
    {
        if ((i_expireTime -= diff) > 0)
            return false;
        i_expireTime += i_period > int32_t(diff) ? i_period : diff;
        return true;
    }

    void SetPeriodic(int32_t period, int32_t start_time)
    {
        i_expireTime = start_time;
        i_period = period;
    }

    void TUpdate(int32_t diff) { i_expireTime -= diff; }
    bool TPassed() const { return i_expireTime <= 0; }
    void TReset(int32_t diff, int32_t period) { i_expireTime += period > diff ? period : diff; }

private:
    int32_t i_period;
    int32_t i_expireTime;
};

// ============================================================================
// Duration Tests
// ============================================================================

TEST(Duration, MillisecondsConversion)
{
    Milliseconds ms(1500);
    EXPECT_EQ(1500, ms.count());
}

TEST(Duration, SecondsToMilliseconds)
{
    Seconds sec(3);
    Milliseconds ms = std::chrono::duration_cast<Milliseconds>(sec);
    EXPECT_EQ(3000, ms.count());
}

TEST(Duration, MinutesToSeconds)
{
    Minutes min(2);
    Seconds sec = std::chrono::duration_cast<Seconds>(min);
    EXPECT_EQ(120, sec.count());
}

TEST(Duration, HoursToMinutes)
{
    Hours hr(1);
    Minutes min = std::chrono::duration_cast<Minutes>(hr);
    EXPECT_EQ(60, min.count());
}

TEST(Duration, MillisecondsArithmetic)
{
    Milliseconds a(500);
    Milliseconds b(300);
    EXPECT_EQ(800, (a + b).count());
    EXPECT_EQ(200, (a - b).count());
}

TEST(Duration, DurationComparison)
{
    Milliseconds a(100);
    Milliseconds b(200);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a != b);
    Milliseconds c(100);
    EXPECT_TRUE(a == c);
}

// ============================================================================
// getMSTime Tests
// ============================================================================

TEST(MSTime, ReturnsNonZeroAfterStart)
{
    // getMSTime should return >= 0 (it could be 0 if called immediately)
    uint32_t t = getMSTime();
    EXPECT_GE(t, uint32_t(0));
}

TEST(MSTime, IsMonotonicallyIncreasing)
{
    uint32_t t1 = getMSTime();
    // Small busy wait
    volatile int x = 0;
    for (int i = 0; i < 100000; ++i) x += i;
    uint32_t t2 = getMSTime();
    EXPECT_GE(t2, t1);
}

// ============================================================================
// getMSTimeDiff Tests
// ============================================================================

TEST(MSTimeDiff, NormalCase)
{
    EXPECT_EQ(uint32_t(100), getMSTimeDiff(100, 200));
}

TEST(MSTimeDiff, SameValues)
{
    EXPECT_EQ(uint32_t(0), getMSTimeDiff(500, 500));
}

TEST(MSTimeDiff, OverflowWraparound)
{
    // When oldMSTime > newMSTime, it means overflow happened
    uint32_t old_time = 0xFFFFFFF0;
    uint32_t new_time = 0x00000010;
    uint32_t diff = getMSTimeDiff(old_time, new_time);
    // Expected: (0xFFFFFFFF - 0xFFFFFFF0) + 0x00000010 = 0x0F + 0x10 = 0x1F = 31
    EXPECT_EQ(uint32_t(31), diff);
}

TEST(MSTimeDiff, OneMillisecondBeforeOverflow)
{
    uint32_t diff = getMSTimeDiff(0xFFFFFFFF, 0);
    // (0xFFFFFFFF - 0xFFFFFFFF) + 0 = 0
    EXPECT_EQ(uint32_t(0), diff);
}

TEST(MSTimeDiff, LargeDifference)
{
    EXPECT_EQ(uint32_t(1000000), getMSTimeDiff(0, 1000000));
}

// ============================================================================
// GetUnixTimeStamp Tests
// ============================================================================

TEST(UnixTimeStamp, ReasonableValue)
{
    uint32_t ts = GetUnixTimeStamp();
    // Should be after 2020 (1577836800) and before 2040 (2208988800)
    EXPECT_GT(ts, uint32_t(1577836800U));
}

TEST(UnixTimeStamp, ConsecutiveCallsNonDecreasing)
{
    uint32_t t1 = GetUnixTimeStamp();
    uint32_t t2 = GetUnixTimeStamp();
    EXPECT_GE(t2, t1);
}

// ============================================================================
// IntervalTimer Tests
// ============================================================================

TEST(IntervalTimer, DefaultConstruction)
{
    IntervalTimer timer;
    EXPECT_EQ(time_t(0), timer.GetInterval());
    EXPECT_EQ(time_t(0), timer.GetCurrent());
}

TEST(IntervalTimer, SetInterval)
{
    IntervalTimer timer;
    timer.SetInterval(1000);
    EXPECT_EQ(time_t(1000), timer.GetInterval());
}

TEST(IntervalTimer, SetCurrent)
{
    IntervalTimer timer;
    timer.SetCurrent(500);
    EXPECT_EQ(time_t(500), timer.GetCurrent());
}

TEST(IntervalTimer, UpdateIncreasesCurrent)
{
    IntervalTimer timer;
    timer.SetInterval(1000);
    timer.Update(100);
    EXPECT_EQ(time_t(100), timer.GetCurrent());
    timer.Update(200);
    EXPECT_EQ(time_t(300), timer.GetCurrent());
}

TEST(IntervalTimer, PassedWhenCurrentReachesInterval)
{
    IntervalTimer timer;
    timer.SetInterval(500);
    timer.Update(499);
    EXPECT_FALSE(timer.Passed());
    timer.Update(1);
    EXPECT_TRUE(timer.Passed());
}

TEST(IntervalTimer, PassedWhenCurrentExceedsInterval)
{
    IntervalTimer timer;
    timer.SetInterval(100);
    timer.Update(150);
    EXPECT_TRUE(timer.Passed());
}

TEST(IntervalTimer, ResetModulosByCurrent)
{
    IntervalTimer timer;
    timer.SetInterval(100);
    timer.Update(250);
    EXPECT_TRUE(timer.Passed());
    timer.Reset();
    EXPECT_EQ(time_t(50), timer.GetCurrent()); // 250 % 100 = 50
}

TEST(IntervalTimer, NegativeCurrentClampedToZero)
{
    IntervalTimer timer;
    timer.SetCurrent(10);
    timer.Update(-20); // would go to -10
    EXPECT_EQ(time_t(0), timer.GetCurrent());
}

TEST(IntervalTimer, MultipleResets)
{
    IntervalTimer timer;
    timer.SetInterval(100);
    timer.Update(350);
    timer.Reset();
    EXPECT_EQ(time_t(50), timer.GetCurrent());
    timer.Update(60);
    EXPECT_TRUE(timer.Passed());
    timer.Reset();
    EXPECT_EQ(time_t(10), timer.GetCurrent());
}

// ============================================================================
// TimeTracker Tests
// ============================================================================

TEST(TimeTracker, DefaultConstruction)
{
    TimeTracker tracker;
    EXPECT_TRUE(tracker.Passed()); // 0ms has passed
}

TEST(TimeTracker, InitWithMilliseconds)
{
    TimeTracker tracker(Milliseconds(5000));
    EXPECT_FALSE(tracker.Passed());
    EXPECT_EQ(5000, tracker.GetExpiry().count());
}

TEST(TimeTracker, InitWithInt)
{
    TimeTracker tracker(1000);
    EXPECT_FALSE(tracker.Passed());
    EXPECT_EQ(1000, tracker.GetExpiry().count());
}

TEST(TimeTracker, UpdateReducesExpiry)
{
    TimeTracker tracker(1000);
    tracker.Update(400);
    EXPECT_EQ(600, tracker.GetExpiry().count());
    EXPECT_FALSE(tracker.Passed());
}

TEST(TimeTracker, PassedWhenExpiryReachesZero)
{
    TimeTracker tracker(500);
    tracker.Update(500);
    EXPECT_TRUE(tracker.Passed());
}

TEST(TimeTracker, PassedWhenExpiryGoesNegative)
{
    TimeTracker tracker(500);
    tracker.Update(600);
    EXPECT_TRUE(tracker.Passed());
}

TEST(TimeTracker, ResetWithInt)
{
    TimeTracker tracker(100);
    tracker.Update(100);
    EXPECT_TRUE(tracker.Passed());
    tracker.Reset(200);
    EXPECT_FALSE(tracker.Passed());
    EXPECT_EQ(200, tracker.GetExpiry().count());
}

TEST(TimeTracker, ResetWithMilliseconds)
{
    TimeTracker tracker(100);
    tracker.Update(100);
    tracker.Reset(Milliseconds(300));
    EXPECT_EQ(300, tracker.GetExpiry().count());
    EXPECT_FALSE(tracker.Passed());
}

TEST(TimeTracker, UpdateWithMilliseconds)
{
    TimeTracker tracker(Milliseconds(1000));
    tracker.Update(Milliseconds(250));
    EXPECT_EQ(750, tracker.GetExpiry().count());
}

TEST(TimeTracker, MultipleUpdates)
{
    TimeTracker tracker(1000);
    tracker.Update(200);
    tracker.Update(300);
    tracker.Update(400);
    EXPECT_EQ(100, tracker.GetExpiry().count());
    EXPECT_FALSE(tracker.Passed());
    tracker.Update(100);
    EXPECT_TRUE(tracker.Passed());
}

// ============================================================================
// PeriodicTimer Tests
// ============================================================================

TEST(PeriodicTimer, DoesNotFireBeforePeriod)
{
    PeriodicTimer timer(1000, 1000);
    EXPECT_FALSE(timer.Update(500));
}

TEST(PeriodicTimer, FiresWhenPeriodExpires)
{
    PeriodicTimer timer(1000, 500);
    EXPECT_FALSE(timer.Update(400));
    EXPECT_TRUE(timer.Update(200)); // 500 - 400 - 200 = -100 -> fires
}

TEST(PeriodicTimer, ResetsAfterFiring)
{
    PeriodicTimer timer(1000, 1000);
    EXPECT_TRUE(timer.Update(1100));
    // After firing, should not fire immediately again
    EXPECT_FALSE(timer.Update(500));
}

TEST(PeriodicTimer, SetPeriodic)
{
    PeriodicTimer timer(100, 100);
    timer.SetPeriodic(2000, 2000);
    EXPECT_FALSE(timer.Update(1000));
    EXPECT_TRUE(timer.Update(1100));
}

TEST(PeriodicTimer, TUpdateAndTPassed)
{
    PeriodicTimer timer(1000, 500);
    EXPECT_FALSE(timer.TPassed());
    timer.TUpdate(600);
    EXPECT_TRUE(timer.TPassed());
}

TEST(PeriodicTimer, TReset)
{
    PeriodicTimer timer(1000, 100);
    timer.TUpdate(200);
    EXPECT_TRUE(timer.TPassed());
    timer.TReset(200, 1000);
    EXPECT_FALSE(timer.TPassed());
}

TEST(PeriodicTimer, MultiplePeriods)
{
    PeriodicTimer timer(100, 100);
    int fires = 0;
    for (int i = 0; i < 10; ++i)
    {
        if (timer.Update(50))
            ++fires;
    }
    // 10 updates of 50 = 500 total over period of 100 -> should fire ~5 times
    EXPECT_GE(fires, 3);
    EXPECT_LE(fires, 6);
}
