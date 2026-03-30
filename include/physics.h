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
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

// GLM forward declarations are fine
#include <cstdint>
#include <glm/fwd.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "linegroup.h"
#include "mesh.h"

class Application;
class Model;

enum MotionType : uint8_t {
    Static = 0,
    Kinematic,
    Dynamic,
};

enum ColliderType {
    Box = 0,
    Capsule = 1,
    TriList = 5,
    Compound,
};

struct PhysicsComponent {
        PhysicsComponent(JPH::BodyID id);
        virtual ~PhysicsComponent() = default;
        virtual void onContactAdded(Model* self, Model* other);
        virtual void onContactRemoved(Model* self, Model* other);
        JPH::BodyID bodyId;
        ColliderType colliderType = ColliderType::Box;
        std::optional<LineGroup> mDebugLines = std::nullopt;
        glm::vec3 localOffset;
};

namespace physics {

struct HitResult {
        glm::vec3 point;
        glm::vec3 normal;
        JPH::BodyID bodyId;
        float fraction;  // 0-1, how far along the ray the hit occurred
        bool valid;
};

glm::vec3 syncPhysicsFromRender(Transform& render, PhysicsComponent* physic);
PhysicsComponent* CreatePhysicsBox(const glm::vec3& localoffset, const glm::vec3& model_half_extents,
                                   glm::vec3& world_bottom_position, void* userData);
JPH::Body* createPhysicFromShape(const std::vector<uint32_t> indices, const std::vector<VertexAttributes>& vertices,
                                 const glm::mat4& transformMatrix);
class BoxCollider {
    public:
        BoxCollider(Application* app, const std::string& name, const glm::vec3& center, const glm::vec3& halfExtent,
                    MotionType motionType, bool isSensor, void* userData);
        glm::vec3 mCenter;
        glm::vec3 mHalfExtent;
        std::string mName;
        LineGroup* getDebugLines();
        std::shared_ptr<PhysicsComponent> getPhysicsComponent();
        glm::mat4 getTransformation() const;

    private:
        std::shared_ptr<PhysicsComponent> mPhysicComponent;
        MotionType mMotionType;
        bool mIsSensor;
        void* mUserData;
};

class PhysicSystem {
    public:
        static uint32_t createCollider(Application* app, const std::string& name, const glm::vec3& center,
                                       const glm::vec3& halfExtent, MotionType motionType, bool isSensor,
                                       void* userData);
        static BoxCollider createCollider2(Application* app, const std::string& name, const glm::vec3& center,
                                           const glm::vec3& halfExtent, MotionType motionType, bool isSensor,
                                           void* userData);
        static void removeCollider(BoxCollider& collider);
        static inline std::vector<BoxCollider> mColliders;

    private:
        Application* mApp;
};

HitResult ShootRay(glm::vec3 origin, glm::vec3 direction, float maxDistance);
void prepareJolt();
void JoltLoop(float dt);
std::pair<glm::vec3, glm::quat> getPositionAndRotationyId(JPH::BodyID id);
JPH::BodyID createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, const glm::quat& rotation,
                             MotionType motionType, float friction, float restitution, float linearDamping,
                             float gravityFactor, bool isSensor = false, void* userData = nullptr);
JPH::BodyInterface& getBodyInterface();
void setRotation(JPH::BodyID id, const glm::quat& rot);
void setPosition(JPH::BodyID id, const glm::vec3& pos);
JPH::CharacterVirtual* createCharacter(JPH::Ref<JPH::Shape> shape, const glm::vec3& initialPosition);
JPH::PhysicsSystem* getPhysicsSystem();
JPH::CapsuleShape* createCapsuleShape(float halfHeight, float radius);
void updateCharacter(JPH::CharacterVirtual* physicalCharacter, float dt, JPH::Vec3 movement);
}  // namespace physics

#endif  // !H_WORLDEXPLORER_PHYSICS
