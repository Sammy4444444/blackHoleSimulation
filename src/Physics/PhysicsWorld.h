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

    // Innermost stable circular orbit (ISCO) radius for a non-rotating
    // Schwarzschild black hole: Risco = 6GM/c^2 = 3*Rs. Derived from
    // schwarzschildRadius() for the same reason photonSphereRadius() is
    // (keeps the two radii from drifting apart if mass ever becomes
    // mutable). Used by Milestone 6 as the physically-motivated default
    // inner edge of the accretion disk: material on a circular orbit
    // inside this radius is dynamically unstable and plunges inward on an
    // orbital timescale rather than persisting as disk material, which is
    // the standard justification (Shakura & Sunyaev 1973) for treating the
    // ISCO as the disk's inner boundary.
    float iscoRadius() const { return 3.0f * schwarzschildRadius(); }

private:
    PhysicsWorld() = default;
    bool m_initialized = false;
    float m_mass = 1.0f;
};

} // namespace bhs::physics
