

//

#include "physics.h"

//
#include <Jolt/RegisterTypes.h>
//
#include <Jolt/Core/Factory.h>
//
#include <Jolt/Physics/PhysicsSystem.h>
//
#include <Jolt/Physics/Body/BodyInterface.h>
//
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
//
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <algorithm>
#include <cstdint>
#include <glm/fwd.hpp>
#include <memory>

#include "Jolt/Core/Reference.h"
#include "Jolt/Geometry/Triangle.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "Jolt/Physics/EActivation.h"
#include "application.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "mesh.h"
#include "shapes.h"
#include "utils.h"

void PhysicsComponent::onContactAdded(Model* self, Model* other) {}
void PhysicsComponent::onContactRemoved(Model* self, Model* other) {}

PhysicsComponent::PhysicsComponent(JPH::BodyID id) : bodyId(id) {}

namespace physics {
using namespace JPH;

// ==================================================================
// JOLT LAYER SYSTEM — YOU MUST PROVIDE THESE
// ==================================================================

namespace Layers {
static constexpr uint8_t NON_MOVING = 0;
static constexpr uint8_t MOVING = 1;
static constexpr uint8_t NUM_LAYERS = 2;
};  // namespace Layers
    //

class MyContactListener : public ContactListener {
    public:
        void OnContactAdded(const Body& body1, const Body& body2, const ContactManifold& manifold,
                            ContactSettings& settings) override {
            // Log or adjust impulses for cube-character collisions
            // if (body1.IsDynamic() && body2.IsDynamic() /* USERDATA_CHARACTER*/) {
            // settings.mCombinedFriction = 0.5f;  // Example: Adjust friction
            // std::cout << "Happening!\n";
            // }
        }
};

// physics_system->SetContactListener(new MyContactListener());

// Broad phase layer interface (decides which broadphase layers objects belong to)
class BroadPhaseLayerInterfaceImpl final : public BroadPhaseLayerInterface {
    public:
        BroadPhaseLayerInterfaceImpl() {
            // Map object layer → broad phase layer
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayer(0);
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayer(1);
        }

        uint GetNumBroadPhaseLayers() const override { return Layers::NUM_LAYERS; }

        BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
            switch ((BroadPhaseLayer::Type)inLayer) {
                case (BroadPhaseLayer::Type)0:
                    return "NON_MOVING";
                case (BroadPhaseLayer::Type)1:
                    return "MOVING";
                default:
                    JPH_ASSERT(false);
                    return "INVALID";
            }
        }
#endif

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// Object layer vs broad phase layer filter (should moving objects collide in broadphase?)
class ObjectVsBroadPhaseLayerFilterImpl final : public ObjectVsBroadPhaseLayerFilter {
    public:
        bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
                case Layers::NON_MOVING:
                    return inLayer2 == BroadPhaseLayer(1);  // non-moving collides only with moving
                case Layers::MOVING:
                    return true;  // moving collides with everything
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
};

// Object layer pair filter (which object layers should generate contacts?)
class ObjectLayerPairFilterImpl final : public ObjectLayerPairFilter {
    public:
        bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
            return true;  // simple demo: everything collides with everything
        }
};

static BroadPhaseLayerInterfaceImpl sBroadPhaseLayerInterface;
static ObjectVsBroadPhaseLayerFilterImpl sObjectVsBroadPhaseLayerFilter;
static ObjectLayerPairFilterImpl sObjectLayerPairFilter;
static TempAllocatorImpl* temp_allocator;
static JobSystemThreadPool* job_system;

static PhysicsSystem physicsSystem;
static BodyID boxBodyID;

JPH::Vec3 toJolt(const glm::vec3 vec) { return {vec.x, vec.z, vec.y}; }

JPH::Quat toJolt(const glm::quat& rot) {
    auto qu = glm::normalize(rot);
    qu.z *= -1;
    qu = glm::normalize(qu);
    return {qu.x, qu.z, qu.y, qu.w};
}

glm::vec3 toGLM(const JPH::Vec3& vec) { return {vec.GetX(), vec.GetZ(), vec.GetY()}; }

