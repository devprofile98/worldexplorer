

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <random>
#include <vector>

#include "GLFW/glfw3.h"
#include "animation.h"
#include "application.h"
#include "editor.h"
#include "glm/ext/matrix_common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "particle_system.h"
#include "physics.h"
#include "rendererResource.h"
#include "shapes.h"
#include "utils.h"
#include "world.h"

namespace {

constexpr glm::vec2 offsets[]{glm::vec2{0.0, 0.0f},   glm::vec2{0.0, 0.333f},    glm::vec2{0.0, 0.666f},
                              glm::vec2{0.333f, 0.0}, glm::vec2{0.333f, 0.333f}, glm::vec2{0.333f, 0.666f},
                              glm::vec2{0.666f, 0.0}, glm::vec2{0.666f, 0.333f}, glm::vec2{0.666f}};

}  // namespace

static Particle spawnParticle(ParticleSystem* system, const glm::vec3& emitterPosition) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    auto& settings = system->getSettings();
    glm::vec2& speed_range = settings.speedRange;
    std::uniform_real_distribution<float> dist_for_speed(speed_range.x, speed_range.y);
    std::uniform_real_distribution<float> dist_for_rotation(1.0, 90.0);
    std::uniform_int_distribution<uint16_t> dist_for_uvoffset(0, 8);
    std::uniform_real_distribution<float> dist_for_scale(settings.scaleRange.x, settings.scaleRange.y);

    Particle particle;
    particle.position = emitterPosition;
    particle.life = settings.mLifeTime;
    particle.speed = -dist_for_speed(gen);
    particle.rotate = glm::vec3{dist_for_rotation(gen), dist_for_rotation(gen), dist_for_rotation(gen)};
    particle.scale = dist_for_scale(gen);
    particle.uvoffset = offsets[dist_for_uvoffset(gen)];
    return particle;
}

glm::mat4 BillboardMatrix(Application* app, glm::vec3 position, float size) {
    // Extract camera right and up from view matrix (transpose of rotation part)
    auto viewMatrix = app->mCamera.getView();
    glm::vec3 right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    glm::vec3 up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

    // Build matrix that always faces camera
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(right * size, 0);
    m[1] = glm::vec4(up * size, 0);
    m[2] = glm::vec4(0, 0, 1, 0);  // doesn't matter, facing camera
    m[3] = glm::vec4(position, 1);

    return m;
}

static glm::vec3 turbulence(glm::vec3 pos, float time, float strength) {
    float x = glm::sin(pos.y * 1.7f + time * 2.1f) * glm::cos(pos.z * 1.3f + time * 1.7f);
    float z = 0;  // don't disturb vertical rise much
    float y = glm::cos(pos.x * 1.5f + time * 2.3f) * glm::sin(pos.y * 1.9f + time * 1.4f);
    return glm::vec3(x, y, z) * strength;
}

glm::vec4 FireColor(float t) {
    // t = 0 (just born / hottest) → 1 (dying / coolest)

    if (t < 0.2f) {
        // Core — bright white/yellow center
        float s = t / 0.2f;
        return glm::mix(glm::vec4(1.0f, 1.0f, 0.9f, 0.9f),  // white-yellow
                        glm::vec4(1.0f, 0.8f, 0.1f, 0.8f),  // yellow
                        s);
    } else if (t < 0.6f) {
        // Mid — orange
        float s = (t - 0.2f) / 0.4f;
        return glm::mix(glm::vec4(1.0f, 0.8f, 0.1f, 0.8f),  // yellow
                        glm::vec4(1.0f, 0.3f, 0.0f, 0.6f),  // orange-red
                        s);
    } else {
        // Tail — dark red to transparent smoke
        float s = (t - 0.6f) / 0.4f;
        return glm::mix(glm::vec4(1.0f, 0.3f, 0.0f, 0.6f),  // orange-red
                        glm::vec4(0.1f, 0.1f, 0.1f, 0.0f),  // dark smoke, fade out
                        s);
    }
}

static void simulateParticles(ParticleSystem* system, float dt, std::vector<Particle>& particles,
                              std::vector<ParticleData>& particlesData) {
    auto& settings = system->getSettings();
    for (size_t i = 0; i < particles.size(); ++i) {
        auto velocity = settings.getSpeedDirection() * particles[i].speed;
        float t = 1.0f - (particles[i].life / settings.mLifeTime);  // 0=new, 1=old
        auto turb = turbulence(particles[i].position, t * particles[i].scale, 0.8 * t) * particles[i].scale;
        velocity += turb * dt;

        // Add upward buoyancy (hot air rises)
        velocity.z += 2.0f * dt;

        // Drag — fire slows down as it rises
        velocity *= 1.0f - (0.4f * dt);
        particles[i].position += glm::vec3{velocity * dt};
        particles[i].life -= dt;
    }

    for (size_t i = 0; i < particles.size(); ++i) {
        float t = 1.0f - (particles[i].life / settings.mLifeTime);  // 0=new, 1=old
        particlesData[i].transformation = BillboardMatrix(system->mApp, particles[i].position, settings.mSize);
        particlesData[i].props = FireColor(t);
        particlesData[i].uvoffset = particles[i].uvoffset;
    }
}

BoneSocket* muzzleSocket = nullptr;
extern glm::vec3 extern_position;

struct CameraTransition {
        bool active;
        glm::vec3 from;
        glm::vec3 to;
        float timeLeft;
};

enum CharacterState {
    Idle = 0,
    Running,
    Walking,
    Jumping,
    Falling,
    Aiming,
    AimingWalking,
    Crouch,
};

static Model* contactmodel = nullptr;

