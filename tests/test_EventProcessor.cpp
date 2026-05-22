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
#include <queue>
#include <functional>
#include <algorithm>
#include <map>

// ============================================================================
// Standalone reimplementation of EventProcessor patterns from
// src/shared/Utilities/EventProcessor.h
// ============================================================================

class BasicEvent
{
public:
    virtual ~BasicEvent() {}
    virtual bool Execute(uint64_t /*e_time*/, uint32_t /*p_time*/) { return false; }
    virtual void Abort(uint64_t /*e_time*/) {}
    bool IsAborted() const { return m_aborted; }
    void SetAborted() { m_aborted = true; }
private:
    bool m_aborted = false;
};

class EventProcessor
{
public:
    ~EventProcessor()
    {
        KillAllEvents(true);
    }

    void Update(uint32_t p_time)
    {
        m_time += p_time;
        ProcessQueue();
    }

    void AddEvent(BasicEvent* event, uint64_t e_time, bool addToQueue = true)
    {
        if (addToQueue)
            m_events.insert(std::make_pair(e_time, event));
    }

    void KillAllEvents(bool force)
    {
        for (auto& pair : m_events)
        {
            if (force || !pair.second->IsAborted())
            {
                pair.second->Abort(m_time);
                delete pair.second;
            }
        }
        m_events.clear();
    }

    void KillEvent(BasicEvent* event)
    {
        event->SetAborted();
    }

    uint64_t CalculateTime(uint32_t t_offset) const
    {
        return m_time + t_offset;
    }

    size_t GetEventCount() const { return m_events.size(); }
    uint64_t GetTime() const { return m_time; }

private:
    void ProcessQueue()
    {
        auto it = m_events.begin();
        while (it != m_events.end())
        {
            if (it->first > m_time)
                break;

            BasicEvent* event = it->second;
            it = m_events.erase(it);

            if (!event->IsAborted())
            {
                if (event->Execute(m_time, 0))
                    delete event;
                else
                    delete event;
            }
            else
            {
                delete event;
            }
        }
    }

    std::multimap<uint64_t, BasicEvent*> m_events;
    uint64_t m_time = 0;
};

// Test event that tracks execution
class TestEvent : public BasicEvent
{
public:
    TestEvent(int& counter) : m_counter(counter) {}
    bool Execute(uint64_t, uint32_t) override { ++m_counter; return true; }
private:
    int& m_counter;
};

class AbortableEvent : public BasicEvent
{
public:
    AbortableEvent(int& execCount, int& abortCount)
        : m_execCount(execCount), m_abortCount(abortCount) {}
    bool Execute(uint64_t, uint32_t) override { ++m_execCount; return true; }
    void Abort(uint64_t) override { ++m_abortCount; }
private:
    int& m_execCount;
    int& m_abortCount;
};

// ============================================================================
// EventProcessor Tests
// ============================================================================

TEST(EventProcessor, InitialState)
{
    EventProcessor proc;
    EXPECT_EQ(size_t(0), proc.GetEventCount());
    EXPECT_EQ(uint64_t(0), proc.GetTime());
}

TEST(EventProcessor, AddEventIncreasesCount)
{
    EventProcessor proc;
    int counter = 0;
    proc.AddEvent(new TestEvent(counter), 100);
    EXPECT_EQ(size_t(1), proc.GetEventCount());
}

TEST(EventProcessor, EventNotExecutedBeforeTime)
{
    EventProcessor proc;
    int counter = 0;
    proc.AddEvent(new TestEvent(counter), 100);
    proc.Update(50);
    EXPECT_EQ(0, counter);
    EXPECT_EQ(size_t(1), proc.GetEventCount());
}

TEST(EventProcessor, EventExecutedAtTime)
{
    EventProcessor proc;
    int counter = 0;
    proc.AddEvent(new TestEvent(counter), 100);
    proc.Update(100);
    EXPECT_EQ(1, counter);
    EXPECT_EQ(size_t(0), proc.GetEventCount());
}