HitResult ShootRay(glm::vec3 origin, glm::vec3 direction, float maxDistance) {
    RRayCast ray;
    ray.mOrigin = toJolt(origin);
    ray.mDirection = toJolt(direction * maxDistance);

    RayCastResult hit;

    if (!physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit)) return {.valid = false};

    // Get normal from the hit surface
    BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
    glm::vec3 normal = glm::vec3(0);

    if (lock.Succeeded()) {
        const Body& body = lock.GetBody();

        // Get surface normal at hit point
        Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, ray.GetPointOnRay(hit.mFraction));
        normal = toGLM(joltNormal);
    }

    return {
        .point = toGLM(ray.GetPointOnRay(hit.mFraction)),
        .normal = normal,
        .bodyId = hit.mBodyID,
        .fraction = hit.mFraction,
        .valid = true,
    };
}

HitResult ShootRay2(glm::vec3 origin, glm::vec3 direction, float maxDistance) {
    using namespace JPH;
    RRayCast ray;
    ray.mOrigin = toJolt(origin);
    ray.mDirection = toJolt(direction * maxDistance);

    RayCastResult hit;
    RayCastSettings settings;
    settings.mBackFaceModeConvex = EBackFaceMode::IgnoreBackFaces;  // don't hit inside of meshes

    // Use broadphase + narrowphase for accuracy
    bool didHit = physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit, BroadPhaseLayerFilter{}, ObjectLayerFilter{},
                                                              BodyFilter{});

    if (!didHit) return {.valid = false};

    return {
        .point = toGLM(ray.mOrigin + ray.mDirection * hit.mFraction),
        .normal = glm::vec3{1.0},  // toGLM(hit.mBodyID /* get normal from shape */),
        .bodyId = hit.mBodyID,
        .fraction = hit.mFraction,
        .valid = true,
    };
}

PhysicsSystem* getPhysicsSystem() { return &physicsSystem; }
JPH::CapsuleShape* createCapsuleShape(float halfHeight, float radius) { return new CapsuleShape(halfHeight, radius); }

CharacterVirtual* createCharacter(Ref<Shape> shape, const glm::vec3& initialPosition, uint64 userData) {
    Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();

    // Shape: usually a capsule or cylinder
    settings->mShape = shape;

    // Important parameters
    settings->mMaxSlopeAngle = DegreesToRadians(50.0f);  // max walkable slope
    settings->mMaxStrength = 100.0f;                     // how hard it can push objects
    settings->mCharacterPadding = 0.02f;                 // avoids tunneling
    settings->mPenetrationRecoverySpeed = 1.0f;
    settings->mPredictiveContactDistance = 0.1f;

    // JPH::Quat rotate90X = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(0.0f));
    // Create the character
    auto* character = new CharacterVirtual(
        settings, {initialPosition.x, initialPosition.z, initialPosition.y},  // RVec3(22, 12, 3),  // initial position
        Quat::sIdentity(),                                                    // initial rotation
        userData,                                                             // user data (optional)
        &physicsSystem);

    return character;
}

void updateCharacter(CharacterVirtual* physicalCharacter, float dt, Vec3 movement) {
    // Settings for our update function
    CharacterVirtual::ExtendedUpdateSettings update_settings;

    // This is the key line — tell it what horizontal velocity you want on the ground
    update_settings.mStickToFloorStepDown = JPH::Vec3(0, -0.5f, 0);  // optional: step down small ledges
    update_settings.mWalkStairsStepUp = JPH::Vec3(0, 0.4f, 0);       // optional: climb stairs
                                                                     //
    physicalCharacter->ExtendedUpdate(
        dt, JPH::Vec3(0.0, -9.81, 0.0),  // gravity
        update_settings, physics::getPhysicsSystem()->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        physics::getPhysicsSystem()->GetDefaultLayerFilter(Layers::MOVING), {},  // or your custom one
        {},
        *temp_allocator  // your TempAllocator
    );
}

