#pragma once

namespace bhs::physics {

class PhysicsWorld {
public:
    static PhysicsWorld& instance();

    void initialize();
    void update(float deltaTime);
    void shutdown();

    float mass() const { return m_mass; }

    // Schwarzschild radius Rs = 2GM/c², in geometrized units (G = c = 1, see
    // Physics/Constants.h) this reduces to Rs = 2M.
    float schwarzschildRadius() const;

private:
    PhysicsWorld() = default;
    bool m_initialized = false;
    float m_mass = 1.0f;
};

} // namespace bhs::physics
