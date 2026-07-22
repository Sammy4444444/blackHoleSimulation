#pragma once

#include <chrono>

namespace bhs::core {

class Timer {
public:
    Timer();

    // Seconds since last call (first call returns time since construction).
    float tick();

    float elapsedSeconds() const;

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_start;
    TimePoint m_lastTick;
};

} // namespace bhs::core
