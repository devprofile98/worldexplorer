

//

#include "physics.h"

#include <Jolt/Jolt.h>
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
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

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

        // #if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
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
        // #endif

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

static PhysicsSystem physicsSystem;
static BodyID boxBodyID;

void prepareJolt() {
    JPH::RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    const uint32_t maxBodies = 1024;
    const uint32_t maxBodyPairs = 1024;
    const uint32_t maxContactConstraints = 1024;

    physicsSystem.Init(maxBodies, 0, maxBodyPairs, maxContactConstraints, BroadPhaseLayerInterfaceImpl(),
                       ObjectVsBroadPhaseLayerFilterImpl(), ObjectLayerPairFilterImpl());

    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    // 3. Create static floor (layer 0 = non-moving)
    {
        BodyCreationSettings floorSettings(new BoxShape(Vec3(100.0f, 1.0f, 100.0f)),  // huge flat box
                                           RVec3(0, -1.0f, 0),                        // position
                                           Quat::sIdentity(),                         // rotation
                                           EMotionType::Static, Layers::NON_MOVING);

        floorSettings.mFriction = 0.5f;
        floorSettings.mRestitution = 0.0f;  // no bounce

        bodyInterface.CreateAndAddBody(floorSettings, EActivation::DontActivate);
    }

    {
        BodyCreationSettings boxSettings(new BoxShape(Vec3(1.0f, 1.0f, 1.0f)),  // 2x2x2 meter box
                                         RVec3(0, 10.0f, 0),                    // start 10 meters up
                                         Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);

        boxSettings.mFriction = 0.5f;
        boxSettings.mRestitution = 0.0f;
        boxSettings.mLinearDamping = 0.0f;
        boxSettings.mGravityFactor = 1.0f;  // full gravity

        boxBodyID = bodyInterface.CreateAndAddBody(boxSettings, EActivation::Activate);
    }
}

void JoltLoop(float dt) {
    BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    for (int frame = 0; frame < 1000; ++frame)  // simulate ~16 seconds
    {
        // --- Update physics ---
        physicsSystem.Update(dt, 1, nullptr, nullptr);

        // --- Read back the box transform (this is what you do in your engine!) ---
        RMat44 boxTransform = bodyInterface.GetCenterOfMassTransform(boxBodyID);
        RVec3 position = boxTransform.GetTranslation();
        Quat rotation = boxTransform.GetQuaternion();

        // Print every 30 frames so you can see it fall and stop
        if (frame % 30 == 0) {
            printf("Frame %d | Box Y: %.4f\n", frame, (float)position.GetY());
        }

        // In your real engine you would now do:
        // yourMesh->SetWorldMatrix(ConvertJoltToYourMath(boxTransform));
        // Render(yourMesh);
    }
}
}  // namespace physics
