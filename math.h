#pragma once
#include <vector>
#include <cmath>
#include <cassert>
#include <libsolmath/vectors.h>
#include <random>

// tools includes
#include <libsolmath/tools/average.h>
#include <libsolmath/tools/slock.h>
#include <libsolmath/tools/svector.h>
#include <libsolmath/tools/timer.h>
#include <libsolmath/tools/threadpool.h>
#include <libsolmath/tools/sangle.h>
#include <libsolmath/tools/svector.h>
#include <libsolmath/tools/logger.h>
#include <libsolmath/tools/exception.h>
#include <libsolmath/tools/sbuffer.h>
#include <magic_enum/magic_enum.hpp>

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
constexpr stype e() { return M_E; }
stype point_direction(Vector2f const&, Vector2f const&);
stype point_distance(Vector2f const&, Vector2f const&);
bool point_distance(Vector2f const&, Vector2f const&, float);

stype cos(stype);
stype sin(stype);
stype degtorad(stype); // NOLINT
stype pow(stype, stype);
bool are_almost_equal(stype _a, stype _b);
Vector2f abs(Vector2f const& _x);
stype abs(stype _x);

stype normalize_angle(stype);
stype angle_to_diff(stype, stype);

stype rand(stype _max = 1.0f);
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

template<class T>
std::string enum_to_string(T _enum)
{
    return std::string{magic_enum::enum_name(_enum)};
}

inline bool float_equal(float const _a, float const _b)
{
    constexpr float epsilon = 1e-6f; // Adjust as appropriate
    return std::abs(_a - _b) < epsilon;
}
}
