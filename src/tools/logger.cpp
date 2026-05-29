/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#include "libsolmath/tools/logger.h"
#include <iostream>
#include <mutex>

#include "libsolmath/math.h"
#include "libsolmath/tools/timer.h"

namespace {
struct ConsoleColor {
    std::string_view start;
    std::string_view end;
};

struct LevelData {
    float total_time{};
    float last_time{};
    std::string padding;
};

ConsoleColor color_by_time(float const _time)
{
    if (_time > 1.f)
        return {"\033[31m", "\033[0m"}; // RED
    return {"", ""};
}

std::string prepare_message(float const _time, std::string_view const _padding, std::string_view const _message)
{
    auto const color = color_by_time(_time); // NOLINT
    auto const time  = sol::math::to_string(_time, 2);

    std::string line;
    line.reserve(_padding.size()
                 + color.start.size()
                 + 1 // "["
                 + time.size()
                 + 2 // "] "
                 + _message.size()
                 + color.end.size()
                 + 4 // extra
    );

    line.append(_padding);
    line.append(color.start);
    line += "[";
    line += time;
    line += "] ";
    line.append(_message);
    line.append(color.end);
    return line;
}

sol::math::timer g_time;

std::map<size_t, LevelData> g_level_time;
}

namespace sol::math {
std::mutex g_logger_lock;
size_t dc::internal::DebugChannel::num = 0;

dc::internal::DebugChannel::DebugChannel()
{
    m_num = ++num;
}

logger& logger::get()
{
    static logger log;
    return log;
}

void logger::filter_out(const dc::internal::DebugChannel& _channel)
{
    m_filter.emplace(_channel.to_sizet());
}

void logger::set_console_topic(log_topic const& _topic)
{
    m_console_filter_topic = _topic;
}

void logger::log(dc::internal::DebugChannel const& _channel, log_topic const& _topic, std::string_view const _msg)
{
    auto const channel_id = _channel.to_sizet();
    auto const& channel   = _channel.as_string();

    std::string message;
    message.reserve(channel.size() + _msg.size());
    message += channel;
    message.append(_msg);

    std::lock_guard<std::mutex> lock(g_logger_lock);
    if (m_console_filter_topic.id == 0 || m_console_filter_topic.id == _topic.id)
        std::cout << message << '\n';

    if (!m_filter.contains(channel_id))
    {
        m_last_messages.emplace_back(message);
        m_remove_timer = 0;

        if (m_last_messages.size() > 10)
            m_last_messages.pop_front();
    }

    m_log[channel_id].emplace_back(std::move(message));
}

void logger::update(const float _td)
{
    m_remove_timer += _td;
    if (m_remove_timer > 1000)
    {
        m_remove_timer = 0;
        if (!m_last_messages.empty())
            m_last_messages.pop_front();
    }
}

std::deque<std::string> const& logger::messages()
{
    return m_last_messages;
}

tracer& tracer::get()
{
    static tracer logger;
    return logger;
}

void tracer::write(size_t const _level, std::string_view const _message)
{
    static size_t current_level = 0;
    static bool has_current     = false;

    float const now = g_time.passed_milliseconds();

    if (has_current)
    {
        if (_level > current_level)
        {
            auto const previous = g_level_time.find(current_level);
            if (previous != g_level_time.end())
                previous->second.total_time += now - previous->second.last_time;
        }
        else if (_level < current_level)
        {
            for (size_t level = current_level; level > _level; --level)
            {
                auto previous = g_level_time.find(level);
                if (previous == g_level_time.end())
                    continue;
                auto& level_data = previous->second; // NOLINT

                level_data.total_time += now - level_data.last_time;
                get()._write(prepare_message(level_data.total_time, level_data.padding, "total time"));
                g_level_time.erase(previous);
            }
        }
    }

    auto [it, inserted] = g_level_time.try_emplace(_level);
    auto& line_data     = it->second; // NOLINT
    if (inserted)
    {
        line_data.padding    = std::string((_level + 1) * 2, '.');
        line_data.last_time  = now;
        line_data.total_time = 0;
    }

    float const time_diff = std::max(0.00f, now - line_data.last_time);
    if (!_message.empty())
        get()._write(prepare_message(time_diff, line_data.padding, _message));

    line_data.last_time  = now;
    line_data.total_time += time_diff;

    current_level = _level;
    has_current   = true;
}

std::string const& tracer::log()
{
    return get()._log();
}

void tracer::flush()
{
    g_time.reset();
    g_level_time.clear();
    get()._flush();
}

std::string const& tracer::_log() const
{
    return m_log;
}

void tracer::_write(std::string_view const _message)
{
    m_log.append(_message);
    m_log.push_back('\n');
}

function_logger::function_logger() : m_name(nullptr)
{
    ++s_depth;
    tracer::write(s_depth, "");
}

function_logger::function_logger(char const* _name)
    : m_name(_name)
{
    ++s_depth;
    tracer::write(s_depth, m_name);
}

void function_logger::add_note(std::string const& _message)
{
    tracer::write(s_depth, _message);
}

void function_logger::increment_log_depth()
{
    s_depth++;
}

void function_logger::decrement_log_depth()
{
    s_depth--;
}

function_logger::~function_logger()
{
    if (m_name != nullptr)
        tracer::write(s_depth, "~" + std::string(m_name));
    else
        tracer::write(s_depth, "");
    --s_depth;
}
}
