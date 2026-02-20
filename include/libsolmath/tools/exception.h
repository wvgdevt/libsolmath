/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <libsolmath/tools/format.h>
#include <string>
#include <exception>
#include <source_location>

#define DEFINE_EXCEPTION()                                                           \
struct exception : virtual std::exception                                            \
{                                                                                    \
    exception(exception&&) noexcept = default;                                       \
    exception(const exception&) noexcept = delete;                                   \
                                                                                     \
    explicit exception(std::string _message = std::string(),                         \
              std::source_location _l = std::source_location::current()) noexcept    \
        : m_message(std::move(_message)), m_location(_l) {}                          \
                                                                                     \
    const char* what() const noexcept override {                                     \
        if (m_message.empty())                                                       \
            return std::exception::what();                                           \
        m_what_cache = math::format(                                                  \
                    "{}:{} [{}]\n{}",                                                \
                    m_location.file_name(),                                          \
                    m_location.line(),                                               \
                    m_location.function_name(),                                      \
                    m_message                                                        \
                );                                                                   \
        return m_what_cache.c_str();                                                 \
    }                                                                                \
                                                                                     \
    const std::source_location& location() const noexcept { return m_location; }     \
                                                                                     \
protected:                                                                           \
    std::string m_message;                                                           \
    std::source_location m_location;                                                 \
    mutable std::string m_what_cache;                                                \
};

#define DEFINE_EXCEPTION_OPERATOR(EXCEPTION)                                         \
    EXCEPTION& operator<<(std::string const& _what)                                  \
    {                                                                                \
        m_message += _what;                                                          \
        return *this;                                                                \
    }

#ifdef NDEBUG
#define ASSERT(COND, EXCEPTION, MSG) do { } while (0)
#else
#define ASSERT(COND, EXCEPTION, MSG)                          \
do {                                                          \
    if (!(COND)) {                                            \
        throw (EXCEPTION((MSG)));                             \
    }                                                         \
} while (0)
#endif

#define THROW(EXCEPTION, MSG)  \
    throw (EXCEPTION((MSG)));
