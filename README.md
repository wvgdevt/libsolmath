# libsolmath

`libsolmath` is a small, header-first C++20 math utility library that provides vector types, common math helpers, and assorted tools for game/graphics-style calculations.

## Features

- `Vector2f`, `Vector2i`, and `Vector2u` types with basic arithmetic helpers.
- Common math helpers such as `point_direction`, `point_distance`, angle utilities, and random helpers.
- Utility helpers for timers, logging, threading, and more (see `include/libsolmath/tools`).
- Configurable precision via `SOLMATH_PRECISION`.

## Requirements

- CMake 3.16+ (or any version that supports C++20)
- C++20 compiler
- [`magic_enum`](https://github.com/Neargye/magic_enum) (included via `deps/magic_enum` in the expected layout)

## Building

```bash
cmake -S . -B build
cmake --build build
```

This produces a `solmath` library target that you can link from your own CMake project.

## Usage

Add the library to your project and include the headers you need:

```cpp
#include <libsolmath/math.h>
#include <libsolmath/vectors.h>

int main() {
    sol::math::Vector2f a{1.0f, 2.0f};
    sol::math::Vector2f b{3.0f, 4.0f};

    auto sum = a + b;
    auto len = sum.length();
    auto angle = sol::math::point_direction(a, b);

    return static_cast<int>(len + angle);
}
```

## Precision

`SOLMATH_PRECISION` controls the floating-point precision:

- `float` (default): `-DSOLMATH_PRECISION=float`
- `double`: `-DSOLMATH_PRECISION=double`

You can set it in your CMake configuration:

```bash
cmake -S . -B build -DSOLMATH_PRECISION=float
```

## License

MIT. See [LICENSE](LICENSE) for details.

