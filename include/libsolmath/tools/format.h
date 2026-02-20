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

// GPT generated code for basic std::format support on android
namespace sol::math {
namespace internal {
    // Stream any type that supports operator<<
    template<class T>
    inline void stream_append(std::ostringstream& _os, T&& _v)
    {
        _os << std::forward<T>(_v);
    }

    inline void stream_format_into(std::ostringstream& _os, std::string_view const _fmt)
    {
        // No args left: copy the remainder; error if any unescaped {} remains.
        for (size_t i = 0; i < _fmt.size(); ++i)
        {
            char const c = _fmt[i];

            if (c == '{')
            {
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '{')
                { // escaped {
                    _os << '{';
                    ++i;
                    continue;
                }
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '}')
                {
                    throw std::runtime_error("math::format: not enough arguments for {}");
                }
                throw std::runtime_error("math::format: invalid '{' (use {} or escape as {{)");
            }

            if (c == '}')
            {
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '}')
                { // escaped }
                    _os << '}';
                    ++i;
                    continue;
                }
                throw std::runtime_error("math::format: stray '}' (escape as }})");
            }
            _os << c;
        }
    }

    template<class T, class... Ts>
    inline void stream_format_into(std::ostringstream& _os, std::string_view const _fmt, T&& _v, Ts&&... _rest)
    {
        for (size_t i = 0; i < _fmt.size(); ++i)
        {
            char c = _fmt[i];

            if (c == '{')
            {
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '{')
                { // escaped {
                    _os << '{';
                    ++i;
                    continue;
                }
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '}')
                {
                    // Found {} placeholder: write arg and recurse on the tail
                    internal::stream_append(_os, std::forward<T>(_v));
                    internal::stream_format_into(_os, _fmt.substr(i + 2), std::forward<Ts>(_rest)...);
                    return;
                }
                throw std::runtime_error("math::format: invalid '{' (use {} or escape as {{)");
            }

            if (c == '}')
            {
                if (i + 1 < _fmt.size() && _fmt[i + 1] == '}')
                { // escaped }
                    _os << '}';
                    ++i;
                    continue;
                }
                throw std::runtime_error("math::format: stray '}' (escape as }})");
            }

            _os << c;
        }

        // Format string ended but args remain
        throw std::runtime_error("math::format: too many arguments for format string");
    }
}

template<class... Ts>
inline std::string format(std::string_view _fmt, Ts&&... _args)
{
    std::ostringstream os;
    // If the stream fails, throw (helps debugging)
    os.exceptions(std::ios::badbit | std::ios::failbit);
    internal::stream_format_into(os, _fmt, std::forward<Ts>(_args)...);
    return os.str();
}
}
