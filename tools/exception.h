#pragma once
#include <exception>
#include <string>

#define DEFINE_EXCEPTION()                                                           \
struct exception : virtual std::exception                                            \
{                                                                                    \
    exception(exception&&) noexcept = default;                                       \
    exception(const exception&) noexcept = delete;                                   \
                                                                                     \
    exception(std::string _message = std::string()) noexcept                         \
        : m_message(std::move(_message)) {}                                          \
                                                                                     \
    const char* what() const noexcept override {                                     \
        return m_message.empty() ? std::exception::what() : m_message.c_str();       \
    }                                                                                \
                                                                                     \
                                                                                     \
protected:                                                                           \
    std::string m_message;                                                           \
};

#define DEFINE_EXCEPTION_OPERATOR(EXCEPTION)                                         \
    EXCEPTION& operator<<(std::string const& _what)                                  \
    {                                                                                \
        m_message += _what;                                                          \
        return *this;                                                                \
    }
