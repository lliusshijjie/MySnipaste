#include "test_framework.h"

#include <exception>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    const std::string filter = argc > 1 ? argv[1] : std::string{};
    std::ofstream progress("ScreenshotMvpTests.progress.txt", std::ios::trunc);
    int passed = 0;
    for (const auto& testCase : test::Registry()) {
        if (!filter.empty() && testCase.name.find(filter) == std::string::npos) {
            continue;
        }
        try {
            progress << "[RUN] " << testCase.name << "\n" << std::flush;
            std::cout << "[RUN] " << testCase.name << "\n" << std::flush;
            testCase.fn();
            ++passed;
            progress << "[PASS] " << testCase.name << "\n" << std::flush;
            std::cout << "[PASS] " << testCase.name << "\n" << std::flush;
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << testCase.name << ": " << ex.what() << "\n";
            return 1;
        } catch (...) {
            std::cerr << "[FAIL] " << testCase.name << ": unknown exception\n";
            return 1;
        }
    }

    std::cout << passed << " tests passed\n";
    return 0;
}
