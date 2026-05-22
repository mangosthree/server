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

/**
 * Database Stress Tests
 *
 * These tests exercise the database-adjacent logic: connection string parsing,
 * query formatting, escape routines, and simulated concurrent query patterns.
 * They do NOT require a live MariaDB instance — they test the C++ logic that
 * wraps the DB layer. For live DB stress tests, see test_mariadb_stress.py.
 */

#include "TestFramework.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <random>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

// ═══════════════════════════════════════════════════════════════════════════════
// Simulated DB connection string parser (mirrors DatabaseMysql.cpp logic)
// ═══════════════════════════════════════════════════════════════════════════════

struct DbConnParams
{
    std::string host;
    std::string port;
    std::string user;
    std::string password;
    std::string database;
};

static std::vector<std::string> StrSplit(const std::string& src, const std::string& sep)
{
    std::vector<std::string> result;
    std::string::size_type start = 0;
    std::string::size_type end = src.find(sep, start);
    while (end != std::string::npos)
    {
        result.push_back(src.substr(start, end - start));
        start = end + sep.size();
        end = src.find(sep, start);
    }
    result.push_back(src.substr(start));
    return result;
}

static DbConnParams ParseConnString(const std::string& info)
{
    DbConnParams p;
    auto tokens = StrSplit(info, ";");
    auto it = tokens.begin();
    if (it != tokens.end()) p.host = *it++;
    if (it != tokens.end()) p.port = *it++;
    if (it != tokens.end()) p.user = *it++;
    if (it != tokens.end()) p.password = *it++;
    if (it != tokens.end()) p.database = *it++;
    return p;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Simulated SQL escape (mirrors mysql_real_escape_string behavior)
// ═══════════════════════════════════════════════════════════════════════════════

static std::string EscapeString(const std::string& input)
{
    std::string result;
    result.reserve(input.size() * 2);
    for (char c : input)
    {
        switch (c)
        {
            case '\0': result += "\\0"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            case '\'': result += "\\'"; break;
            case '"':  result += "\\\""; break;
            case '\x1a': result += "\\Z"; break;
            default: result += c; break;
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Simulated query builder (mirrors PExecute-style formatting)
// ═══════════════════════════════════════════════════════════════════════════════

static std::string FormatQuery(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return std::string(buf);
}

// ═══════════════════════════════════════════════════════════════════════════════
// CONNECTION STRING PARSING TESTS
// ═══════════════════════════════════════════════════════════════════════════════

TEST(DatabaseStress, ConnString_Normal)
{
    auto p = ParseConnString("127.0.0.1;3306;mangos;mangos;realmd");
    EXPECT_STR_EQ("127.0.0.1", p.host);
    EXPECT_STR_EQ("3306", p.port);
    EXPECT_STR_EQ("mangos", p.user);
    EXPECT_STR_EQ("mangos", p.password);
    EXPECT_STR_EQ("realmd", p.database);
}

TEST(DatabaseStress, ConnString_Empty)
{
    auto p = ParseConnString("");
    EXPECT_STR_EQ("", p.host);
    EXPECT_STR_EQ("", p.port);
}

TEST(DatabaseStress, ConnString_PartialFields)
{
    auto p = ParseConnString("localhost;3306");
    EXPECT_STR_EQ("localhost", p.host);
    EXPECT_STR_EQ("3306", p.port);
    EXPECT_STR_EQ("", p.user);
    EXPECT_STR_EQ("", p.password);
    EXPECT_STR_EQ("", p.database);
}

TEST(DatabaseStress, ConnString_ExtraFields)
{
    auto p = ParseConnString("host;port;user;pass;db;extra;more");
    EXPECT_STR_EQ("host", p.host);
    EXPECT_STR_EQ("db", p.database);
}

TEST(DatabaseStress, ConnString_SocketPath)
{
    auto p = ParseConnString(".;/var/run/mysqld/mysqld.sock;mangos;mangos;realmd");
    EXPECT_STR_EQ(".", p.host);
    EXPECT_STR_EQ("/var/run/mysqld/mysqld.sock", p.port);
}

TEST(DatabaseStress, ConnString_SpecialCharsInPassword)
{
    auto p = ParseConnString("host;3306;user;p@$$w0rd!;db");
    EXPECT_STR_EQ("p@$$w0rd!", p.password);
}

TEST(DatabaseStress, ConnString_SemicolonsOnly)
{
    auto p = ParseConnString(";;;;");
    EXPECT_STR_EQ("", p.host);
    EXPECT_STR_EQ("", p.database);
}

TEST(DatabaseStress, ConnString_Concurrent_1000)
{
    std::atomic<int> failures{0};
    auto worker = [&]() {
        for (int i = 0; i < 1000; ++i)
        {
            auto p = ParseConnString("127.0.0.1;3306;mangos;mangos;realmd");
            if (p.host != "127.0.0.1" || p.database != "realmd")
                ++failures;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, failures.load());
}

// ═══════════════════════════════════════════════════════════════════════════════
// SQL ESCAPE / INJECTION PREVENTION TESTS
// ═══════════════════════════════════════════════════════════════════════════════

TEST(DatabaseStress, Escape_NullBytes)
{
    std::string input(1, '\0');
    auto escaped = EscapeString(input);
    EXPECT_STR_EQ("\\0", escaped);
}

TEST(DatabaseStress, Escape_SingleQuote)
{
    auto escaped = EscapeString("test'injection");
    EXPECT_STR_EQ("test\\'injection", escaped);
}

TEST(DatabaseStress, Escape_SQLi_DropTable)
{
    auto escaped = EscapeString("'; DROP TABLE account; --");
    // the single quote must be escaped — raw "'" should become "\\'"
    EXPECT_TRUE(escaped.find("\\'") != std::string::npos);
    // verify escaped version cannot terminate the SQL string
    EXPECT_TRUE(escaped.substr(0, 2) == "\\'");
}

TEST(DatabaseStress, Escape_SQLi_UnionSelect)
{
    auto escaped = EscapeString("' UNION SELECT * FROM account--");
    // the single quote must be escaped
    EXPECT_TRUE(escaped.find("\\'") != std::string::npos);
    EXPECT_TRUE(escaped.substr(0, 2) == "\\'");
}

TEST(DatabaseStress, Escape_Backslash)
{
    auto escaped = EscapeString("\\");
    EXPECT_STR_EQ("\\\\", escaped);
}

TEST(DatabaseStress, Escape_AllDangerousChars)
{
    std::string dangerous = "\0\n\r\\\'\"\x1a";
    auto escaped = EscapeString(dangerous);
    // verify no raw dangerous chars remain (except via escape sequences)
    for (char c : dangerous)
    {
        if (c == '\0') continue;
        // raw character should not appear as a standalone
        bool found_raw = false;
        for (size_t i = 0; i < escaped.size(); ++i)
        {
            if (escaped[i] == c && (i == 0 || escaped[i-1] != '\\'))
                found_raw = true;
        }
        EXPECT_FALSE(found_raw);
    }
}

TEST(DatabaseStress, Escape_LargePayload_1MB)
{
    std::string large(1024 * 1024, 'A');
    large[500000] = '\'';
    large[500001] = '\0';
    auto escaped = EscapeString(large);
    EXPECT_TRUE(escaped.size() >= large.size());
}

TEST(DatabaseStress, Escape_Concurrent_Stress)
{
    std::atomic<int> failures{0};
    auto worker = [&]() {
        std::mt19937 rng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        for (int i = 0; i < 10000; ++i)
        {
            std::string input = "test'value\"with\\special\0chars";
            auto escaped = EscapeString(input);
            if (escaped.find("test\\'value") == std::string::npos)
                ++failures;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, failures.load());
}

// ═══════════════════════════════════════════════════════════════════════════════
// QUERY FORMATTING STRESS
// ═══════════════════════════════════════════════════════════════════════════════

TEST(DatabaseStress, Format_BasicSelect)
{
    auto q = FormatQuery("SELECT * FROM account WHERE id = %u", 42u);
    EXPECT_TRUE(q.find("42") != std::string::npos);
}

TEST(DatabaseStress, Format_StringParam)
{
    auto q = FormatQuery("SELECT * FROM account WHERE username = '%s'", "TEST");
    EXPECT_TRUE(q.find("TEST") != std::string::npos);
}

TEST(DatabaseStress, Format_MaxValues)
{
    auto q = FormatQuery("UPDATE characters SET money = %u WHERE guid = %u",
                          4294967295u, 4294967295u);
    EXPECT_TRUE(q.find("4294967295") != std::string::npos);
}

TEST(DatabaseStress, Format_TruncationAtLimit)
{
    // Query longer than 1024 chars should be truncated safely
    char longName[512];
    memset(longName, 'A', 511);
    longName[511] = '\0';
    auto q = FormatQuery("SELECT * FROM account WHERE username = '%s' AND sha_pass_hash = '%s'",
                          longName, longName);
    EXPECT_TRUE(q.size() <= 1024);
}

TEST(DatabaseStress, Format_ConcurrentFormatting_10k)
{
    std::atomic<int> failures{0};
    auto worker = [&](int tid) {
        for (int i = 0; i < 10000; ++i)
        {
            auto q = FormatQuery("UPDATE account SET active_realm_id = %u WHERE id = %u", tid, i);
            if (q.empty())
                ++failures;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back(worker, t);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, failures.load());
}

// ═══════════════════════════════════════════════════════════════════════════════
// SIMULATED CONNECTION POOL STRESS
// ═══════════════════════════════════════════════════════════════════════════════

// Simulated pool: models the connection lifecycle without real MySQL
class MockConnectionPool
{
public:
    MockConnectionPool(int maxConns) : m_maxConns(maxConns), m_active(0), m_totalCreated(0) {}

    bool acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_active >= m_maxConns)
            return false;
        ++m_active;
        ++m_totalCreated;
        return true;
    }

    void release()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_active > 0)
            --m_active;
    }

    int active() const { return m_active; }
    int totalCreated() const { return m_totalCreated; }

private:
    int m_maxConns;
    int m_active;
    int m_totalCreated;
    mutable std::mutex m_mutex;
};

TEST(DatabaseStress, Pool_ExhaustAndRecover)
{
    MockConnectionPool pool(10);
    // exhaust
    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE(pool.acquire());
    EXPECT_FALSE(pool.acquire());  // 11th should fail

    // release one and re-acquire
    pool.release();
    EXPECT_TRUE(pool.acquire());
    EXPECT_EQ(10, pool.active());

    // cleanup
    for (int i = 0; i < 10; ++i)
        pool.release();
    EXPECT_EQ(0, pool.active());
}

TEST(DatabaseStress, Pool_ConcurrentAcquireRelease)
{
    MockConnectionPool pool(20);
    std::atomic<int> acquireFailures{0};

    auto worker = [&]() {
        for (int i = 0; i < 1000; ++i)
        {
            if (pool.acquire())
            {
                // simulate a query
                std::this_thread::yield();
                pool.release();
            }
            else
            {
                ++acquireFailures;
                std::this_thread::yield();
            }
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 50; ++t)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, pool.active());
    EXPECT_GT(pool.totalCreated(), 0);
}

TEST(DatabaseStress, Pool_Thundering_Herd)
{
    MockConnectionPool pool(5);
    std::atomic<int> successes{0};
    std::atomic<int> ready{0};

    auto worker = [&](int tid) {
        ++ready;
        while (ready.load() < 100)
            std::this_thread::yield();

        for (int i = 0; i < 100; ++i)
        {
            if (pool.acquire())
            {
                ++successes;
                std::this_thread::yield();
                pool.release();
            }
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 100; ++t)
        threads.emplace_back(worker, t);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, pool.active());
    EXPECT_GT(successes.load(), 0);
}

// ═══════════════════════════════════════════════════════════════════════════════
// SIMULATED ASYNC QUERY QUEUE STRESS
// ═══════════════════════════════════════════════════════════════════════════════

class MockQueryQueue
{
public:
    void push(const std::string& sql)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(sql);
    }

    std::string pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return "";
        auto front = m_queue.front();
        m_queue.erase(m_queue.begin());
        return front;
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::vector<std::string> m_queue;
    std::mutex m_mutex;
};

TEST(DatabaseStress, AsyncQueue_ProducerConsumer)
{
    MockQueryQueue queue;
    std::atomic<int> consumed{0};
    std::atomic<bool> producing{true};

    // 10 producers
    auto producer = [&](int tid) {
        for (int i = 0; i < 1000; ++i)
        {
            auto q = FormatQuery("UPDATE characters SET level = %d WHERE guid = %d", i, tid);
            queue.push(q);
        }
    };

    // 5 consumers
    auto consumer = [&]() {
        while (producing.load() || queue.size() > 0)
        {
            auto sql = queue.pop();
            if (!sql.empty())
                ++consumed;
            else
                std::this_thread::yield();
        }
    };

    std::vector<std::thread> producers, consumers;
    for (int t = 0; t < 10; ++t)
        producers.emplace_back(producer, t);
    for (int t = 0; t < 5; ++t)
        consumers.emplace_back(consumer);

    for (auto& t : producers)
        t.join();
    producing = false;
    for (auto& t : consumers)
        t.join();

    EXPECT_EQ(10000, consumed.load());
    EXPECT_EQ(0u, queue.size());
}

TEST(DatabaseStress, AsyncQueue_FloodThenDrain)
{
    MockQueryQueue queue;
    // flood with 100k queries
    for (int i = 0; i < 100000; ++i)
        queue.push(FormatQuery("SELECT %d", i));

    EXPECT_EQ(100000u, queue.size());

    // drain with 20 threads
    std::atomic<int> drained{0};
    auto drainer = [&]() {
        while (true)
        {
            auto sql = queue.pop();
            if (sql.empty()) break;
            ++drained;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 20; ++t)
        threads.emplace_back(drainer);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(100000, drained.load());
}

// ═══════════════════════════════════════════════════════════════════════════════
// TRANSACTION SIMULATION STRESS
// ═══════════════════════════════════════════════════════════════════════════════

class MockTransaction
{
public:
    MockTransaction() : m_active(false), m_committed(false), m_rolledBack(false) {}

    bool begin() { m_active = true; m_committed = false; m_rolledBack = false; return true; }
    bool commit() { if (!m_active) return false; m_active = false; m_committed = true; return true; }
    bool rollback() { if (!m_active) return false; m_active = false; m_rolledBack = true; return true; }
    bool isActive() const { return m_active; }
    bool wasCommitted() const { return m_committed; }
    bool wasRolledBack() const { return m_rolledBack; }

private:
    bool m_active, m_committed, m_rolledBack;
};

TEST(DatabaseStress, Transaction_CommitRollbackCycle_10k)
{
    MockTransaction txn;
    int commits = 0, rollbacks = 0;
    for (int i = 0; i < 10000; ++i)
    {
        txn.begin();
        EXPECT_TRUE(txn.isActive());
        if (i % 3 == 0)
        {
            txn.rollback();
            ++rollbacks;
            EXPECT_TRUE(txn.wasRolledBack());
        }
        else
        {
            txn.commit();
            ++commits;
            EXPECT_TRUE(txn.wasCommitted());
        }
        EXPECT_FALSE(txn.isActive());
    }
    EXPECT_GT(commits, 0);
    EXPECT_GT(rollbacks, 0);
}

TEST(DatabaseStress, Transaction_DoubleCommit)
{
    MockTransaction txn;
    txn.begin();
    EXPECT_TRUE(txn.commit());
    EXPECT_FALSE(txn.commit()); // should fail — already committed
}

TEST(DatabaseStress, Transaction_RollbackWithoutBegin)
{
    MockTransaction txn;
    EXPECT_FALSE(txn.rollback()); // no active transaction
}

// ═══════════════════════════════════════════════════════════════════════════════
// ACCOUNT NAME VALIDATION STRESS
// ═══════════════════════════════════════════════════════════════════════════════

static bool IsValidAccountName(const std::string& name)
{
    if (name.empty() || name.size() > 32) return false;
    for (unsigned char c : name)
    {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}

TEST(DatabaseStress, AccountName_ValidNames)
{
    EXPECT_TRUE(IsValidAccountName("TEST"));
    EXPECT_TRUE(IsValidAccountName("admin_user"));
    EXPECT_TRUE(IsValidAccountName("Player-1"));
    EXPECT_TRUE(IsValidAccountName("a"));
    EXPECT_TRUE(IsValidAccountName("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345"));
}

TEST(DatabaseStress, AccountName_InvalidNames)
{
    EXPECT_FALSE(IsValidAccountName(""));
    EXPECT_FALSE(IsValidAccountName(std::string(33, 'A')));
    EXPECT_FALSE(IsValidAccountName("user'inject"));
    EXPECT_FALSE(IsValidAccountName("user;drop"));
    // C++ string("user\x00null") is truncated to "user" at the null byte
    // Test with an explicit embedded null using string constructor
    EXPECT_FALSE(IsValidAccountName(std::string("user\x00null", 9)));
    EXPECT_FALSE(IsValidAccountName("user space"));
    EXPECT_FALSE(IsValidAccountName("tëst"));
    EXPECT_FALSE(IsValidAccountName("<script>"));
}

TEST(DatabaseStress, AccountName_Fuzz_100k)
{
    std::mt19937 rng(42);
    for (int i = 0; i < 100000; ++i)
    {
        int len = rng() % 64;
        std::string name;
        for (int j = 0; j < len; ++j)
            name += (char)(rng() % 256);
        // just verify it doesn't crash — result correctness is secondary
        (void)IsValidAccountName(name);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// GUID GENERATION STRESS
// ═══════════════════════════════════════════════════════════════════════════════

class MockGuidGenerator
{
public:
    MockGuidGenerator() : m_next(1) {}

    uint64_t generate()
    {
        return m_next.fetch_add(1);
    }

private:
    std::atomic<uint64_t> m_next;
};

TEST(DatabaseStress, Guid_ConcurrentGeneration_Unique)
{
    MockGuidGenerator gen;
    std::mutex setMutex;
    std::set<uint64_t> allGuids;
    std::atomic<int> duplicates{0};

    auto worker = [&]() {
        std::vector<uint64_t> local;
        for (int i = 0; i < 10000; ++i)
            local.push_back(gen.generate());

        std::lock_guard<std::mutex> lock(setMutex);
        for (auto g : local)
        {
            if (!allGuids.insert(g).second)
                ++duplicates;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 20; ++t)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, duplicates.load());
    EXPECT_EQ(200000u, allGuids.size());
}

// ═══════════════════════════════════════════════════════════════════════════════
// QUERY RESULT MEMORY STRESS
// ═══════════════════════════════════════════════════════════════════════════════

struct MockQueryResult
{
    std::vector<std::vector<std::string>> rows;

    void addRow(std::vector<std::string> row) { rows.push_back(std::move(row)); }
    size_t rowCount() const { return rows.size(); }
    const std::string& getField(size_t row, size_t col) const { return rows[row][col]; }
};

TEST(DatabaseStress, QueryResult_LargeResultSet)
{
    MockQueryResult result;
    for (int i = 0; i < 50000; ++i)
    {
        result.addRow({"guid_" + std::to_string(i), "name_" + std::to_string(i),
                        std::to_string(i % 85 + 1)});
    }
    EXPECT_EQ(50000u, result.rowCount());
    EXPECT_STR_EQ("guid_0", result.getField(0, 0));
    EXPECT_STR_EQ("guid_49999", result.getField(49999, 0));
}

TEST(DatabaseStress, QueryResult_RapidCreateDestroy)
{
    for (int cycle = 0; cycle < 1000; ++cycle)
    {
        auto* result = new MockQueryResult();
        for (int i = 0; i < 100; ++i)
            result->addRow({"col1", "col2", "col3"});
        EXPECT_EQ(100u, result->rowCount());
        delete result;
    }
}

TEST(DatabaseStress, QueryResult_ConcurrentAccess)
{
    MockQueryResult result;
    for (int i = 0; i < 1000; ++i)
        result.addRow({std::to_string(i), "data"});

    std::atomic<int> errors{0};
    auto reader = [&]() {
        for (int i = 0; i < 10000; ++i)
        {
            size_t idx = i % result.rowCount();
            auto& val = result.getField(idx, 0);
            if (val != std::to_string(idx))
                ++errors;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t)
        threads.emplace_back(reader);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(0, errors.load());
}
