#include "Core/Timer.h"

namespace bhs::core {

Timer::Timer()
    : m_start(Clock::now())
    , m_lastTick(m_start) {}

float Timer::tick() {
    const TimePoint now = Clock::now();
    const float delta = std::chrono::duration<float>(now - m_lastTick).count();
    m_lastTick = now;
    return delta;
}

float Timer::elapsedSeconds() const {
    return std::chrono::duration<float>(Clock::now() - m_start).count();
}

} // namespace bhs::core