BodyID createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, const glm::quat& rotation,
                        MotionType motionType, float friction, float restitution, float linearDamping,
                        float gravityFactor, bool isSensor, void* userData) {
    BodyInterface& bi = physicsSystem.GetBodyInterface();

    BodyCreationSettings boxSettings(
        new BoxShape(Vec3(shape.x, shape.z, shape.y), 0.01), RVec3(centerPos.x, centerPos.z, centerPos.y),
        {rotation.x, rotation.z, rotation.y, rotation.w}, (EMotionType)motionType,
        (motionType == MotionType::Static || motionType == MotionType::Kinematic) ? Layers::NON_MOVING
                                                                                  : Layers::MOVING);
    boxSettings.mAllowSleeping = true;

    boxSettings.mFriction = friction;
    boxSettings.mRestitution = restitution;
    boxSettings.mLinearDamping = linearDamping;
    boxSettings.mUserData = reinterpret_cast<uint64_t>(userData);
    boxSettings.mIsSensor = isSensor;
    boxSettings.mGravityFactor = gravityFactor;

    return bi.CreateAndAddBody(boxSettings, EActivation::Activate);
}

static Ref<Shape> CreateBottomOriginBoxShape(const glm::vec3& half_extents) {
    // BoxShape is centered on its own local origin.
    // If the model origin is at the bottom, shift the collision shape upward by half height.
    Vec3 shape_offset(0.0f, 0.0, 0.0f);

    BoxShapeSettings box_settings(toJolt(half_extents));
    Ref<Shape> box_shape = box_settings.Create().Get();

    RotatedTranslatedShapeSettings offset_shape_settings(shape_offset, Quat::sIdentity(), box_shape);
    return offset_shape_settings.Create().Get();
}

glm::vec3 syncPhysicsFromRender(Transform& render, PhysicsComponent* physic) {
    glm::vec3 com_pos = render.mPosition + render.mOrientation * physic->localOffset;

    BodyInterface& bi = physicsSystem.GetBodyInterface();
    bi.SetPositionAndRotation(physic->bodyId, RVec3(com_pos.x, com_pos.z, com_pos.y), toJolt(render.mOrientation),
                              EActivation::Activate);
    return com_pos;
}

glm::vec3 syncPhysicsFromRender(const glm::vec3& position, const glm::quat& orientation, PhysicsComponent* physic) {
    glm::vec3 com_pos = position + orientation * physic->localOffset;

    BodyInterface& bi = physicsSystem.GetBodyInterface();
    bi.SetPositionAndRotation(physic->bodyId, RVec3(com_pos.x, com_pos.z, com_pos.y), toJolt(orientation),
                              EActivation::Activate);
    return com_pos;
}

