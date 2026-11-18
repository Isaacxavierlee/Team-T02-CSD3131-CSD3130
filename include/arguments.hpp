#pragma once

#include <charconv>
#include <stdexcept>
#include <string>

namespace seam_app {

inline int integer_option(const std::string& flag, const std::string& text, int minimum) {
    int value = 0;
    const char* end = text.data() + text.size();
    const auto parsed = std::from_chars(text.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end || value < minimum) {
        throw std::invalid_argument(flag + " expects a whole number of at least " +
                                    std::to_string(minimum) + ", got '" + text + "'");
    }
    return value;
}

} // namespace seam_app
