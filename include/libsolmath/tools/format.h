/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sol::math {
    // GPT generated code
// Stream any type that supports operator<<
    template <class T>
    inline void _append(std::ostringstream& os, T&& v) {
        os << std::forward<T>(v);
    }

    inline void _format_into(std::ostringstream& os, std::string_view fmt) {
        // No args left: copy the remainder; error if any unescaped {} remains.
        for (size_t i = 0; i < fmt.size(); ++i) {
            char c = fmt[i];

            if (c == '{') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '{') { // escaped {
                    os << '{';
                    ++i;
                    continue;
                }
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
                    throw std::runtime_error("math::format: not enough arguments for {}");
                }
                throw std::runtime_error("math::format: invalid '{' (use {} or escape as {{)");
            }

            if (c == '}') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') { // escaped }
                    os << '}';
                    ++i;
                    continue;
                }
                throw std::runtime_error("math::format: stray '}' (escape as }})");
            }

            os << c;
        }
    }

    template <class T, class... Ts>
    inline void _format_into(std::ostringstream& os, std::string_view fmt, T&& v, Ts&&... rest) {
        for (size_t i = 0; i < fmt.size(); ++i) {
            char c = fmt[i];

            if (c == '{') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '{') { // escaped {
                    os << '{';
                    ++i;
                    continue;
                }
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
                    // Found {} placeholder: write arg and recurse on the tail
                    _append(os, std::forward<T>(v));
                    _format_into(os, fmt.substr(i + 2), std::forward<Ts>(rest)...);
                    return;
                }
                throw std::runtime_error("math::format: invalid '{' (use {} or escape as {{)");
            }

            if (c == '}') {
                if (i + 1 < fmt.size() && fmt[i + 1] == '}') { // escaped }
                    os << '}';
                    ++i;
                    continue;
                }
                throw std::runtime_error("math::format: stray '}' (escape as }})");
            }

            os << c;
        }

        // Format string ended but args remain
        throw std::runtime_error("math::format: too many arguments for format string");
    }

    template <class... Ts>
    inline std::string format(std::string_view fmt, Ts&&... args) {
        std::ostringstream os;
        // If stream fails, throw (helps debugging)
        os.exceptions(std::ios::badbit | std::ios::failbit);
        _format_into(os, fmt, std::forward<Ts>(args)...);
        return os.str();
    }
}
