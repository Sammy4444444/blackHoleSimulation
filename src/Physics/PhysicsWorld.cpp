#include "Physics/PhysicsWorld.h"

#include "Core/Log.h"
#include "Physics/Constants.h"

namespace bhs::physics {
    using bhs::core::Log;

PhysicsWorld& PhysicsWorld::instance() {
    static PhysicsWorld world;
    return world;
}

void PhysicsWorld::initialize() {
    if (m_initialized) {
        return;
    }

    m_initialized = true;
    Log::info("Physics world initialized (stub - simulation not yet implemented).");
}

void PhysicsWorld::update(float /*deltaTime*/) {
    // Relativistic physics and orbital integration will be implemented here.
}

float PhysicsWorld::schwarzschildRadius() const {
    // Geometrized units (G = c = 1): Rs = 2GM/c² reduces to Rs = 2M.
    return 2.0f * kGravitationalConstant * m_mass / (kSpeedOfLight * kSpeedOfLight);
}

void PhysicsWorld::shutdown() {
    m_initialized = false;
}

} // namespace bhs::physics
