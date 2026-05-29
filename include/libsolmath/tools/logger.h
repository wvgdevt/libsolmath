/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <string>
#include <map>
#include <deque>
#include <set>
#include <fmt/format.h>

namespace sol::math {
namespace dc::internal {
    class DebugChannel {
    public:
        virtual ~DebugChannel() = default;

        std::size_t to_sizet() const { return m_num; } // NOLINT
        [[nodiscard]] virtual std::string const& as_string() const = 0;

    protected:
        DebugChannel();

    private:
        DebugChannel(const DebugChannel&)            = delete; // NOLINT
        DebugChannel& operator=(const DebugChannel&) = delete; // NOLINT
        static size_t num;                                     // NOLINT
        size_t m_num;
    };
}

struct log_topic {
    int id = 0;
};

///
/// \brief The logger class
/// Allows tracking log messages with custom debug channels defined from any lib
///
class logger // NOLINT
{
public:
    static logger& get();

    template<class... Args>
    void log_formated(dc::internal::DebugChannel const& _channel,
                      log_topic const& _topic,
                      fmt::format_string<Args...> _fmt,
                      Args&&... _args)
    {
        log(_channel, _topic, fmt::format(_fmt, std::forward<Args>(_args)...));
    }

    void log(dc::internal::DebugChannel const&,
             log_topic const& _topic = log_topic(),
             std::string_view _msg   = {});
    void filter_out(const dc::internal::DebugChannel&);
    void set_console_topic(log_topic const& _topic);
    void update(float);
    std::deque<std::string> const& messages();

private:
    typedef std::deque<std::string> Messages;
    std::map<size_t, Messages> m_log;
    Messages m_last_messages;
    std::set<size_t> m_filter;
    float m_remove_timer = 0;

    log_topic m_console_filter_topic;

private:
    logger()                         = default;
    ~logger()                        = default;
    logger(const logger&)            = delete; // NOLINT
    logger& operator=(const logger&) = delete; // NOLINT
};

#if NDEBUG
#define DEBUG_ONLY(code) ((void)0)
#else
#define DEBUG_ONLY(code) do { code; } while(0)
#endif

class tracer {
public:
    static tracer& get();
    static void write(size_t _level, std::string_view);
    static std::string const& log();
    static void flush();

private:
    [[nodiscard]] std::string const& _log() const;
    void _write(std::string_view);
    void _flush() { m_log = ""; }
    tracer() { m_log.reserve(16 * 1024); }; // NOLINT
    std::string m_log;
};

class function_logger {
public:
    explicit function_logger();
    explicit function_logger(char const* _name);

    template<class... Args>
    static void add_note(fmt::format_string<Args...> _fmt, Args&&... _args)
    {
        add_note_string(fmt::format(_fmt, std::forward<Args>(_args)...));
    }

    static void add_note_string(std::string_view _msg);

    static void increment_log_depth();
    static void decrement_log_depth();
    ~function_logger();

private:
    static inline thread_local size_t s_depth = 0; // NOLINT
    char const* m_name;
};

#ifdef NDEBUG
#define LOG_FUNCTION_SCOPE() ((void)0)
#define LOG_FUNCTION_SCOPE_SILENT() ((void)0)
#else
#define LOG_FUNCTION_SCOPE() \
sol::math::function_logger function_logger_scope_{__PRETTY_FUNCTION__}
#define LOG_FUNCTION_SCOPE_SILENT() \
sol::math::function_logger function_logger_scope_{}
#endif

#define LOG_FUNCTION_ADD_NOTE(...) \
    DEBUG_ONLY(sol::math::function_logger::add_note(__VA_ARGS__););

#define DEFINE_LOG_CHANNEL(CHANNEL_NAME, CHANNEL_STR)                                           \
    namespace sol::math::dc {                                                                   \
    namespace internal {                                                                        \
    class CHANNEL_NAME##_decl : public sol::math::dc::internal::DebugChannel                    \
    {                                                                                           \
    public:                                                                                     \
        CHANNEL_NAME##_decl() : DebugChannel() {};                                              \
        std::string const& as_string() const override { return m_name; }                        \
    private:                                                                                    \
        std::string const m_name = CHANNEL_STR;                                                 \
    };                                                                                          \
    }                                                                                           \
    extern internal::CHANNEL_NAME##_decl CHANNEL_NAME;                                          \
    }

#define INIT_LOG_CHANNEL(CHANNEL_NAME)                                                          \
    namespace sol::math::dc {                                                                   \
    sol::math::dc::internal::CHANNEL_NAME##_decl CHANNEL_NAME;                                  \
    }

#define LOG_TOPIC(TOPIC)                                                                        \
    DEBUG_ONLY(sol::math::logger::get().set_console_topic(TOPIC););
#define LOGT(CHANNEL, TOPIC, ...)                                                               \
    DEBUG_ONLY(sol::math::logger::get().log_formated(CHANNEL, TOPIC, __VA_ARGS__););
#define LOG(CHANNEL, ...)                                                                       \
    DEBUG_ONLY(sol::math::logger::get().log_formated(CHANNEL, { .id = 1 }, __VA_ARGS__););
}
