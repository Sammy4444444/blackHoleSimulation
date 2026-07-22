#pragma once

namespace bhs::physics {

class PhysicsWorld {
public:
    static PhysicsWorld& instance();

    void initialize();
    void update(float deltaTime);
    void shutdown();

private:
    PhysicsWorld() = default;
    bool m_initialized = false;
};

} // namespace bhs::physics
