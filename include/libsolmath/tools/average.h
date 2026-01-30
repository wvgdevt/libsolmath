/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <deque>

namespace sol::math {
template<class T>
struct average // NOLINT
{
    average& operator=(T _value);
    T avg() const;
    T val() const;
    size_t logs_count() const;
    void clear() { m_history.clear(); }

private:
    std::deque<T> m_history;
};
}