TEST(EventProcessor, EventExecutedAfterTime)
{
    EventProcessor proc;
    int counter = 0;
    proc.AddEvent(new TestEvent(counter), 50);
    proc.Update(100);
    EXPECT_EQ(1, counter);
}

TEST(EventProcessor, MultipleEventsOrdered)
{
    EventProcessor proc;
    int c1 = 0, c2 = 0, c3 = 0;
    proc.AddEvent(new TestEvent(c1), 10);
    proc.AddEvent(new TestEvent(c2), 20);
    proc.AddEvent(new TestEvent(c3), 30);

    proc.Update(15);
    EXPECT_EQ(1, c1);
    EXPECT_EQ(0, c2);
    EXPECT_EQ(0, c3);

    proc.Update(10); // time = 25
    EXPECT_EQ(1, c2);
    EXPECT_EQ(0, c3);

    proc.Update(10); // time = 35
    EXPECT_EQ(1, c3);
}

TEST(EventProcessor, KillEventPreventsExecution)
{
    EventProcessor proc;
    int execCount = 0, abortCount = 0;
    auto* event = new AbortableEvent(execCount, abortCount);
    proc.AddEvent(event, 100);
    proc.KillEvent(event);
    proc.Update(200);
    EXPECT_EQ(0, execCount);
    // Aborted event is cleaned up but Abort() is only called by KillAllEvents
}

TEST(EventProcessor, KillAllEventsAbortsAll)
{
    EventProcessor proc;
    int c1 = 0, c2 = 0;
    proc.AddEvent(new TestEvent(c1), 100);
    proc.AddEvent(new TestEvent(c2), 200);
    proc.KillAllEvents(true);
    EXPECT_EQ(size_t(0), proc.GetEventCount());
    proc.Update(300);
    EXPECT_EQ(0, c1);
    EXPECT_EQ(0, c2);
}

TEST(EventProcessor, CalculateTime)
{
    EventProcessor proc;
    proc.Update(100);
    EXPECT_EQ(uint64_t(150), proc.CalculateTime(50));
    EXPECT_EQ(uint64_t(200), proc.CalculateTime(100));
}

TEST(EventProcessor, UpdateAccumulatesTime)
{
    EventProcessor proc;
    proc.Update(10);
    proc.Update(20);
    proc.Update(30);
    EXPECT_EQ(uint64_t(60), proc.GetTime());
}

TEST(EventProcessor, MultipleEventsAtSameTime)
{
    EventProcessor proc;
    int c1 = 0, c2 = 0, c3 = 0;
    proc.AddEvent(new TestEvent(c1), 50);
    proc.AddEvent(new TestEvent(c2), 50);
    proc.AddEvent(new TestEvent(c3), 50);
    proc.Update(50);
    EXPECT_EQ(1, c1);
    EXPECT_EQ(1, c2);
    EXPECT_EQ(1, c3);
}

TEST(EventProcessor, AbortedEventField)
{
    BasicEvent event;
    EXPECT_FALSE(event.IsAborted());
    event.SetAborted();
    EXPECT_TRUE(event.IsAborted());
}

TEST(EventProcessor, ZeroTimeUpdate)
{
    EventProcessor proc;
    int counter = 0;
    proc.AddEvent(new TestEvent(counter), 0);
    proc.Update(0);
    EXPECT_EQ(1, counter);
}

TEST(EventProcessor, LargeTimeJump)
{
    EventProcessor proc;
    int c1 = 0, c2 = 0, c3 = 0;
    proc.AddEvent(new TestEvent(c1), 100);
    proc.AddEvent(new TestEvent(c2), 500);
    proc.AddEvent(new TestEvent(c3), 1000);
    proc.Update(2000);
    EXPECT_EQ(1, c1);
    EXPECT_EQ(1, c2);
    EXPECT_EQ(1, c3);
    EXPECT_EQ(size_t(0), proc.GetEventCount());
}
