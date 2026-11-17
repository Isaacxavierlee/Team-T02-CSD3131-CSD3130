#pragma once

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace test {

inline void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Action> void rejects(Action action, const std::string& message) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

template <class Suite> int run(Suite suite) {
    try {
        suite();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

} // namespace test
