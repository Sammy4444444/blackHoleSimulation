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

void PhysicsWorld::shutdown() {
    m_initialized = false;
}

float PhysicsWorld::schwarzschildRadius() const {
    // Rs = 2GM/c^2. This simulator uses geometrized units (G = c = 1, see
    // Constants.h), so the equation reduces to Rs = 2M. G and c are kept as
    // named factors rather than folded away so the formula stays
    // dimensionally honest and correct if units ever change.
    return 2.0f * kGravitationalConstant * m_mass / (kSpeedOfLight * kSpeedOfLight);
}

} // namespace bhs::physics
