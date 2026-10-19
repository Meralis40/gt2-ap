// SPDX-License-Identifier: GPL-2.0-or-later
// Minimal test harness: TEST_CASE registers a test, CHECK records a failure and
// continues, REQUIRE aborts the current test.
#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace test {

struct Case {
    const char* name;
    std::function<void()> body;
};

std::vector<Case>& registry();
void fail(const char* file, int line, const std::string& message);

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

struct RequireFailed {};

} // namespace test

#define TEST_CONCAT_(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_(a, b)
#define TEST_CASE(name)                                                                       \
    static void TEST_CONCAT(test_fn_, __LINE__)();                                            \
    static const test::Registrar TEST_CONCAT(test_reg_, __LINE__)(name, &TEST_CONCAT(test_fn_, __LINE__)); \
    static void TEST_CONCAT(test_fn_, __LINE__)()

#define CHECK(expr)                                         \
    do {                                                    \
        if (!(expr))                                        \
            test::fail(__FILE__, __LINE__, "CHECK(" #expr ")"); \
    } while (0)

#define REQUIRE(expr)                                         \
    do {                                                      \
        if (!(expr)) {                                        \
            test::fail(__FILE__, __LINE__, "REQUIRE(" #expr ")"); \
            throw test::RequireFailed{};                      \
        }                                                     \
    } while (0)

#define CHECK_EQ(a, b)                                                         \
    do {                                                                       \
        const auto& test_a_ = (a);                                             \
        const auto& test_b_ = (b);                                             \
        if (!(test_a_ == test_b_)) {                                           \
            std::ostringstream test_os_;                                       \
            test_os_ << "CHECK_EQ(" #a ", " #b "): " << test_a_ << " != " << test_b_; \
            test::fail(__FILE__, __LINE__, test_os_.str());                    \
        }                                                                      \
    } while (0)

#define CHECK_THROWS(expr)                                                 \
    do {                                                                   \
        bool test_threw_ = false;                                          \
        try {                                                              \
            (void)(expr);                                                  \
        } catch (...) {                                                    \
            test_threw_ = true;                                            \
        }                                                                  \
        if (!test_threw_)                                                  \
            test::fail(__FILE__, __LINE__, "CHECK_THROWS(" #expr ")");     \
    } while (0)
