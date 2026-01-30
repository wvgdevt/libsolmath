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
struct LevelData {
    float total_time{};
    float last_time{};
    float first_record_time{};
    std::string padding;
};

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

void logger::log(dc::internal::DebugChannel const& _channel, log_topic const& _topic, std::string const& _msg)
{
    std::lock_guard<std::mutex> lock(g_logger_lock);
    std::string const message = _channel.to_string() + _msg;

    if (m_console_filter_topic.id == 0 || m_console_filter_topic.id == _topic.id)
        std::cout << message << std::endl;

    if (!m_filter.contains(_channel.to_sizet()))
    {
        m_last_messages.emplace_back(message);
        m_remove_timer = 0;
        if (m_last_messages.size() > 10)
            m_last_messages.pop_front();
    }
    m_log[_channel.to_sizet()].emplace_back(message);
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

std::tuple<std::string, std::string> color_by_time(float const _time)
{
    if (_time > 1)
        return {"\033[31m", "\033[0m"}; // RED
    return {"", ""};
}

void tracer::write(size_t const _level, std::string const& _message)
{
    static size_t current_level = 0;
    auto& data                  = g_level_time[_level];
    if (data.padding.empty())
    {
        data.padding           = std::string((_level + 1) * 2, '.');
        data.last_time         = g_time.passed_milliseconds();
        data.first_record_time = g_time.passed_milliseconds();
        if (_level < current_level)
        {
            auto& previous_data    = g_level_time[current_level];
            data.first_record_time = previous_data.first_record_time;
        }
    }

    if (_level < current_level)
    {
        auto& previous_data = g_level_time[current_level];
        data.last_time      = previous_data.first_record_time;
    }

    if (_level != current_level)
    {
        auto& previous_data = g_level_time[current_level];
        previous_data.total_time += g_time.passed_milliseconds() - previous_data.last_time;

        if (_level < current_level)
        {
            auto const color = color_by_time(previous_data.total_time);
            get()._write(
                previous_data.padding
                + std::get<0>(color) + "[" + to_string(previous_data.total_time, 2) + "] " + "total time" +
                std::get<1>(color));
        }
        current_level = _level;
    }

    float const time_diff = std::max(0.00f, g_time.passed_milliseconds() - data.last_time);
    auto const color      = color_by_time(time_diff);
    get()._write(
        data.padding + std::get<0>(color) + "[" + to_string(time_diff, 2) + "] " + _message + std::get<1>(color));
    data.last_time = g_time.passed_milliseconds();
    data.total_time += time_diff;
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

void tracer::_write(std::string const& _message)
{
    m_log += _message + "\n";
}
}
