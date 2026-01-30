/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#include "libsolmath/tools/sangle.h"
#include <cmath>

namespace sol::math {
sangle::sangle() // NOLINT
{
    m_angle = 0;
    m_sin   = std::sin(0);
    m_cos   = std::cos(0);
}

sangle::sangle(float _value) // NOLINT
{
    m_angle = _value;
    m_sin   = std::sin(_value);
    m_cos   = std::cos(_value);
}
}
