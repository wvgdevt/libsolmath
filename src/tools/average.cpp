#include "libsolmath/tools/average.h"
#include <numeric>

namespace sol::math {
template<class T>
average<T>& average<T>::operator=(T _value)
{
    m_history.push_back(_value);
    if (m_history.size() > 100)
        m_history.pop_front();
    return *this;
}

template<class T>
T average<T>::avg() const
{
    if (m_history.empty())
        return 0.0f;
    T const sum = std::accumulate(m_history.begin(), m_history.end(), 0.0f);
    return sum / m_history.size();
}

template<class T>
T average<T>::val() const
{
    if (m_history.empty())
        return 0.0f;
    return m_history.back();
}

template<class T>
size_t average<T>::logs_count() const
{
    return m_history.size();
}

// Explicit instantiation for specific types
template class average<int>;
template class average<float>;
template class average<double>;
}
