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
