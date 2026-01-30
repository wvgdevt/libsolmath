/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <mutex>
#include <shared_mutex>

namespace sol::math {
template<class T>
class slock // NOLINT
{
public:
    slock() {}                                   // NOLINT
    slock(T const& _other) { m_value = _other; } // NOLINT

    void operator =(T const& _other) { store(_other); }

    void store(T _new_value)
    {
        std::unique_lock<std::shared_timed_mutex> lock(m_lock);
        m_value = _new_value;
    }

    T load()
    {
        std::shared_lock<std::shared_timed_mutex> lock(m_lock);
        return m_value;
    }

    std::unique_lock<std::shared_timed_mutex> get_unique_lock()
    {
        return std::unique_lock<std::shared_timed_mutex>(m_lock);
    }

    std::shared_timed_mutex& get_mutex() { return m_lock; }

    T& get_thread_unsafe() { return m_value; }
    void lock() { m_lock.lock(); }
    void unlock() { m_lock.unlock(); }

private:
    T m_value;
    std::shared_timed_mutex m_lock;
};
}
