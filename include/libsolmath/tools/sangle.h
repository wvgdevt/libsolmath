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
