// SPDX-License-Identifier: GPL-2.0-or-later
#include "test.hpp"

#include <cstdio>
#include <exception>

namespace test {

namespace {
int g_failures = 0;
}

std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

void fail(const char* file, int line, const std::string& message)
{
    ++g_failures;
    std::fprintf(stderr, "  %s:%d: %s\n", file, line, message.c_str());
}

} // namespace test

int main()
{
    int failedCases = 0;
    for (const test::Case& c : test::registry()) {
        const int before = test::g_failures;
        try {
            c.body();
        } catch (const test::RequireFailed&) {
        } catch (const std::exception& e) {
            test::fail(__FILE__, __LINE__, std::string("unexpected exception: ") + e.what());
        }
        const bool ok = test::g_failures == before;
        failedCases += ok ? 0 : 1;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
    }
    std::printf("%zu tests, %d failed\n", test::registry().size(), failedCases);
    return failedCases == 0 ? 0 : 1;
}
