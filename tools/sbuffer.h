#pragma once
#include <atomic>

namespace sol::math {
template<class T>
class sbuffer // NOLINT
{
public:
    sbuffer() {} // NOLINT

    T copy_inactive()
    {
        T copy = m_use_buffer1.load() ? std::move(m_buffer2) : std::move(m_buffer1);
        _swap();
        return std::move(copy);
    }

    T& active_buffer() { return m_use_buffer1.load() ? m_buffer1 : m_buffer2; }

private:
    T& _swap()
    {
        bool const expected = m_use_buffer1.load();
        m_use_buffer1.store(!expected);
        return expected ? m_buffer1 : m_buffer2;
    }

private:
    T m_buffer1;
    T m_buffer2;
    std::atomic<bool> m_use_buffer1 = true;
};
}
