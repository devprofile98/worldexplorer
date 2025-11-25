#ifndef WORLD_EXPLORER_PHYSICS_H
#define WORLD_EXPLORER_PHYSICS_H

#include <Jolt/Jolt.h>

// Core Jolt headers that define BodyID, BodyCreationSettings, etc.
#include <Jolt/RegisterTypes.h>
//
#include <Jolt/Physics/PhysicsSystem.h>
//
#include <Jolt/Physics/Body/BodyCreationSettings.h>
//
#include <Jolt/Physics/Body/BodyID.h>

// GLM forward declarations are fine
#include <glm/fwd.hpp>

struct PhysicsComponent {
        JPH::BodyID bodyId;
};

namespace physics {
void prepareJolt();
glm::vec3 JoltLoop(float dt);
std::pair<glm::vec3, JPH::Quat> getPositionById(JPH::BodyID id);
PhysicsComponent* createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, bool active, float friction,
                                   float restitution, float linearDamping, float gravityFactor);

}  // namespace physics

#endif  // !H_WORLDEXPLORER_PHYSICS