PhysicsComponent* CreatePhysicsBox(const glm::vec3& localoffset, const glm::vec3& model_half_extents,
                                   glm::vec3& world_bottom_position, void* userData) {
    // auto local_pivot_offset = localoffset;glm::vec3{0.0f, 0.0, model_half_extents.z};
    auto local_pivot_offset = localoffset;
    // auto local_pivot_offset = model_half_extents - world_bottom_position;  // glm::vec3{0.0f, model_half_extents.y,
    // 0.0};

    Ref<Shape> shape = CreateBottomOriginBoxShape(model_half_extents);

    // Put the body's COM at the correct world position.
    Vec3 body_com_pos = toJolt(world_bottom_position + local_pivot_offset);

    BodyCreationSettings settings(shape, RVec3(body_com_pos.GetX(), body_com_pos.GetZ(), body_com_pos.GetY()),
                                  Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
    settings.mUserData = reinterpret_cast<uint64_t>(userData);

    BodyInterface& bi = physicsSystem.GetBodyInterface();
    Body* body = bi.CreateBody(settings);
    bi.AddBody(body->GetID(), EActivation::Activate);
    PhysicsComponent* comp = new PhysicsComponent{body->GetID()};
    comp->localOffset = local_pivot_offset;
    return comp;
}

// PhysicsComponent* CreatePhysicsCapsule(const glm::vec3& localoffset, const glm::vec3& model_half_extents,
//                                        glm::vec3& world_bottom_position, void* userData) {
PhysicsComponent* CreatePhysicsCapsule(const glm::vec3& center, float halfHeight, float radius, void* userData) {
    // auto local_pivot_offset = localoffset;glm::vec3{0.0f, 0.0, model_half_extents.z};
    // auto local_pivot_offset = localoffset;
    // auto local_pivot_offset = model_half_extents - world_bottom_position;  // glm::vec3{0.0f, model_half_extents.y,
    // 0.0};

    Ref<Shape> shape = createCapsuleShape(halfHeight, radius);

    // Put the body's COM at the correct world position.
    // Vec3 body_com_pos = toJolt(world_bottom_position + local_pivot_offset);

    BodyCreationSettings settings(shape, toJolt(center), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
    settings.mUserData = reinterpret_cast<uint64_t>(userData);

    BodyInterface& bi = physicsSystem.GetBodyInterface();
    Body* body = bi.CreateBody(settings);
    bi.AddBody(body->GetID(), EActivation::Activate);
    PhysicsComponent* comp = new PhysicsComponent{body->GetID()};
    // comp->localOffset = local_pivot_offset;
    return comp;
}

BodyInterface& getBodyInterface() { return physicsSystem.GetBodyInterface(); }

void prepareJolt() {
    JPH::RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    const uint32_t maxBodies = 1024;
    const uint32_t maxBodyPairs = 1024;
    const uint32_t maxContactConstraints = 1024;

    physicsSystem.Init(maxBodies, 0, maxBodyPairs, maxContactConstraints, sBroadPhaseLayerInterface,  // Persistent ref
                       sObjectVsBroadPhaseLayerFilter, sObjectLayerPairFilter);

    physicsSystem.SetGravity(Vec3(0, -9.81, 0.0));
    physicsSystem.SetContactListener(nullptr);
    physicsSystem.SetBodyActivationListener(nullptr);

    temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);
    job_system = new JobSystemThreadPool{cMaxPhysicsJobs, cMaxPhysicsBarriers, (int)thread::hardware_concurrency() - 1};

    getPhysicsSystem()->SetContactListener(new MyContactListener());
}
std::pair<glm::vec3, glm::quat> getPositionAndRotationyId(BodyID id) {
    // TRS getTRSById(BodyID id) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
    RMat44 boxTransform = bodyInterface.GetCenterOfMassTransform(id);
    RVec3 position = boxTransform.GetTranslation();
    Quat rotation = boxTransform.GetQuaternion();
    auto y = rotation.GetY();
    rotation.SetY(rotation.GetZ());
    // negative sing is CRITICAL to sync the physical world with the renderer world
    rotation.SetZ(-y);

    glm::quat glm_rot;
    glm_rot.x = rotation.GetX();
    glm_rot.y = rotation.GetY();
    glm_rot.z = rotation.GetZ();
    glm_rot.w = rotation.GetW();
    return {toGLM(position), glm_rot};
}

void setRotation(BodyID id, const glm::quat& rot) {
    auto& interface = physics::getBodyInterface();
    auto qu = glm::normalize(rot);
    qu.z *= -1;
    qu = glm::normalize(qu);
    interface.SetRotation(id, {qu.x, qu.z, qu.y, qu.w}, EActivation::Activate);
}

void setPosition(BodyID id, const glm::vec3& pos) {
    auto& interface = physics::getBodyInterface();
    interface.SetPosition(id, toJolt(pos), EActivation::Activate);
}

void JoltLoop(float dt) { physicsSystem.Update(dt, 1, temp_allocator, job_system); }

BoxCollider::BoxCollider(Application* app, const std::string& name, const glm::vec3& center,
                         const glm::vec3& halfExtent, MotionType motionType, bool isSensor, void* userData)
    : mCenter(center), mHalfExtent(halfExtent), mName(name), mMotionType(motionType) {
    glm::quat qu;
    qu.w = 1.0;
    qu.x = 0.0;
    qu.y = 0.0;
    qu.z = 0.0;
    mPhysicComponent = std::make_shared<PhysicsComponent>(
        physics::createAndAddBody(halfExtent, center, glm::normalize(qu), mMotionType, 0.5, 0.0f, 0.0f, 1.f, isSensor));

    auto box = generateBox({0.0, 0.0, 0.0}, {0.5, 0.5, 0.5});
    mPhysicComponent->mDebugLines = app->mLineEngine->create(
        box, glm::translate(glm::mat4{1.0}, mCenter) * glm::scale(glm::mat4{1.0}, halfExtent),
        (mMotionType == MotionType::Static) ? (isSensor ? glm::vec3{0.3, 0.3, 0.1} : glm::vec3{0.0, 1.0, 0.0})
                                            : glm::vec3{1.0, 0.209, 0.0784});
}

glm::mat4 BoxCollider::getTransformation() const {
    auto [pos, rot] = getPositionAndRotationyId(mPhysicComponent->bodyId);

    return glm::translate(glm::mat4{1.0}, pos) * glm::toMat4(rot) *
           glm::scale(glm::mat4{1.0}, mHalfExtent * glm::vec3{2.0});
}

LineGroup* BoxCollider::getDebugLines() {
    if (mPhysicComponent != nullptr && mPhysicComponent->mDebugLines.has_value()) {
        return &mPhysicComponent->mDebugLines.value();
    }
    return nullptr;
}

std::shared_ptr<PhysicsComponent> BoxCollider::getPhysicsComponent() { return mPhysicComponent; }

uint32_t PhysicSystem::createCollider(Application* app, const std::string& name, const glm::vec3& center,
                                      const glm::vec3& halfExtent, MotionType motionType, bool isSensor,
                                      void* userData) {
    mColliders.emplace_back(app, name, center, halfExtent, motionType, isSensor, userData);
    return mColliders.size() - 1;
}

BoxCollider PhysicSystem::createCollider2(Application* app, const std::string& name, const glm::vec3& center,
                                          const glm::vec3& halfExtent, MotionType motionType, bool isSensor,
                                          void* userData) {
    BoxCollider box = {app, name, center, halfExtent, motionType, isSensor, userData};
    mColliders.emplace_back(box);
    return box;
}

void PhysicSystem::removeCollider(BoxCollider& collider) {
    // TODO check how to check for the result
    auto body_id = collider.getPhysicsComponent()->bodyId;
    BodyInterface& body_interface = physicsSystem.GetBodyInterface();

    auto _ = std::remove_if(mColliders.begin(), mColliders.end(),
                            [&body_id](BoxCollider& box) { return body_id == box.getPhysicsComponent()->bodyId; });
    body_interface.RemoveBody(body_id);
}

Body* createPhysicFromShape(const std::vector<uint32_t> indices, const std::vector<VertexAttributes>& vertices,
                            const glm::mat4& transformMatrix) {
    TriangleList triangles;
    triangles.reserve(indices.size() / 3);

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        auto p = transformMatrix * glm::vec4{vertices[i0].position, 0.0};
        auto p1 = transformMatrix * glm::vec4{vertices[i1].position, 0.0};
        auto p2 = transformMatrix * glm::vec4{vertices[i2].position, 0.0};
        triangles.push_back(Triangle(Float3(p.x, p.z, p.y), Float3(p1.x, p1.z, p1.y), Float3(p2.x, p2.z, p2.y)));
    }

    MeshShapeSettings mesh_setting(triangles);
    mesh_setting.mMaxTrianglesPerLeaf = 4;

    ShapeSettings::ShapeResult result = mesh_setting.Create();
    if (result.HasError()) {
        return nullptr;
    }
    RefConst<Shape> shape = result.Get();

    BodyCreationSettings body_settings(shape, RVec3(0, 0, 0), Quat::sIdentity(), EMotionType::Static,
                                       Layers::NON_MOVING);

    BodyInterface& body_interface = physicsSystem.GetBodyInterface();
    Body* body = body_interface.CreateBody(body_settings);
    body_interface.AddBody(body->GetID(), EActivation::DontActivate);
    return body;
}

}  // namespace physics
