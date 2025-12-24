

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
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <glm/fwd.hpp>
#include <memory>

#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MotionType.h"
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
#include "shapes.h"
#include "utils.h"

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
            std::cout << "Happening!\n";
            // }
        }
};

// class MyCharacterContactListener : public CharacterContactListener {
//     public:
//         MyCharacterContactListener(PhysicsSystem* physics_system) : CharacterContactListener() {}
//
//         void OnContactAdded(const CharacterVirtual* character, const BodyID& body_id, const SubShapeID& sub_shape_id,
//                             RVec3Arg contact_position, Vec3Arg contact_normal,
//                             CharacterContactSettings& settings) override {
//             // Log collision with a dynamic body
//             const BodyLockInterface& lock_interface = mPhysicsSystem->GetBodyLockInterfaceNoLock();
//             {
//                 BodyLockRead lock(lock_interface, body_id);
//                 if (lock.Succeeded()) {
//                     const Body& body = lock.GetBody();
//                     if (body.IsDynamic()) {
//                         std::cout << "CharacterVirtual collided with dynamic body ID: "
//                                   << body_id.GetIndexAndSequenceNumber() << "\n";
//                     }
//                 }
//             }
//             // Allow the contact to proceed (e.g., apply friction or push)
//             settings.mCanPushCharacter = true;
//             settings.mCanReceiveImpulses = true;
//         }
// };

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

PhysicsSystem* getPhysicsSystem() { return &physicsSystem; }

CharacterVirtual* createCharacter() {
    Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();

    // Shape: usually a capsule or cylinder
    Ref<Shape> capsule = new CapsuleShape(0.01f, 0.01f);  // half_height = 0.8m (full cyl height 1.6m), radius = 0.4m
    settings->mShape = capsule;

    // Important parameters
    settings->mMaxSlopeAngle = DegreesToRadians(50.0f);  // max walkable slope
    settings->mMaxStrength = 100.0f;                     // how hard it can push objects
    settings->mCharacterPadding = 0.02f;                 // avoids tunneling
    settings->mPenetrationRecoverySpeed = 1.0f;
    settings->mPredictiveContactDistance = 0.1f;
    // settings->mSupportingVolume = Plane(Vec3::sAxisY(), -0.3f);  // plane below feet

    JPH::Quat rotate90X = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(90.0f));
    // Create the character
    return new CharacterVirtual(settings, RVec3(-1.3, -3.0, 0.14),  // initial position
                                rotate90X,                          // initial rotation
                                0,                                  // user data (optional)
                                &physicsSystem);
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

PhysicsComponent* createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, const glm::quat& rotation,
                                   bool active, float friction, float restitution, float linearDamping,
                                   float gravityFactor) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    BodyCreationSettings boxSettings(new BoxShape(Vec3(shape.x, shape.z, shape.y), 0.01),  // 2x2x2 meter box
                                     RVec3(centerPos.x, centerPos.z, centerPos.y),         // start 10 meters up
                                     {rotation.x, rotation.z, rotation.y, rotation.w},
                                     active ? EMotionType::Dynamic : EMotionType::Static,
                                     active ? Layers::MOVING : Layers::NON_MOVING);
    boxSettings.mAllowSleeping = false;

    boxSettings.mFriction = friction;
    boxSettings.mRestitution = restitution;
    boxSettings.mLinearDamping = linearDamping;
    boxSettings.mGravityFactor = gravityFactor;  // full gravity
                                                 //
    return new PhysicsComponent{bodyInterface.CreateAndAddBody(boxSettings, EActivation::Activate)};
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

    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    // 3. Create static floor (layer 0 = non-moving)
    {
        BodyCreationSettings floorSettings(new BoxShape(Vec3(5.0f, 0.2f, 5.0f)),  // huge flat box
                                           RVec3(0, -3.46, 1.910),                // position
                                           Quat::sIdentity(),                     // rotation
                                           EMotionType::Static, Layers::MOVING);

        floorSettings.mFriction = 1.0f;
        floorSettings.mRestitution = 0.1f;  // no bounce

        bodyInterface.CreateAndAddBody(floorSettings, EActivation::DontActivate);
    }

    // {
    //     BodyCreationSettings boxSettings(new BoxShape(Vec3(0.2f, 0.2f, 0.2f)),  // 2x2x2 meter box
    //                                      RVec3(0, 10.0f, 0),                    // start 10 meters up
    //                                      Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    //
    //     boxSettings.mFriction = 0.5f;
    //     boxSettings.mRestitution = 0.0f;
    //     boxSettings.mLinearDamping = 0.0f;
    //     boxSettings.mGravityFactor = 0.1f;  // full gravity
    //
    //     boxBodyID = bodyInterface.CreateAndAddBody(boxSettings, EActivation::Activate);
    // }
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
    return {glm::vec3{position.GetX(), position.GetZ(), position.GetY()}, glm_rot};
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
    interface.SetPosition(id, {pos.x, pos.z, pos.y}, EActivation::Activate);
}

void JoltLoop(float dt) {
    // BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
    physicsSystem.Update(dt, 1, temp_allocator, job_system);
}

BoxCollider::BoxCollider(Application* app, const glm::vec3& center, const glm::vec3& halfExtent, bool isStatic)
    : mCenter(center), mHalfExtent(halfExtent), mIsStatic(isStatic) {
    auto box = generateBox();
    mBoxId = app->mLineEngine->addLines(
        box, glm::translate(glm::mat4{1.0}, mCenter) * glm::scale(glm::mat4{1.0}, halfExtent * glm::vec3{2.0}),
        mIsStatic ? glm::vec3{0.0, 1.0, 0.0} : glm::vec3{1.0, 0.0, 0.0});

    glm::quat qu;
    qu.w = 1.0;
    qu.x = 0.0;
    qu.y = 0.0;
    qu.z = 0.0;
    mPhysicComponent = std::shared_ptr<PhysicsComponent>{
        physics::createAndAddBody(halfExtent, center, glm::normalize(qu), isStatic, 0.5, 0.0f, 0.0f, 1.f)};
}

glm::mat4 BoxCollider::getTransformation() const {
    auto [pos, rot] = getPositionAndRotationyId(mPhysicComponent->bodyId);

    return glm::translate(glm::mat4{1.0}, pos) * glm::toMat4(rot) *
           glm::scale(glm::mat4{1.0}, mHalfExtent * glm::vec3{2.0});
}

uint32_t BoxCollider::getBoxId() const { return mBoxId; }

uint32_t PhysicSystem::createCollider(Application* app, const glm::vec3& center, const glm::vec3& halfExtent,
                                      bool isStatic) {
    mColliders.emplace_back(app, center, halfExtent, isStatic);
    return mColliders.size() - 1;
}

}  // namespace physics