class MyCharacterContactListener : public JPH::CharacterContactListener {
    public:
        void OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
                            const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition,
                            JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override {
            // Called when character starts touching another body
            const JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), inBodyID2);
            if (!lock.Succeeded()) return;

            const JPH::Body& body = lock.GetBody();
            uint64_t userData = body.GetUserData();
            Model* model = reinterpret_cast<Model*>(userData);

            auto name = model == nullptr ? "No Name" : model->getName();
            if (model != nullptr) {
                contactmodel = model;
            }
        }

        void OnContactPersisted(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
                                const JPH::SubShapeID& inSubShapeID2, JPH::Vec3Arg inContactPosition,
                                JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override {
            // Called every frame while touching
            // printf("Character persisted body %u\n", inBodyID2.GetIndex());
        }

        void OnContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
                              const JPH::SubShapeID& inSubShapeID2) override {
            const JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), inBodyID2);
            if (!lock.Succeeded()) return;

            const JPH::Body& body = lock.GetBody();
            uint64_t userData = body.GetUserData();
            Model* model = reinterpret_cast<Model*>(userData);

            // auto name = model == nullptr ? "No Name" : model->getName();
            // printf("Character touched body %u %lu %s\n", inBodyID2.GetIndex(), userData, name.c_str());
            if (model != nullptr && model->mPhysicComponent != nullptr) {
                model->mPhysicComponent->onContactRemoved(model, character);
            }
        }
        Model* character = nullptr;
};

struct WeaponBehaviour : public PawnBehaviour {
        WeaponBehaviour(std::string name) {
            maxPickDistance = 2.5;  // all weapon should be pickable at this range
            ModelRegistry::instance().registerBehaviour(name, this);
        }

        BoneSocket* activeSocket = nullptr;
        BoneSocket* inactiveSocket = nullptr;

        virtual void onFirePrimary(Model* model, const glm::vec3& characterFront) {
            std::cout << "Action from " << model->getName() << std::endl;
        }

        virtual void onFireSecondary(Model* model, const glm::vec3& characterFront) {
            std::cout << "Action from " << model->getName() << std::endl;
        }

        virtual void onReload(Model* model) { std::cout << "Reload from " << model->getName() << std::endl; }

        void onEquip(Model* weapon, Model* target) override {
            activeSocket->model = target;
            weapon->mSocket = activeSocket;
        }

        void onUnequip(Model* weapon, Model* target) override {
            inactiveSocket->model = target;
            weapon->mSocket = inactiveSocket;
        }
};

class JetPackPhysics : public PhysicsComponent {
    public:
        JetPackPhysics(JPH::BodyID id) : PhysicsComponent(id) {}
        void onContactAdded(Model* self, Model* other) override {
            auto& bodyInterface = physics::getPhysicsSystem()->GetBodyInterface();

            std::cout << "Jetpack contacted by " << other->getName() << '\n';

            bodyInterface.SetAngularVelocity(bodyId, {0.0, 1.0, 0.0});

            self->getAnimation()->playAction("Rotate", true);
        }

        void onContactRemoved(Model* self, Model* other) override {
            std::cout << "Jetpack contact Removed from " << other->getName() << '\n';
            self->getAnimation()->playAction("Rotate", false);
        }
};

struct JetpackBehaviour : public WeaponBehaviour {
        ParticleSystem* particleSystem = nullptr;
        ParticleSystemSetting activeSettings{
            {0.0, 0.0, -1.0}, spawnParticle, simulateParticles, {-1.110, -0.09}, {9.578, 28.660}, 0.3f, 0.02, 50};

        ParticleSystemSetting InactiveSettings{
            {0.0, 0.0, -1.0}, spawnParticle, simulateParticles, {-0.290, -0.09}, {9.578, 28.660}, 0.3f, 0.02, 10};
        JetpackBehaviour(std::string name) : WeaponBehaviour(name) {
            ModelRegistry::instance().registerBehaviour(name, this);

            activeSocket = new BoneSocket{nullptr,
                                          "DEF-spine001",
                                          {-0.0, 0.3499999940395355, -0.10000000149011612},
                                          {0.20000000298023224, 0.20000000298023224, 0.20000000298023224},
                                          glm::quat({-1.5707963705062866, 0.2617993950843811, 0.0}),
                                          AnchorType::Bone};

            inactiveSocket = new BoneSocket{nullptr,
                                            "DEF-spine001",
                                            {-0.0, 0.3499999940395355, -0.10000000149011612},
                                            {0.0, 0.0, 0.0},
                                            glm::quat({-1.5707963705062866, 0.2617993950843811, 0.0}),
                                            AnchorType::Bone};
        }

        void onTick(Model* model, float dt) override {}

        void onEquip(Model* weapon, Model* target) override {
            weapon->setVisible(true);
            particleSystem->setActive(true);
            activeSocket->model = target;
            weapon->mSocket = activeSocket;
        }

        void onUnequip(Model* weapon, Model* target) override {
            weapon->setVisible(false);
            particleSystem->setActive(false);
            inactiveSocket->model = target;
            weapon->mSocket = inactiveSocket;
        }

        void onLoad(Model* model) override {
            auto* cube_phy = new JetPackPhysics{model->mPhysicComponent->bodyId};
            delete model->mPhysicComponent;
            model->mPhysicComponent = cube_phy;

            std::cout << "Jetpack model loooaded " << (model->mPhysicComponent == nullptr) << std::endl;

            particleSystem = new ParticleSystem{PawnBehaviour::app, "jetpack exhaust", InactiveSettings};
            muzzleSocket = new BoneSocket{model,
                                          "",
                                          glm::vec3{0.0, 0.0, -0.990},
                                          glm::vec3{1.0},
                                          glm::quat(glm::radians(glm::vec3{0.0, 180.0f, 0.0})),
                                          AnchorType::Model};
            particleSystem->setEmitterSocket(muzzleSocket);

            app->mParticleSystemsManager->registerSystem(particleSystem);
        }
        void handleKey(BaseModel* model, KeyEvent event, float dt) override {
            auto key = std::get<Keyboard>(event);
            if (key.key == GLFW_KEY_SPACE) {
                if (key.action == GLFW_PRESS) {
                    particleSystem->getSettings() = activeSettings;
                } else if (key.action == GLFW_RELEASE) {
                    particleSystem->getSettings() = InactiveSettings;
                }
            }
        }
};

