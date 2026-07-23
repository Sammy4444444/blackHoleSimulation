#pragma once

namespace bhs::physics {

// Geometrized units (G = c = 1), the standard convention in numerical
// relativity and toy simulators. This lets mass be expressed as a small
// dimensionless "world units" value that produces a visually sensible
// Schwarzschild radius (Rs = 2GM/c² reduces to Rs = 2M), instead of using
// real SI values (solar masses, kilometers) that would be meaningless
// next to this project's world scale.
constexpr float kGravitationalConstant = 1.0f;
constexpr float kSpeedOfLight = 1.0f;

} // namespace bhs::physics
