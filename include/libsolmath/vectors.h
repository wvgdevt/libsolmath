/* BEGIN_LICENSE
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 winsvega
 *
 * Full license text is in the repository root LICENSE file.
 * END_LICENSE */

#pragma once
#include <cmath>
#include <tuple>

namespace sol::math {
struct Vector2f // NOLINT
{
    Vector2f() : x(0), y(0) {}

    // TODO where is it used?
    explicit constexpr Vector2f(std::tuple<float, float> const& _xy) : x(std::get<0>(_xy)), y(std::get<1>(_xy)) {}

    constexpr Vector2f(const float _x, const float _y) : x(_x), y(_y) {}

    static Vector2f zero_vector()
    {
        return Vector2f{0.0f, 0.0f};
    }

    [[nodiscard]] Vector2f normalized() const
    {
        float const magnitude = std::sqrt(x * x + y * y); // NOLINT
        if (magnitude != 0)
            return {x / magnitude, y / magnitude};
        return {0, 0};
    }

    [[nodiscard]] float length() const { return std::sqrt(x * x + y * y); }
    [[nodiscard]] bool is_zero() const { return std::fabs(x) < 0.001 && std::fabs(y) < 0.001; }

    void operator+=(Vector2f const _rhs)
    {
        x += _rhs.x;
        y += _rhs.y;
    }

    void operator-=(Vector2f const _rhs)
    {
        x -= _rhs.x;
        y -= _rhs.y;
    }

    void operator*=(float const _rhs)
    {
        x *= _rhs;
        y *= _rhs;
    }

    bool operator >(Vector2f const _rhs) const { return x > _rhs.x && y > _rhs.y; }
    bool operator <(Vector2f const _rhs) const { return x < _rhs.x && y < _rhs.y; }

public:
    float x;
    float y;
};

struct Vector2u // NOLINT
{
    constexpr Vector2u() : x(0), y(0) {}
    constexpr Vector2u(const unsigned int _x, const unsigned int _y) : x(_x), y(_y) {}
    unsigned int x;
    unsigned int y;
};

struct Vector2st // NOLINT
{
    constexpr Vector2st() : x(0), y(0) {}
    constexpr Vector2st(const size_t _x, const size_t _y) : x(_x), y(_y) {}
    size_t x;
    size_t y;
};

struct Vector2i // NOLINT
{
    constexpr Vector2i() : x(0), y(0) {}
    constexpr Vector2i(const int _x, const int _y) : x(_x), y(_y) {}
    int x;
    int y;
};

struct FloatRect {
    float x;
    float y;
    float width;
    float height;
};

inline bool intersects(FloatRect const& _lhs, FloatRect const& _rhs)
{
    return _lhs.x <= _rhs.x + _rhs.width &&
           _lhs.x + _lhs.width >= _rhs.x &&
           _lhs.y <= _rhs.y + _rhs.height &&
           _lhs.y + _lhs.height >= _rhs.y;
}

inline bool contains(FloatRect const& _rect, Vector2f const& _point)
{
    return _point.x >= _rect.x &&
           _point.x <= _rect.x + _rect.width &&
           _point.y >= _rect.y &&
           _point.y <= _rect.y + _rect.height;
}

inline FloatRect make_view_rect(Vector2f const& _center, float const _distance_x, float const _distance_y)
{
    return {
        .x = _center.x - _distance_x,
        .y = _center.y - _distance_y,
        .width = _distance_x * 2.0f,
        .height = _distance_y * 2.0f,
    };
}

template<class T>
concept vec2_only = std::same_as<T, Vector2f> || std::same_as<T, Vector2i>;

template<vec2_only T>
constexpr T operator-(T const _lhs, T const _rhs) noexcept
{
    return {_lhs.x - _rhs.x, _lhs.y - _rhs.y};
}

template<vec2_only T>
constexpr T operator+(T const _lhs, T const _rhs)
{
    return {_lhs.x + _rhs.x, _lhs.y + _rhs.y};
}

inline Vector2f operator*(Vector2f const _lhs, float const _rhs) { return {_lhs.x * _rhs, _lhs.y * _rhs}; }
inline Vector2f operator/(Vector2f const _lhs, float const _rhs) { return {_lhs.x / _rhs, _lhs.y / _rhs}; }

inline Vector2f operator/(Vector2i const _lhs, float const _rhs)
{
    return {static_cast<float>(_lhs.x) / _rhs, static_cast<float>(_lhs.y) / _rhs};
}
}