JetpackBehaviour jpbehaviour{"jp"};

struct PistolBehaviour : public WeaponBehaviour {
        PistolBehaviour(std::string name) : WeaponBehaviour(name) {
            inactiveSocket = new BoneSocket{nullptr,
                                            "DEF-spine001",
                                            {0.0, -0.1, -0.1},
                                            {0.05, 0.05, 0.05},
                                            glm::quat({-1.04719, -0.0, -1.22173}),
                                            AnchorType::Bone};

            activeSocket = new BoneSocket{
                nullptr,         "DEF-handR", {-0.05, 0.290, 0.09}, {0.06, 0.06, 0.06}, glm::quat({0.0, -0.26, 1.57}),
                AnchorType::Bone};
        }

        void onFirePrimary(Model* model, const glm::vec3& characterFront) override {
            static LineGroup debuglinegroup =
                app->mLineEngine->create(generateBox(), glm::mat4{1.0}, {0.2, 0.0, 8.0}).updateVisibility(true);
            std::vector<glm::vec4> line;
            line.push_back(glm::vec4{model->mSocket->globalPosition, 0.0});
            line.push_back(glm::vec4{model->mSocket->globalPosition + (100.0f * glm::normalize(characterFront)), 1.0});
            debuglinegroup.updateLines(line);

            auto [w, h] = PawnBehaviour::app->getWindowSize();
            auto [origin, ray] = PawnBehaviour::app->getCamera().getLookingRay(w, h, {w / 2, h / 2});
            auto hit = physics::ShootRay(origin, ray, 10.0f);
            // auto hit = physics::ShootRay(model->mSocket->globalPosition, characterFront, 100.0f);
            if (hit.valid) {
                JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), hit.bodyId);
                if (lock.Succeeded()) {
                    // Retrieve your entity from userdata
                    Model* entity = (Model*)lock.GetBody().GetUserData();
                    if (entity == nullptr) return;
                    std::cout << "Hit result: " << hit.valid << entity->getName() << std::endl;
                }
            }
        }
};

struct EnemyBehaviour : public PawnBehaviour {
        EnemyBehaviour(std::string name) { ModelRegistry::instance().registerBehaviour(name, this); }
        virtual void TakeDamage() {}
        void onLoad(Model* model) override {}
        int damage = 100;
};

struct ZombieBehaviour : public EnemyBehaviour {
        int state = 0;
        ZombieBehaviour(std::string name) : EnemyBehaviour(name) {}

        void TakeDamage() override {
            damage -= 1;
            std::cout << name << " Damage is " << damage << std::endl;
        }

        void decideAnimation() {
            static int last_state = state;
            if (last_state != state) {
                this->model->getAnimation()->playAction("Zombie_Walk_Fwd_Loop", true);
            }
        }

        void onTick(Model* model, float dt) override {
            static LineGroup debuglinegroup =
                PawnBehaviour::app->mLineEngine->create(generateBox(), glm::mat4{1.0}, {0.1, 0.1, 1.0})
                    .updateVisibility(true);

            auto* actor = PawnBehaviour::app->mWorld->actor;
            if (actor == nullptr) return;
            auto origin = model->mTransform.getPosition();
            origin.z += model->getAABBSize().z + 0.1;  // works as eye socket
            auto diff_vec = glm::normalize(
                (actor->mTransform.getPosition() + glm::vec3{0.0, 0.0, actor->getAABBSize().z / 4}) - origin);
            physics::HitResult hit = physics::ShootRay(origin, diff_vec, 10.0f);

            std::vector<glm::vec4> line;
            line.push_back({origin, 0.0});
            line.push_back({origin + (10.0f * diff_vec), 1.0});
            debuglinegroup.updateLines(line);

            if (hit.valid) {
                JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), hit.bodyId);
                if (lock.Succeeded()) {
                    // Retrieve your entity from userdata
                    Model* entity = (Model*)lock.GetBody().GetUserData();
                    // std::cout << "Succeeded but " << entity << std::endl;
                    if (entity == nullptr) {
                        return;
                    }
                    if (actor == entity) {
                        // std::cout << "Hit result: " << hit.valid << entity->getName() << std::endl;
                        state = 1;  // chasing
                    }
                }
            } else {
                state = 0;
            }

            decideAnimation();
            if (state == 1) {
                diff_vec.z = 0;
                model->moveBy(glm::normalize(diff_vec) * 0.01f);
            }
        }
};

ZombieBehaviour zombiebehaviour{"zombie"};

struct Ak47Behaviour : public WeaponBehaviour {
        Ak47Behaviour(std::string name) : WeaponBehaviour(name) {
            maxPickDistance = 2.5;  // all weapon should be pickable at this range
            inactiveSocket = new BoneSocket{nullptr,
                                            "DEF-neck",
                                            {-0.1, -0.2, -0.1},
                                            {0.002, 0.002, 0.002},
                                            glm::quat({1.919862151145935, 0.08726654201745987, 2.268928050994873}),
                                            AnchorType::Bone};

            activeSocket = new BoneSocket{nullptr,
                                          "DEF-handR",
                                          {-0.01, 0.22, -0.12},
                                          {0.002, 0.002, 0.002},
                                          glm::quat(glm::radians(glm::vec3{0.0, -5.0, -91.417})),
                                          AnchorType::Bone};
        }

