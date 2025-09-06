#pragma once
#include <exception>
#include <source_location>
#include <string>

#define DEFINE_EXCEPTION()                                                           \
struct exception : virtual std::exception                                            \
{                                                                                    \
    exception(exception&&) noexcept = default;                                       \
    exception(const exception&) noexcept = delete;                                   \
                                                                                     \
    exception(std::string    _message = std::string(),                               \
              std::source_location _l = std::source_location::current()) noexcept    \
        : m_message(std::move(_message)), m_location(_l) {}                          \
                                                                                     \
    const char* what() const noexcept override {                                     \
        return m_message.empty() ? std::exception::what() : m_message.c_str();       \
    }                                                                                \
                                                                                     \
    const std::source_location& location() const noexcept { return m_location; }     \
                                                                                     \
protected:                                                                           \
    std::string m_message;                                                           \
    std::source_location m_location;                                                 \
};

#define DEFINE_EXCEPTION_OPERATOR(EXCEPTION)                                         \
    EXCEPTION& operator<<(std::string const& _what)                                  \
    {                                                                                \
        m_message += _what;                                                          \
        return *this;                                                                \
    }
