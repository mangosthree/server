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
 */

#ifndef MANGOS_TEST_FRAMEWORK_H
#define MANGOS_TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <sstream>
#include <cstring>
#include <chrono>

namespace MaNGOSTest
{

struct TestResult
{
    std::string testSuite;
    std::string testName;
    bool passed;
    std::string failureMessage;
    std::string file;
    int line;
};

class TestRegistry
{
public:
    struct TestEntry
    {
        std::string suite;
        std::string name;
        std::function<void()> func;
    };

    static TestRegistry& instance()
    {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& suite, const std::string& name, std::function<void()> func)
    {
        m_tests.push_back({suite, name, func});
    }

    int runAll()
    {
        int passed = 0;
        int failed = 0;
        int total = 0;

        auto startTime = std::chrono::steady_clock::now();

        std::cout << "========================================" << std::endl;
        std::cout << "  MaNGOS Server Test Suite" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << std::endl;

        std::string currentSuite;

        for (auto& test : m_tests)
        {
            if (test.suite != currentSuite)
            {
                if (!currentSuite.empty())
                    std::cout << std::endl;
                currentSuite = test.suite;
                std::cout << "--- " << currentSuite << " ---" << std::endl;
            }

            ++total;
            m_currentFailure.clear();
            m_currentFailed = false;

            try
            {
                test.func();
            }
            catch (const std::exception& e)
            {
                m_currentFailed = true;
                m_currentFailure = std::string("Exception: ") + e.what();
            }
            catch (...)
            {
                m_currentFailed = true;
                m_currentFailure = "Unknown exception";
            }

            if (m_currentFailed)
            {
                ++failed;
                std::cout << "  [FAIL] " << test.name << std::endl;
                if (!m_currentFailure.empty())
                    std::cout << "         " << m_currentFailure << std::endl;
                m_results.push_back({test.suite, test.name, false, m_currentFailure, "", 0});
            }
            else
            {
                ++passed;
                std::cout << "  [PASS] " << test.name << std::endl;
                m_results.push_back({test.suite, test.name, true, "", "", 0});
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Results: " << passed << " passed, " << failed << " failed, " << total << " total" << std::endl;
        std::cout << "  Time:    " << duration.count() << "ms" << std::endl;
        std::cout << "========================================" << std::endl;

        if (failed > 0)
        {
            std::cout << std::endl << "Failed tests:" << std::endl;
            for (auto& r : m_results)
            {
                if (!r.passed)
                    std::cout << "  " << r.testSuite << "::" << r.testName
                              << " - " << r.failureMessage << std::endl;
            }
        }

        return failed;
    }

    void fail(const std::string& msg)
    {
        m_currentFailed = true;
        m_currentFailure = msg;
    }

    bool hasFailed() const { return m_currentFailed; }

private:
    TestRegistry() : m_currentFailed(false) {}
    std::vector<TestEntry> m_tests;
    std::vector<TestResult> m_results;
    bool m_currentFailed;
    std::string m_currentFailure;
};

struct TestRegistrar
{
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void()> func)
    {
        TestRegistry::instance().addTest(suite, name, func);
    }
};

} // namespace MaNGOSTest

// Macros for defining tests
#define TEST_SUITE(suite) namespace TestSuite_##suite

#define TEST(suite, name)                                                           \
    static void TestFunc_##suite##_##name();                                         \
    static MaNGOSTest::TestRegistrar TestReg_##suite##_##name(                       \
        #suite, #name, TestFunc_##suite##_##name);                                   \
    static void TestFunc_##suite##_##name()

// Assertion macros
#define EXPECT_TRUE(expr)                                                           \
    do {                                                                            \
        if (!(expr)) {                                                              \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_TRUE(" #expr ") failed"; \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_FALSE(expr)                                                          \
    do {                                                                            \
        if ((expr)) {                                                               \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_FALSE(" #expr ") failed";\
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_EQ(expected, actual)                                                 \
    do {                                                                            \
        auto _e = (expected);                                                       \
        auto _a = (actual);                                                         \
        if (_e != _a) {                                                             \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_EQ(" #expected ", " #actual ")" \
                << " expected=" << _e << " actual=" << _a;                          \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_NE(val1, val2)                                                       \
    do {                                                                            \
        auto _v1 = (val1);                                                          \
        auto _v2 = (val2);                                                          \
        if (_v1 == _v2) {                                                           \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_NE(" #val1 ", " #val2 ")" \
                << " both=" << _v1;                                                 \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_LT(val1, val2)                                                       \
    do {                                                                            \
        auto _v1 = (val1);                                                          \
        auto _v2 = (val2);                                                          \
        if (!(_v1 < _v2)) {                                                         \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_LT(" #val1 ", " #val2 ")" \
                << " v1=" << _v1 << " v2=" << _v2;                                 \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_LE(val1, val2)                                                       \
    do {                                                                            \
        auto _v1 = (val1);                                                          \
        auto _v2 = (val2);                                                          \
        if (!(_v1 <= _v2)) {                                                        \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_LE(" #val1 ", " #val2 ")" \
                << " v1=" << _v1 << " v2=" << _v2;                                 \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_GT(val1, val2)                                                       \
    do {                                                                            \
        auto _v1 = (val1);                                                          \
        auto _v2 = (val2);                                                          \
        if (!(_v1 > _v2)) {                                                         \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_GT(" #val1 ", " #val2 ")" \
                << " v1=" << _v1 << " v2=" << _v2;                                 \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_GE(val1, val2)                                                       \
    do {                                                                            \
        auto _v1 = (val1);                                                          \
        auto _v2 = (val2);                                                          \
        if (!(_v1 >= _v2)) {                                                        \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_GE(" #val1 ", " #val2 ")" \
                << " v1=" << _v1 << " v2=" << _v2;                                 \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_FLOAT_EQ(expected, actual)                                           \
    do {                                                                            \
        float _e = (expected);                                                      \
        float _a = (actual);                                                        \
        if (std::fabs(_e - _a) > 0.0001f) {                                        \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_FLOAT_EQ(" #expected ", " #actual ")" \
                << " expected=" << _e << " actual=" << _a;                          \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_DOUBLE_EQ(expected, actual)                                          \
    do {                                                                            \
        double _e = (expected);                                                     \
        double _a = (actual);                                                       \
        if (std::fabs(_e - _a) > 0.00001) {                                        \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_DOUBLE_EQ(" #expected ", " #actual ")" \
                << " expected=" << _e << " actual=" << _a;                          \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_NEAR(expected, actual, tolerance)                                    \
    do {                                                                            \
        auto _e = (expected);                                                       \
        auto _a = (actual);                                                         \
        auto _t = (tolerance);                                                      \
        if (std::fabs(_e - _a) > _t) {                                             \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_NEAR(" #expected ", " #actual ", " #tolerance ")" \
                << " expected=" << _e << " actual=" << _a << " tolerance=" << _t;  \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_STR_EQ(expected, actual)                                             \
    do {                                                                            \
        std::string _e = (expected);                                                \
        std::string _a = (actual);                                                  \
        if (_e != _a) {                                                             \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_STR_EQ(" #expected ", " #actual ")" \
                << " expected=\"" << _e << "\" actual=\"" << _a << "\"";            \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_THROW(expr, exType)                                                  \
    do {                                                                            \
        bool _caught = false;                                                       \
        try { expr; } catch (const exType&) { _caught = true; } catch (...) {}      \
        if (!_caught) {                                                             \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_THROW(" #expr ", " #exType ") - no exception thrown"; \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#define EXPECT_NO_THROW(expr)                                                       \
    do {                                                                            \
        try { expr; } catch (...) {                                                 \
            std::ostringstream oss;                                                 \
            oss << __FILE__ << ":" << __LINE__ << " EXPECT_NO_THROW(" #expr ") - exception thrown"; \
            MaNGOSTest::TestRegistry::instance().fail(oss.str());                   \
            return;                                                                 \
        }                                                                           \
    } while(0)

#endif // MANGOS_TEST_FRAMEWORK_H
