#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace test {

using TestFn = void (*)();

struct TestCase {
    std::string name;
    TestFn fn;
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(std::string name, TestFn fn) {
        Registry().push_back({std::move(name), fn});
    }
};

inline void Fail(const char* file, int line, std::string_view expr) {
    std::cerr << file << ":" << line << ": assertion failed: " << expr << "\n";
    std::exit(1);
}

} // namespace test

#define TEST_CASE(name) \
    void name(); \
    static test::Registrar registrar_##name(#name, &name); \
    void name()

#define REQUIRE(expr) \
    do { \
        if (!(expr)) { \
            test::Fail(__FILE__, __LINE__, #expr); \
        } \
    } while (false)

