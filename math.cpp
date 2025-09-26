#include "math.h" // NOLINT
#include <sstream>
#include <iomanip>

using namespace sol::math;

namespace sol::math {
bool are_almost_equal(const stype _a, const stype _b) { return std::fabs(_a - _b) < EPSILON; }

stype pow(const stype _x, const stype _y) { return std::pow(_x, _y); }

stype point_direction(const stype _x, const stype _y, const stype _x2, const stype _y2)
{
    return std::atan2(-_x2 + _x, _y2 - _y);
}

stype point_direction(Vector2f const& _x, Vector2f const& _y) { return std::atan2(-_y.x + _x.x, _y.y - _x.y); }

stype point_distance(const stype _x, const stype _y, const stype _x2, const stype _y2)
{
    const stype dx = _x2 - _x;
    const stype dy = _y2 - _y;
    return std::sqrt(dx * dx + dy * dy);
}

stype point_distance(Vector2f const& _a, Vector2f const& _b)
{
    const stype dx = _b.x - _a.x;
    const stype dy = _b.y - _a.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool point_distance(const stype _x, const stype _y, const stype _x2, const stype _y2, const stype _distance)
{
    if (_x < _x2 + _distance && _x > _x2 - _distance &&
        _y < _y2 + _distance && _y > _y2 - _distance)
        return point_distance(_x, _y, _x2, _y2) < _distance;
    return false;
}

bool point_distance(Vector2f const& _a, Vector2f const& _b, float const _distance)
{
    if (_a.x < _b.x + _distance && _a.x > _b.x - _distance &&
        _a.y < _b.y + _distance && _a.y > _b.y - _distance)
        return point_distance(_a, _b) < _distance;
    return false;
}

bool is_close(const stype _x, const stype _y, const stype _x2, const stype _y2, const stype _r, const stype _distance)
{
    if (_x < _x2 + _r && _x > _x2 - _r &&
        _y < _y2 + _r && _y > _y2 - _r)
        return point_distance(_x, _y, _x2, _y2) < _distance;
    return false;
}

bool is_close(Vector2f const& _a, Vector2f const& _b, const stype _r, const stype _distance)
{
    if (_a.x < _b.x + _r && _a.x > _b.x - _r &&
        _a.y < _b.y + _r && _a.y > _b.y - _r)
        return point_distance(_a, _b) < _distance;
    return false;
}

Vector2f abs(Vector2f const& _x) { return Vector2f{abs(_x.x), abs(_x.y)}; }
stype abs(const stype _x) { return std::abs(_x); }
stype cos(const stype _x) { return std::cos(_x); }
stype sin(const stype _x) { return std::sin(_x); }

stype degtorad(const stype _deg) // NOLINT
{
    return _deg * PI / static_cast<stype>(180.0);
}

stype normalize_angle(stype _angle) // NOLINT
{
    _angle = static_cast<stype>(fmod(_angle, TWO_PI));
    if (_angle < 0)
        _angle += TWO_PI;
    return _angle;
}

stype angle_to_diff(const stype _angle, const stype _to) // NOLINT
{
    /*
     * Calculates left or right we should turn from `_angle` to `_to`
     * And return the value [-1...0...1] where |1| means difference between `_angle` and `_to` is 180
     */
    const stype angle_a = normalize_angle(_angle);
    const stype angle_b = normalize_angle(_to);

    const stype diff = angle_a - angle_b;
    if (diff <= PI && diff >= -PI)
        return -diff / PI;
    if (diff > PI)
        return -(diff - TWO_PI) / PI;
    if (diff < -PI)
        return -(diff + TWO_PI) / PI;
    return 0;
}

stype rand(const stype _from, const stype _to)
{
    const stype shift = abs(_to - _from);
    if (_to > _from)
        return _from + rand(shift);
    return _to + rand(shift);
}

bool in_rect(Vector2f const& _point, FloatRect const& _rect)
{
    if (_point.x >= _rect.x && _point.x <= _rect.x + _rect.width &&
        _point.y >= _rect.y && _point.y <= _rect.y + _rect.height)
        return true;
    return false;
}

std::string to_string(stype const _value, int const _precision)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(_precision) << _value;
    return out.str();
}

/*template<class T>
T rand(T _max)
{
    return (T)(std::rand() / RAND_MAX) * _max;
}
template int rand<int>(int);
template float rand<float>(float);*/
}