        void onFirePrimary(Model* model, const glm::vec3& characterFront) override {
            auto [w, h] = PawnBehaviour::app->getWindowSize();
            auto [origin, ray] = PawnBehaviour::app->getCamera().getLookingRay(w, h, {w / 2, h / 2});
            auto hit = physics::ShootRay(origin, ray, 10.0f);
            if (hit.valid) {
                JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), hit.bodyId);
                if (lock.Succeeded()) {
                    // Retrieve your entity from userdata
                    Model* entity = (Model*)lock.GetBody().GetUserData();
                    if (entity == nullptr) return;
                    // std::cout << "Hit result: " << hit.valid << entity->getName() << std::endl;
                    if (entity && entity->getName() == "zombie") {
                        ZombieBehaviour* zombie = static_cast<ZombieBehaviour*>(entity->mBehaviour);
                        zombie->TakeDamage();
                    }
                }
            }
        }

        void onEquip(Model* weapon, Model* target) override {
            activeSocket->model = target;
            weapon->mSocket = activeSocket;
            if (weapon->mPhysicComponent != nullptr) {
                physics::getBodyInterface().RemoveBody(weapon->mPhysicComponent->bodyId);
                delete weapon->mPhysicComponent;
                weapon->mPhysicComponent = nullptr;
            }
        }
};

Ak47Behaviour ak47behaviour{"ak47"};

struct TorchBehaviour : public WeaponBehaviour {
        ParticleSystem* particleSystem = nullptr;
        int torchlight = -1;
        TorchBehaviour(std::string name) : WeaponBehaviour(name) {
            inactiveSocket = new BoneSocket{nullptr,
                                            "DEF-spine001",
                                            {0.0, -0.1, -0.1},
                                            {0.05, 0.05, 0.05},
                                            glm::quat({-1.04719, -0.0, -1.22173}),
                                            AnchorType::Bone};

            activeSocket = new BoneSocket{
                nullptr,         "DEF-handL", {0.05, 0.070, 0.08}, {0.06, 0.06, 0.06}, glm::quat({0.0, -0.0, 0.0}),
                AnchorType::Bone};
        }

        void onTick(Model* model, float dt) override {
            if (torchlight >= 0) {
                app->mLightManager->getLights()[torchlight].mPosition =
                    glm::vec4{particleSystem->getEmitterSocket()->globalPosition, 0.0f};
                app->mLightManager->update(torchlight, false);
            }
        }

        void onFirePrimary(Model* model, const glm::vec3& characterFront) override {}

        void onEquip(Model* weapon, Model* target) override {
            weapon->setVisible(true);
            particleSystem->setActive(true);
            activeSocket->model = target;
            weapon->mSocket = activeSocket;
        }

        void onUnequip(Model* weapon, Model* target) override {
            weapon->setVisible(false);
            particleSystem->setActive(false);
            inactiveSocket->model = target;
            weapon->mSocket = inactiveSocket;
        }
        void onLoad(Model* model) override {
            ParticleSystemSetting settings{
                {0.0, 0.0, -1.0}, spawnParticle, simulateParticles, {-0.190, -0.02}, {1.528, 9.7}, 0.4f, 0.01, 20};

            particleSystem = new ParticleSystem{PawnBehaviour::app, "torch fire", settings};

            particleSystem->setEmitterSocket(new BoneSocket{model, "", glm::vec3{0.0, 0.0, 2.840}, glm::vec3{1.0},
                                                            glm::vec3{0.0}, AnchorType::Model});

            app->mParticleSystemsManager->registerSystem(particleSystem);

            torchlight = app->mLightManager->createPointLight(
                glm::vec4{0.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0},
                glm::vec4{1.0, 0.0, 0.0, 1.0}, 1.0, -1.0, 1.8, 1.0, "torch light");
        }
};
TorchBehaviour torchbehaviour{"torch"};

struct SwordBehaviour : public WeaponBehaviour {
        SwordBehaviour(std::string name) : WeaponBehaviour(name) {
            inactiveSocket = new BoneSocket{
                nullptr,         "DEF-spine001", {0.16, 0.1, -0.0}, {0.02, 0.02, 0.02}, glm::quat({-1.65, 1.48, 1.74}),
                AnchorType::Bone};

            activeSocket = new BoneSocket{
                nullptr,         "DEF-handR", {-0.05, 0.1, -0.0}, {0.02, 0.02, 0.02}, glm::quat({1.588, 0.180, 0.0104}),
                AnchorType::Bone};
        }
};

PistolBehaviour pistolbehaviour{"pistol"};
SwordBehaviour swordbehaviour{"sword"};

extern std::map<std::string, Action*> selectedanimation;
struct HumanInputHandler : public InputHandler, public PawnBehaviour {
        std::string name;
        glm::vec3 front;  // Character's forward direction
        glm::vec3 up;     // Up vector (typically {0, 1, 0} for world up)
        glm::vec3 targetDistance;
        glm::vec3 targetOffset;
        float roll;      // Horizontal rotation angle (degrees)
        float yaw;       // Horizontal rotation angle (degrees)
        float speed;     // Movement speed (units per second)
        bool isWalking;  // Track movement state for animation
        bool isRunning;
        bool isAiming;
        bool isShooting;
        bool isInJump;
        float cameraOffset;
        float lastX;
        float lastY;
        CharacterState state;
        uint32_t stateBit;
        CameraTransition transitoin;
        float groundLevel = -3.262;
        glm::vec3 velocity{0.0};
        Model* weapon;
        Model* objectToGrab = nullptr;
        std::string attackAction;
        std::string idleAction;
        JPH::CharacterVirtual* physicalCharacter;
        std::vector<Model*> inventory{};

