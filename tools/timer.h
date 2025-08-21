#pragma once
#include <chrono>

namespace sol::math {
class timer // NOLINT
{
public:
    timer();
    void reset();
    float passed_milliseconds() const;
    double passed_precise() const;

private:
    std::chrono::high_resolution_clock::time_point m_start;
};
}
