#pragma once

namespace bhs::physics {

    // This simulator works in geometrized units, where the gravitational
    // constant and the speed of light are both defined as 1. This is the
    // standard convention for numerical relativity / toy black-hole sims: it
    // keeps physically-derived quantities (Schwarzschild radius, and later
    // the metric tensor and geodesic integrator) at world-scale numbers
    // instead of astronomically large/small SI values that would be
    // meaningless next to this project's scene scale.
    constexpr float kGravitationalConstant = 1.0f;
    constexpr float kSpeedOfLight = 1.0f;

} // namespace bhs::physics