        HumanInputHandler(std::string name)
            : name(name),
              front(0.0f, 1.0f, 0.0f),  // Forward after 90-degree X rotation
              up(0.0f, 0.0f, -1.0f),    // Up after 90-degree X rotation
              targetDistance(0.3),
              targetOffset(0.0, 0.0, 0.2),
              roll(0.0f),
              yaw(90.0f),
              speed(20.0f),
              isWalking(false),
              isRunning(false),
              isAiming(false),
              isShooting(false),
              isInJump(false),
              cameraOffset(0.0),
              lastX(0),
              lastY(0),
              state(Idle),
              stateBit(0),
              transitoin({false, glm::vec3{0.0}, glm::vec3{0.0}, 0.0}),
              weapon(nullptr),
              attackAction("Punch_Jab"),
              idleAction("Idle_Loop"),
              physicalCharacter(nullptr) {
            ModelRegistry::instance().registerInputHandler(name, this);
            ModelRegistry::instance().registerBehaviour(name, this);
        }

        // void sayHello() override { std::cout << "Hello from " << name << '\n'; }

        // Handle mouse movement for rotation
        void handleMouseMove(BaseModel* model, MouseEvent event) override {
            // Sensitivity for mouse movement
            if (physicalCharacter == nullptr) return;
            auto mouse = std::get<Move>(event);
            float diffX = mouse.xPos - lastX;
            float diffY = mouse.yPos - lastY;
            lastX = mouse.xPos;
            lastY = mouse.yPos;

            float sensitivity = 0.3f;

            // Update yaw based on mouse X movement
            yaw -= diffX * sensitivity;
            roll -= diffY * sensitivity;
            if (roll > 70) {
                roll = 70;
            }
            if (roll < -70) {
                roll = -70;
            }

            glm::vec3 baseFront(0.0f, -1.0f, 0.0f);  // Model's forward after initial X rotation

            // Apply yaw rotation around the world Y-axis (since yaw is horizontal)
            glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 0.0f, 1.0f));
            front = glm::normalize(glm::vec3(yawRotation * glm::vec4(baseFront, 0.0f)));
            // std::cout << glm::to_string(front) << std::endl;

            // Update model orientation
            // glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f,
            // 0.0f, 0.0f)); glm::mat4 totalRotation = yawRotation * initialRotation;  // Combine yaw and
            // initial X rotation

            // 1. +90° rotation around X (converts Y-up → Z-up orientation)
            // JPH::Quat rot90X = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(0.0f));

            // 2. Yaw rotation around world Y (Jolt's up axis)
            JPH::Quat yawQuat = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), JPH::DegreesToRadians(-yaw));
            JPH::Quat finalRot = yawQuat;  // * rot90X;

            // auto y = finalRot.GetY();
            // finalRot.SetY(finalRot.GetZ());
            // finalRot.SetZ(-y);

            physicalCharacter->SetRotation(finalRot);
        }

        BaseModel* getWeapon() override { return weapon; }

        void handleMouseClick(BaseModel* model, MouseEvent event) override {
            auto mouse = std::get<Click>(event);
            if (mouse.click == GLFW_MOUSE_BUTTON_RIGHT) {
                if (mouse.action == GLFW_PRESS) {
                    state = Aiming;
                    stateBit |= 1 << Aiming;
                    isAiming = true;

                    // if (!transitoin.active) {
                    //     transitoin.from = cameraOffset + glm::vec3{0.4};
                    //     transitoin.to = glm::vec3{0.3};
                    //     transitoin.active = true;
                    //     transitoin.timeLeft = 0.5;
                    // }
                } else if (mouse.action == GLFW_RELEASE) {
                    state = Idle;
                    stateBit |= 1 << Idle;
                    isAiming = false;
                    // if (!transitoin.active) {
                    //     transitoin.from = glm::vec3{0.3};
                    //     transitoin.to = cameraOffset + glm::vec3{0.4};
                    //     transitoin.timeLeft = 0.5;
                    //     transitoin.active = true;
                    // }
                }
            } else if (mouse.click == GLFW_MOUSE_BUTTON_LEFT) {
                if (weapon != nullptr) {
                    static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onFirePrimary(weapon, front);
                }
                isShooting = true;
            }
        }

        void handleMouseScroll(BaseModel* model, MouseEvent event) override {
            auto scroll = std::get<Scroll>(event);
            cameraOffset += scroll.yOffset < 0 ? 0.1 : -0.1;
            targetDistance += scroll.yOffset < 0 ? 0.1 : -0.1;
        }

        void handleAttachedCamera(BaseModel* model, Camera* camera) override {
            static LineGroup debuglinegroup =
                PawnBehaviour::app->mLineEngine->create(generateBox(), glm::mat4{1.0}, {0.5, 0.5, 0.0})
                    .updateVisibility(true);
            auto [w, h] = PawnBehaviour::app->getWindowSize();
            auto [origin, ray] = PawnBehaviour::app->getCamera().getLookingRay(w, h, {w / 2, h / 2});
            std::vector<glm::vec4> line;
            line.push_back({origin, 0.0});
            line.push_back({origin + (10.0f * glm::normalize(ray)), 1.0});
            debuglinegroup.updateLines(line);
            {
                auto hit = physics::ShootRay(origin, ray, 10.0f);
                if (hit.valid) {
                    JPH::BodyLockRead lock(physics::getPhysicsSystem()->GetBodyLockInterface(), hit.bodyId);
                    if (lock.Succeeded()) {
                        // Retrieve your entity from userdata
                        Model* entity = (Model*)lock.GetBody().GetUserData();

                        // if (entity != nullptr) {
                        // std::cout << entity->getName() << std::endl;
                        // }
                        if (entity != nullptr && objectToGrab != entity && entity->mBehaviour != nullptr &&
                            entity->mBehaviour->maxPickDistance > 0.0) {
                            auto pawn_to_object =
                                glm::length(entity->mTransform.getPosition() - model->mTransform.getPosition());
                            std::cout << pawn_to_object << std::endl;
                            if (pawn_to_object < entity->mBehaviour->maxPickDistance) {
                                std::cout << entity->getName() << " Is pickable " << std::endl;
                                objectToGrab = entity;
                            }
                            // if (entity && entity->type == EntityType::Zombie) {
                            //     Zombie* zombie = static_cast<Zombie*>(entity);
                            //     zombie->TakeDamage(weaponDamage, hit.point, hit.normal);
                            // }
                            debuglinegroup.updateColor({0.5, 0.5, 0.0});
                        } else {
                            objectToGrab = nullptr;
                        }
                    }
                } else {
                    debuglinegroup.updateColor({1.0, 1.0, 1.0});
                    objectToGrab = nullptr;
                }
            }

            auto forward = getForward();

            auto [min, max] = model->getWorldSpaceAABB();

            glm::vec3 offset_dir = {forward.y, -forward.x, forward.z};
            offset_dir = glm::normalize(offset_dir);

            glm::vec3 camera_offset = offset_dir * targetDistance + (-targetOffset);

            // Compute the camera position by subtracting the offset from the actor's position
            // glm::vec3 cam_pos = model->mTransform.getPosition() - camera_offset + glm::vec3{0.0, 0.0, (max.z
            // - min.z)};

            if (model->mBehaviour) {
                auto roll_factor_z = -glm::sin(glm::radians(roll)) * 3.0f;
                auto cp = model->mTransform.getPosition() - camera_offset +
                          glm::vec3{0.0, 0.0, (max.z - min.z) + roll_factor_z};

                glm::vec3 cam_pos = cp;
                camera->mCameraUp = glm::vec3{0, 0, 1};
                // Set the camera position
                camera->setPosition(cam_pos);
                // Set the camera to look at the actor's position
                camera->setTarget(model->mTransform.getPosition() + targetOffset - cam_pos +
                                  glm::vec3{0.0, 0.0, (max.z - min.z)});
            }
        }

        // Handle keyboard input for movement
        void handleKey(BaseModel* model, KeyEvent event, float dt) override {
            auto space_value = InputManager::keys[GLFW_KEY_SPACE];
            if (weapon != nullptr) {
                weapon->mBehaviour->handleKey(model, event, dt);
            }
            if (space_value) {
                state = Jumping;
                stateBit |= 1 << Jumping;
                isInJump = true;
            } else if (state == Jumping) {
                state = Falling;
                stateBit |= 1 << Falling;
            }

            if (InputManager::keys[GLFW_KEY_E] && objectToGrab != nullptr) {
                std::cout << " >> You Grabbed object " << objectToGrab->getName() << std::endl;
                static_cast<WeaponBehaviour*>(objectToGrab->mBehaviour)
                    ->onUnequip(objectToGrab, static_cast<Model*>(model));
                inventory.push_back(objectToGrab);
            }
            if (InputManager::keys[GLFW_KEY_1]) {
                // for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                // if (m->mName == "pistol") {
                if (inventory.size() > 0) {
                    if (weapon != nullptr && inventory[0] == weapon) {
                        return;
                    }
                    if (weapon != nullptr)
                        static_cast<WeaponBehaviour*>(weapon->mBehaviour)
                            ->onUnequip(weapon, static_cast<Model*>(model));
                    weapon = inventory[0];
                    if (weapon->getName() == "ak47") {
                        // idleAction = "ak47_Aim_Loop";
                        idleAction = "ak47_Idle_Loop3";
                        attackAction = "ak47-fire";
                    } else {
                        idleAction = "Pistol_Idle_Loop";
                        attackAction = "Pistol_Shoot";
                    }

                    static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onEquip(weapon, static_cast<Model*>(model));
                }
                //
                //     return;
                // }
                // }
            }
            if (InputManager::keys[GLFW_KEY_2]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "sword") {
                        if (weapon != nullptr)
                            static_cast<WeaponBehaviour*>(weapon->mBehaviour)
                                ->onUnequip(weapon, static_cast<Model*>(model));
                        weapon = m;
                        static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onEquip(weapon, static_cast<Model*>(model));
                        idleAction = "Sword_Idle";
                        attackAction = "Sword_Attack";
                        return;
                    }
                }
            }
            if (InputManager::keys[GLFW_KEY_3]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "jp") {
                        physics::getBodyInterface().DeactivateBody(m->mPhysicComponent->bodyId);
                        if (weapon != nullptr)
                            static_cast<WeaponBehaviour*>(weapon->mBehaviour)
                                ->onUnequip(weapon, static_cast<Model*>(model));
                        weapon = m;
                        static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onEquip(weapon, static_cast<Model*>(model));
                        return;
                    }
                }
            }
            if (InputManager::keys[GLFW_KEY_4]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "torch") {
                        if (weapon != nullptr)
                            static_cast<WeaponBehaviour*>(weapon->mBehaviour)
                                ->onUnequip(weapon, static_cast<Model*>(model));
                        weapon = m;
                        static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onEquip(weapon, static_cast<Model*>(model));
                        idleAction = "Idle_Torch_Loop";
                        attackAction = "Idle_Torch_Loop";
                        return;
                    }
                }
            }

            if (InputManager::keys[GLFW_KEY_SPACE]) {
                if (weapon != nullptr && weapon->getName() == "jp") {
                    physicalCharacter->SetLinearVelocity({0.0, 1.0, 0.0});
                }
            }
        }

        glm::vec3 processInput() {
            glm::vec3 moveDir(0.0f);  // Movement direction relative to front vector
            bool noKey = true;
            bool is_left_shift_down = InputManager::isKeyDown(GLFW_KEY_LEFT_SHIFT);
            bool left_ctrl_down = InputManager::isKeyDown(GLFW_KEY_LEFT_CONTROL);

            isWalking = false;
            isRunning = false;
            if (InputManager::isKeyDown(GLFW_KEY_W)) {
                if (is_left_shift_down) {
                    state = Running;
                    stateBit |= 1 << Running;
                    isRunning = true;
                } else if (left_ctrl_down) {
                    state = Crouch;
                    stateBit |= 1 << Crouch;
                } else if (state != Jumping) {
                    state = Walking;
                    stateBit |= 1 << Walking;
                    isWalking = true;
                }
                moveDir += front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_S)) {
                if (is_left_shift_down) {
                    state = Running;
                    stateBit |= 1 << Running;
                } else if (left_ctrl_down) {
                    state = Crouch;
                    stateBit |= 1 << Crouch;
                } else if (state != Jumping) {
                    state = Walking;
                    stateBit |= 1 << Walking;
                }
                moveDir -= front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_A)) {
                if (is_left_shift_down) {
                    state = Running;
                    stateBit |= 1 << Running;
                } else if (left_ctrl_down) {
                    state = Crouch;
                    stateBit |= 1 << Crouch;
                } else if (state != Jumping) {
                    state = Walking;
                    stateBit |= 1 << Jumping;
                }
                moveDir += glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_D)) {
                if (is_left_shift_down) {
                    state = Running;
                    stateBit |= 1 << Running;
                } else if (left_ctrl_down) {
                    state = Crouch;
                } else if (state != Jumping) {
                    state = Walking;
                    stateBit |= 1 << Walking;
                }
                moveDir -= glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_SPACE) && state != Jumping) {
                if (weapon != nullptr && weapon->getName() == "jp") {
                } else {
                    state = Jumping;
                    stateBit |= 1 << Jumping;
                    moveDir.z += 1;
                    noKey = false;
                }
            }

            if (noKey) {
                state = Idle;
                stateBit |= 1 << Idle;
            }

            // stateBit = 0;  // clearing the state bit for the next frame
            return moveDir;
        }

        void decideCurrentAnimation2(Model* model, float dt) {
            static std::string last_clip;
            std::string clip_name;
            bool loop = false;
            model->getAnimation()->isEnded();

            std::cout << std::hex << stateBit << std::endl;

            if (isAiming) {
                if (isShooting && weapon != nullptr) {
                    if (!model->getAnimation()->isEnded()) {
                        clip_name = attackAction;
                        loop = false;
                    } else {
                        clip_name = idleAction;
                        loop = true;
                        isShooting = false;
                    }
                } else {
                    isShooting = false;
                    clip_name = idleAction;
                    loop = true;
                }
            } else {
                switch (state) {
                    case Idle: {
                        clip_name = idleAction;
                        loop = true;
                        break;
                    }
                    case Walking: {
                        if (!isInJump) {
                            clip_name = "Walk_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Running: {
                        if (!isInJump) {
                            clip_name = "Sprint_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Crouch: {
                        if (!isInJump) {
                            clip_name = "Crouch_Fwd_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Falling:
                    case Jumping: {
                        clip_name = "Jump_Start";
                        loop = false;
                        break;
                    }
                    default:
                        clip_name = idleAction;
                        loop = true;
                        break;
                }
            }
            if (last_clip != clip_name) {
                model->getAnimation()->playAction(clip_name, loop);
                last_clip = clip_name;
            }
        }

        void decideCurrentAnimation(Model* model, float dt) {
            static std::string last_clip;
            std::string clip_name;
            bool loop = false;
            model->getAnimation()->isEnded();

            std::cout << std::hex << stateBit << std::endl;

            if (isAiming) {
                if (isShooting && weapon != nullptr) {
                    clip_name = isWalking ? "shoot-walk-47" : attackAction;
                    if (!model->getAnimation()->isEnded()) {
                        loop = false;
                    } else {
                        // clip_name = idleAction;
                        // clip_name = isWalking ? "shoot-walk-47" : attackAction;
                        loop = true;
                        isShooting = false;
                    }
                } else if (isWalking) {
                    clip_name = "new action";
                    loop = true;
                } else if (isRunning) {
                    clip_name = "ak47-run-shoot";
                    loop = true;
                } else {
                    isShooting = false;
                    clip_name = idleAction;
                    loop = true;
                }
            } else {
                switch (state) {
                    case Idle: {
                        clip_name = idleAction;
                        loop = true;
                        break;
                    }
                    case Walking: {
                        if (!isInJump) {
                            clip_name = "Walk_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Running: {
                        if (!isInJump) {
                            clip_name = "Sprint_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Crouch: {
                        if (!isInJump) {
                            clip_name = "Crouch_Fwd_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Falling:
                    case Jumping: {
                        clip_name = "Jump_Start";
                        loop = false;
                        break;
                    }
                    default:
                        clip_name = idleAction;
                        loop = true;
                        break;
                }
            }
            if (last_clip != clip_name) {
                model->getAnimation()->playAction(clip_name, loop);
                last_clip = clip_name;
            }
        }

        void onTick(Model* model, float dt) override {
            if (physicalCharacter == nullptr) return;
            auto movement = processInput();

            decideCurrentAnimation(static_cast<Model*>(model), dt);

            auto current_speed = speed;
            if (state == Running) {
                current_speed = 2.5 * speed;
            } else if (state == Crouch) {
                current_speed = 0.75 * speed;
            }

            auto move_amount = movement * current_speed * dt;

            JPH::Vec3 current_vel = physicalCharacter->GetLinearVelocity();
            JPH::Vec3 jolt_movement = {move_amount.x, current_vel.GetY(), move_amount.y};
            auto ground = physicalCharacter->GetGroundVelocity();
            auto glm_ground = glm::vec3{ground.GetX(), ground.GetY(), ground.GetZ()};

            if (physicalCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
                isInJump = false;
                if (state == Jumping) {
                    if (state == Running) {
                        jolt_movement.SetY(4.0);
                    } else {
                        jolt_movement.SetY(3.0);
                    }
                    isInJump = true;
                } else {
                    jolt_movement.SetY(std::max(0.0f, jolt_movement.GetY()));
                    if (jolt_movement.GetY() < 0.0) {
                        std::cout << "character is supported " << jolt_movement << "\n";
                    }
                }

            } else {
                auto jolt_gravity = physics::getPhysicsSystem()->GetGravity();
                // apply gravity
                jolt_movement += jolt_gravity * (float)dt;
            }
            if (weapon) {
                weapon->mBehaviour->onTick(weapon, dt);
            }

            physicalCharacter->SetLinearVelocity(jolt_movement);
            physics::updateCharacter(physicalCharacter, dt, {});
            if (contactmodel != nullptr) {
                contactmodel->mPhysicComponent->onContactAdded(contactmodel, static_cast<Model*>(model));
                contactmodel = nullptr;
            }

            JPH::RVec3 new_position = physicalCharacter->GetPosition();
            JPH::Quat new_rotation = physicalCharacter->GetRotation();
            new_position.SetY(new_position.GetY() - model->getAABBSize().z / 4);
            model->moveTo({new_position.GetX(), new_position.GetZ(), new_position.GetY()});

            auto y = new_rotation.GetY();
            new_rotation.SetY(new_rotation.GetZ());
            // negative sing is CRITICAL to sync the physical world with the renderer world
            new_rotation.SetZ(-y);

            glm::quat desired_rotation;
            desired_rotation.x = new_rotation.GetX();
            desired_rotation.y = new_rotation.GetY();
            desired_rotation.z = new_rotation.GetZ();
            desired_rotation.w = new_rotation.GetW();
            model->rotate(desired_rotation);

            // if (transitoin.active) {
            //     transitoin.timeLeft -= dt;
            //     auto t = glm::clamp(transitoin.timeLeft, 0.0f, 1.0f);
            //
            //     float eased_t = t * t * (3.0f - 2.0f * t);
            //     targetDistance = glm::mix(transitoin.from, transitoin.to, 1 - eased_t);
            //
            //     if (transitoin.timeLeft <= 0) {
            //         transitoin.active = false;
            //     }
            // }
            stateBit = 0;
        }

        glm::vec3 getForward() override { return glm::normalize(glm::cross(front, up)); }

        void onLoad(Model* model) override {
            std::cout << "Character Human just created\n" << model->getName() << std::endl;
            // physicalCharacter = physics::createCharacter(physics::createCapsuleShape(0.3f, 0.05f), {22, 3,
            // 12});
            physicalCharacter = physics::createCharacter(physics::createCapsuleShape(0.4f, 0.130f), {0.0, 0.0, 2},
                                                         reinterpret_cast<JPH::uint64>(model));

            static auto listener = new MyCharacterContactListener{};
            listener->character = static_cast<Model*>(model);
            physicalCharacter->SetListener(listener);

            std::vector<std::string> upper_bones = {"DEF-shinL",  "DEF-shinR",  "DEF-toetipR", "DEF-toetipL",
                                                    "DEF-footR",  "DEF-footL",  "DEF-toeL",    "DEF-toeR",
                                                    "DEF-thighL", "DEF-thighR", "DEF-hips"};
            auto* anim = model->getAnimation();
            anim->actions["new action"] = anim->createMaskedAction(upper_bones, "Walk_Loop", "ak47_Aim_Loop");
            anim->actions["shoot-walk-47"] = anim->createMaskedAction(upper_bones, "Walk_Loop", "ak47-fire");
            anim->actions["ak47-run-shoot"] = anim->createMaskedAction(upper_bones, "Sprint_Loop", "ak47-fire");
        }
};

HumanInputHandler humaninputhandler{"human"};

class CubePhysics : public PhysicsComponent {
    public:
        CubePhysics(JPH::BodyID id) : PhysicsComponent(id) {}
        void onContactAdded(Model* self, Model* other) override {
            auto& bodyInterface = physics::getPhysicsSystem()->GetBodyInterface();

            JPH::Vec3 currentVel = bodyInterface.GetLinearVelocity(bodyId);
            JPH::Vec3 newVel = currentVel + JPH::Vec3(0.0f, 5.0f, 0.0f);  // add upward speed

            bodyInterface.SetLinearVelocity(bodyId, newVel);

            std::cout << "Cube contacted by " << other->getName() << '\n';
        }

        void onContactRemoved(Model* self, Model* other) override {
            std::cout << "Cube contact Removed from " << other->getName() << '\n';
        }
};

struct CubeBehaviour : public PawnBehaviour {
        CubeBehaviour(std::string name) { ModelRegistry::instance().registerBehaviour(name, this); }

        void onLoad(Model* model) override {
            auto* cube_phy = new CubePhysics{model->mPhysicComponent->bodyId};
            auto lines = model->mPhysicComponent->mDebugLines.value();
            delete model->mPhysicComponent;
            model->mPhysicComponent = cube_phy;
            model->mPhysicComponent->mDebugLines = lines;
            std::cout << "Cube model loooaded " << (model->mPhysicComponent == nullptr) << std::endl;
        }
};

CubeBehaviour cubebehaviour{"cube"};

struct TreeBehaviour : public InputHandler {};
