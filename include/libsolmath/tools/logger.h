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

namespace sol::math {
namespace dc::internal {
    class DebugChannel {
    public:
        virtual ~DebugChannel() = default;

        std::size_t to_sizet() const { return m_num; } // NOLINT
        [[nodiscard]] virtual std::string const& to_string() const = 0;

    protected:
        DebugChannel();

    private:
        DebugChannel(const DebugChannel&)            = delete; // NOLINT
        DebugChannel& operator=(const DebugChannel&) = delete; // NOLINT
        static size_t num;
        size_t m_num;
    };
}

struct log_topic {
    unsigned long id = 0;
};

///
/// \brief The logger class
/// Allows to track log messages with custom debug channels defined from any lib
///
class logger // NOLINT
{
public:
    static logger& get();
    void log(dc::internal::DebugChannel const&,
             log_topic const& _topic = log_topic(),
             std::string const& _msg = std::string());
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
    static void write(size_t _level, std::string const&);
    static std::string const& log();
    static void flush();

private:
    [[nodiscard]] std::string const& _log() const;
    void _write(std::string const&);
    void _flush() { m_log = ""; }
    tracer() {}; // NOLINT
    std::string m_log;
};

#define DEFINE_LOG_CHANNEL(CHANNEL_NAME, CHANNEL_STR)                                           \
    namespace sol::math::dc {                                                                   \
    namespace internal {                                                                        \
    class CHANNEL_NAME##_decl : public sol::math::dc::internal::DebugChannel                    \
    {                                                                                           \
    public:                                                                                     \
        CHANNEL_NAME##_decl() : DebugChannel() {};                                              \
        std::string const& to_string() const override { return m_name; }                        \
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
    sol::math::logger::get().set_console_topic(TOPIC)
#define LOGT(CHANNEL, TOPIC, MESSAGE)                                                           \
    sol::math::logger::get().log(CHANNEL, TOPIC, MESSAGE);
#define LOG(CHANNEL, MESSAGE)                                                                   \
    sol::math::logger::get().log(CHANNEL, { .id = 1 }, MESSAGE);
}
