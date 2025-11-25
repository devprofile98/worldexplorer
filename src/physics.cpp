

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
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <glm/fwd.hpp>

#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"

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

PhysicsComponent* createAndAddBody(const glm::vec3& shape, const glm::vec3 centerPos, bool active, float friction,
                                   float restitution, float linearDamping, float gravityFactor) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
    // glm::quat desiredRot(0.7f, 0.1f, 0.1f, 0.1f);                                    // w,x,y,z order for glm
    // desiredRot = glm::normalize(desiredRot);                                         // ← THIS IS MANDATORY
    BodyCreationSettings boxSettings(new BoxShape(Vec3(shape.x, shape.z, shape.y)),  // 2x2x2 meter box
                                     RVec3(centerPos.x, centerPos.z, centerPos.y),   // start 10 meters up
                                     // Quat{desiredRot.x, desiredRot.y, desiredRot.z, desiredRot.w},
                                     Quat::sIdentity(), active ? EMotionType::Dynamic : EMotionType::Static,
                                     Layers::MOVING);

    // BodyCreationSettings boxSettings(new BoxShape(Vec3(0.2f, 0.2f, 0.2f)),  // 2x2x2 meter box
    //                                  RVec3(0, 10.0f, 0),                    // start 10 meters up
    //                                  Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);

    // boxSettings.mFriction = 0.5f;
    // boxSettings.mRestitution = 0.0f;
    // boxSettings.mLinearDamping = 0.0f;
    // boxSettings.mGravityFactor = 0.1f;  // full gravity

    boxSettings.mFriction = friction;
    boxSettings.mRestitution = restitution;
    boxSettings.mLinearDamping = linearDamping;
    boxSettings.mGravityFactor = gravityFactor;  // full gravity
                                                 //
    return new PhysicsComponent{bodyInterface.CreateAndAddBody(boxSettings, EActivation::Activate)};
}

void prepareJolt() {
    JPH::RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    const uint32_t maxBodies = 1024;
    const uint32_t maxBodyPairs = 1024;
    const uint32_t maxContactConstraints = 1024;

    physicsSystem.Init(maxBodies, 0, maxBodyPairs, maxContactConstraints, sBroadPhaseLayerInterface,  // Persistent ref
                       sObjectVsBroadPhaseLayerFilter, sObjectLayerPairFilter);

    temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);
    job_system = new JobSystemThreadPool{cMaxPhysicsJobs, cMaxPhysicsBarriers, (int)thread::hardware_concurrency() - 1};

    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    // 3. Create static floor (layer 0 = non-moving)
    {
        BodyCreationSettings floorSettings(new BoxShape(Vec3(5.0f, 0.2f, 5.0f)),  // huge flat box
                                           RVec3(0, -3.46, 1.910),                // position
                                           Quat::sIdentity(),                     // rotation
                                           EMotionType::Static, Layers::NON_MOVING);

        floorSettings.mFriction = 0.5f;
        floorSettings.mRestitution = 0.3f;  // no bounce

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
}

std::pair<glm::vec3, Quat> getPositionById(BodyID id) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
    RMat44 boxTransform = bodyInterface.GetCenterOfMassTransform(id);
    RVec3 position = boxTransform.GetTranslation();
    Quat rotation = boxTransform.GetQuaternion();

    // Convert Jolt::Quat → glm::quat directly (they have the same memory layout!)
    // printf("Box x: %.4f Box y: %.4f Box z: %.4f\n", (float)position.GetX(), (float)position.GetZ(),
    //        (float)position.GetY());
    return {glm::vec3{position.GetX(), position.GetZ(), position.GetY()}, rotation};
}

glm::vec3 JoltLoop(float dt) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    physicsSystem.Update(dt, 1, temp_allocator, job_system);

    // --- Read back the box transform (this is what you do in your engine!) ---
    RMat44 boxTransform = bodyInterface.GetCenterOfMassTransform(boxBodyID);
    RVec3 position = boxTransform.GetTranslation();
    Quat rotation = boxTransform.GetQuaternion();

    // printf("Box x: %.4f Box y: %.4f Box z: %.4f\n", (float)position.GetX(), (float)position.GetZ(),
    //        (float)position.GetY());
    return glm::vec3{position.GetX(), position.GetZ(), position.GetY()};
}
}  // namespace physics
