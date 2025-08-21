#include "logger.h"
#include <iostream>
#include <mutex>

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

    if (!m_filter.count(_channel.to_sizet()))
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
}
