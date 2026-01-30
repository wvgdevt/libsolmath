/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
namespace sol::math {
class sangle // NOLINT
{
public:
    sangle(); // NOLINT
    sangle(float _value); // NOLINT
    float sin() const { return m_sin; }
    float cos() const { return m_cos; }
    explicit operator double() const { return m_angle; }

private:
    float m_angle;
    float m_sin;
    float m_cos;
};
}
