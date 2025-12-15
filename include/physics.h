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
#include <cstdint>
#include <glm/fwd.hpp>
#include <limits>
#include <memory>
#include <vector>

#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"

class Application;

struct PhysicsComponent {
        JPH::BodyID bodyId;
};

namespace physics {

class BoxCollider {
    public:
        BoxCollider(Application* app, const glm::vec3& center, const glm::vec3& halfExtent, bool isStatic);
        glm::vec3 mCenter;
        glm::vec3 mHalfExtent;
        uint32_t getBoxId() const;
        glm::mat4 getTransformation() const;

    private:
        uint32_t mBoxId = std::numeric_limits<uint32_t>::max();
        std::shared_ptr<PhysicsComponent> mPhysicComponent;
        bool mIsStatic;
};

class PhysicSystem {
    public:
        static uint32_t createCollider(Application* app, const glm::vec3& center, const glm::vec3& halfExtent,
                                       bool isStatic);
        static inline std::vector<BoxCollider> mColliders;

    private:
        Application* mApp;
};

void prepareJolt();
void JoltLoop(float dt);
std::pair<glm::vec3, glm::quat> getPositionAndRotationyId(JPH::BodyID id);
PhysicsComponent* createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, const glm::quat& rotation,
                                   bool active, float friction, float restitution, float linearDamping,
                                   float gravityFactor);
JPH::BodyInterface& getBodyInterface();
void setRotation(JPH::BodyID id, const glm::quat& rot);
void setPosition(JPH::BodyID id, const glm::vec3& pos);
JPH::CharacterVirtual* createCharacter();
JPH::PhysicsSystem* getPhysicsSystem();
void updateCharacter(JPH::CharacterVirtual* physicalCharacter, float dt, JPH::Vec3 movement);
}  // namespace physics

#endif  // !H_WORLDEXPLORER_PHYSICS
