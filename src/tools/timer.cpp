/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#include "libsolmath/tools/timer.h"

namespace sol::math {
timer::timer() { m_start = std::chrono::high_resolution_clock::now(); }

void timer::reset() { m_start = std::chrono::high_resolution_clock::now(); }

float timer::passed_milliseconds() const
{
    auto const end      = std::chrono::high_resolution_clock::now();
    auto const duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    return static_cast<float>(static_cast<float>(duration.count()) / 1000.0);
}

double timer::passed_precise() const
{
    auto const end      = std::chrono::high_resolution_clock::now();
    auto const duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
    return static_cast<double>(duration.count());
}
}
