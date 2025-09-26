#pragma once
#include <cmath>

namespace sol::math {
struct Vector2f // NOLINT
{
public:
    Vector2f() : x(0), y(0) {}
    explicit constexpr Vector2f(std::tuple<float, float> const& _xy) : x(std::get<0>(_xy)), y(std::get<1>(_xy)) {}
    constexpr Vector2f(const float _x, const float _y) : x(_x), y(_y) {}

    static Vector2f const& zero_vector()
    {
        static Vector2f zero_vector(0.0f, 0.0f);
        return zero_vector;
    }

    Vector2f normalized() const
    {
        float const magnitude = std::sqrt(x * x + y * y); // NOLINT
        if (magnitude != 0)
            return Vector2f(x / magnitude, y / magnitude);
        return Vector2f(0, 0);
    }

    float length() const { return std::sqrt(x * x + y * y); }
    bool is_zero() const { return abs(x) < 0.001 && abs(y) < 0.001; }

    void operator+=(Vector2f const& _rhs)
    {
        x += _rhs.x;
        y += _rhs.y;
    }

    void operator*=(float const& _rhs)
    {
        x *= _rhs;
        y *= _rhs;
    }

    bool operator >(Vector2f const& _rhs) const { return x > _rhs.x && y > _rhs.y; }
    bool operator <(Vector2f const& _rhs) const { return x < _rhs.x && y < _rhs.y; }

public:
    float x;
    float y;
};

inline Vector2f operator+(Vector2f const& _lhs, Vector2f const& _rhs)
{
    return Vector2f(_lhs.x + _rhs.x, _lhs.y + _rhs.y);
}

inline Vector2f operator-(Vector2f const& _lhs, Vector2f const& _rhs)
{
    return Vector2f(_lhs.x - _rhs.x, _lhs.y - _rhs.y);
}

inline Vector2f operator*(Vector2f const& _lhs, float const& _rhs) { return Vector2f(_lhs.x * _rhs, _lhs.y * _rhs); }
inline Vector2f operator/(Vector2f const& _lhs, float const& _rhs) { return Vector2f(_lhs.x / _rhs, _lhs.y / _rhs); }

struct Vector2u // NOLINT
{
    constexpr Vector2u() : x(0), y(0) {}
    constexpr Vector2u(const unsigned int _x, const unsigned int _y) : x(_x), y(_y) {}
    unsigned int x;
    unsigned int y;
};

struct FloatRect {
    explicit FloatRect(Vector2f const _wh = {100, 100})
        : x(0), y(0), width(_wh.x), height(_wh.y) {}

    explicit FloatRect(Vector2f const _xy, Vector2f const _wh = {100, 100})
        : x(_xy.x), y(_xy.y), width(_wh.x), height(_wh.y) {}

    float x;
    float y;
    float width;
    float height;
};
}
