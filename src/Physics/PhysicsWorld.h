#pragma once

namespace bhs::physics {

class PhysicsWorld {
public:
    static PhysicsWorld& instance();

    void initialize();
    void update(float deltaTime);
    void shutdown();

    // Black hole mass, in geometrized units (see Physics/Constants.h). No
    // setter yet — a runtime mass slider is a future UI milestone.
    float mass() const { return m_mass; }

    // Schwarzschild radius Rs = 2GM/c^2, derived from mass().
    float schwarzschildRadius() const;

    // Photon sphere radius Rp = 3GM/c^2 = 1.5 * Rs, for a non-rotating
    // Schwarzschild black hole. Derived from schwarzschildRadius() rather
    // than an independent formula so the two radii can never drift apart if
    // mass ever becomes mutable.
    float photonSphereRadius() const { return 1.5f * schwarzschildRadius(); }

private:
    PhysicsWorld() = default;
    bool m_initialized = false;
    float m_mass = 1.0f;
};

} // namespace bhs::physics
