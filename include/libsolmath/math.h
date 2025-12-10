#pragma once
#include <vector>
#include <cmath>
#include <cassert>
#include <ranges>
#include <random>
#include <magic_enum/magic_enum.hpp>

#include "vectors.h"           // NOLINT
#include "tools/average.h"     // NOLINT
#include "tools/slock.h"       // NOLINT
#include "tools/svector.h"     // NOLINT
#include "tools/timer.h"       // NOLINT
#include "tools/threadpool.h"  // NOLINT
#include "tools/sangle.h"      // NOLINT
#include "tools/svector.h"     // NOLINT
#include "tools/logger.h"      // NOLINT
#include "tools/exception.h"   // NOLINT
#include "tools/sbuffer.h"     // NOLINT

namespace sol {
template<class T, class... Args>
std::shared_ptr<T> SHARED(Args&&... _args) // NOLINT
{
    return std::make_shared<T>(std::forward<Args>(_args)...);
}
}

namespace sol::math {
#if defined(SOLMATH_PRECISION) && SOLMATH_PRECISION == float
constexpr float EPSILON = 1e-6f;
constexpr float TWO_PI  = 2.0 * M_PI;
constexpr float PI      = M_PI;
using stype             = float;
#elif defined(SOLMATH_PRECISION) && SOLMATH_PRECISION == double
    constexpr double EPSILON = 1e-9;
    constexpr double TWO_PI = 2.0 * M_PI;
    constexpr double PI = M_PI;
    using PrecisionType = double;
#endif

constexpr stype pi() { return M_PI; }
constexpr stype two_pi() { return M_PI * 2.f; }
constexpr stype e() { return M_E; }
stype point_direction(Vector2f const&, Vector2f const&);
stype point_distance_heavy(Vector2f const&, Vector2f const&);
bool point_distance(Vector2f const&, Vector2f const&, float);

stype cos(stype);
stype sin(stype);
stype degtorad(stype); // NOLINT
stype radtodeg(stype); // NOLINT
stype pow(stype, stype);
bool are_almost_equal(stype _a, stype _b);
Vector2f abs(Vector2f const& _x);
stype abs(stype _x);

stype normalize_angle(stype);
stype angle_to_diff(stype, stype);

template<class T>
T rand(T const _max = 1.0f)
{
    thread_local static std::mt19937 gen{std::random_device{}()};
    if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> dist(0, _max);
        return dist(gen);
    }
    else if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> dist(0, _max);
        return dist(gen);
    }
    else
        static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
    return 0;
}

stype rand(stype _from, stype _to);
std::string to_string(stype const _value, int const _precision);

bool is_close(Vector2f const&, Vector2f const&, float _r = 50, float _distance = 5);
bool in_rect(Vector2f const& _point, FloatRect const& _rect);

template<class T>
T const& random_element(std::vector<T> const& _options)
{
    assert(_options.size() != 0);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, _options.size() - 1);
    return _options[dist(gen)];
}

template<std::ranges::range R>
auto const& random_element(const R& _r)
{
    if (std::ranges::empty(_r))
    {
        throw std::runtime_error("empty range");
    }

    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> dist(0, std::ranges::size(_r) - 1);

    auto it = std::ranges::begin(_r);
    std::ranges::advance(it, dist(gen));
    return *it;
}

template<class T>
std::string enum_to_string(T _enum)
{
    return std::string{magic_enum::enum_name(_enum)};
}

template<class T>
std::optional<T> string_to_enum(std::string const& _str)
{
    return magic_enum::enum_cast<T>(_str);
}

inline bool float_equal(float const _a, float const _b)
{
    constexpr float epsilon = 1e-6f; // Adjust as appropriate
    return std::abs(_a - _b) < epsilon;
}
}
