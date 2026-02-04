/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <chrono>

namespace sol::math {
class timer // NOLINT
{
public:
    timer();
    void reset();
    [[nodiscard]] float passed_milliseconds() const;
    [[nodiscard]] double passed_precise() const;

private:
    std::chrono::high_resolution_clock::time_point m_start;
};
}
