

#include <cstdint>
#include <random>

#include "GLFW/glfw3.h"
#include "animation.h"
#include "application.h"
#include "editor.h"
#include "glm/ext/matrix_common.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "physics.h"
#include "rendererResource.h"
#include "world.h"

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
            if (model != nullptr) {
                model->mPhysicComponent->onContactRemoved(model, character);
            }
        }
        Model* character = nullptr;
};

struct WeaponBehaviour : public PawnBehaviour {
        WeaponBehaviour(std::string name) { ModelRegistry::instance().registerBehaviour(name, this); }

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

        // void handleKey(BaseModel* model, KeyEvent event, float dt) override {
        //     auto key = std::get<Keyboard>(event);
        //
        //     if (GLFW_KEY_SPACE == key.key /* && GLFW_REPEAT == key.action*/) {
        //         return;
        //     }
        // }

        void onEquip(Model* weapon, Model* target) override {
            weapon->setVisible(true);
            activeSocket->model = target;
            weapon->mSocket = activeSocket;
        }

        void onUnequip(Model* weapon, Model* target) override {
            weapon->setVisible(false);
            inactiveSocket->model = target;
            weapon->mSocket = inactiveSocket;
        }

        void onLoad(Model* model) override {
            auto* cube_phy = new JetPackPhysics{model->mPhysicComponent->bodyId};
            delete model->mPhysicComponent;
            model->mPhysicComponent = cube_phy;

            std::cout << "Jetpack model loooaded " << (model->mPhysicComponent == nullptr) << std::endl;
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
            auto box = physics::PhysicSystem::createCollider2(app, "new collider",
                                                              model->mSocket->model->mTransform.getPosition(),
                                                              glm::vec3{0.01}, MotionType::Dynamic, false, nullptr);

            auto& bodyInterface = physics::getPhysicsSystem()->GetBodyInterface();

            JPH::Vec3 newVel = JPH::Vec3(characterFront.x, characterFront.z, characterFront.y);  // add upward speed
            newVel *= 5.0;

            bodyInterface.SetLinearVelocity(box.getPhysicsComponent()->bodyId, newVel);
        }
};

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

struct HumanInputHandler : public InputHandler, public PawnBehaviour {
        std::string name;
        glm::vec3 front;  // Character's forward direction
        glm::vec3 up;     // Up vector (typically {0, 1, 0} for world up)
        glm::vec3 targetDistance;
        glm::vec3 targetOffset;
        float yaw;      // Horizontal rotation angle (degrees)
        float speed;    // Movement speed (units per second)
        bool isMoving;  // Track movement state for animation
        bool isAiming;
        bool isShooting;
        bool isInJump;
        float cameraOffset;
        float lastX;
        float lastY;
        CharacterState state;
        CameraTransition transitoin;
        float groundLevel = -3.262;
        glm::vec3 velocity{0.0};
        Model* weapon;
        std::string attackAction;
        std::string idleAction;
        JPH::CharacterVirtual* physicalCharacter;

        HumanInputHandler(std::string name)
            : name(name),
              front(0.0f, 1.0f, 0.0f),  // Forward after 90-degree X rotation
              up(0.0f, 0.0f, -1.0f),    // Up after 90-degree X rotation
              targetDistance(0.3),
              targetOffset(0.0, 0.0, 0.2),
              yaw(90.0f),
              speed(20.0f),
              isMoving(false),
              isAiming(false),
              isShooting(false),
              isInJump(false),
              cameraOffset(0.0),
              lastX(0),
              lastY(0),
              state(Idle),
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
            JPH::Quat rot90X = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(90.0f));

            // 2. Yaw rotation around world Y (Jolt's up axis)
            JPH::Quat yawQuat = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), JPH::DegreesToRadians(-yaw));
            JPH::Quat finalRot = yawQuat * rot90X;

            auto y = finalRot.GetY();
            finalRot.SetY(finalRot.GetZ());
            finalRot.SetZ(-y);

            physicalCharacter->SetRotation(finalRot);
        }

        BaseModel* getWeapon() override { return weapon; }

        void handleMouseClick(BaseModel* model, MouseEvent event) override {
            auto mouse = std::get<Click>(event);
            if (mouse.click == GLFW_MOUSE_BUTTON_RIGHT) {
                if (mouse.action == GLFW_PRESS) {
                    state = Aiming;
                    isAiming = true;

                    if (!transitoin.active) {
                        transitoin.from = cameraOffset + glm::vec3{0.4};
                        transitoin.to = glm::vec3{0.3};
                        transitoin.active = true;
                        transitoin.timeLeft = 0.5;
                    }
                } else if (mouse.action == GLFW_RELEASE) {
                    state = Idle;
                    isAiming = false;
                    if (!transitoin.active) {
                        transitoin.from = glm::vec3{0.3};
                        transitoin.to = cameraOffset + glm::vec3{0.4};
                        transitoin.timeLeft = 0.5;
                        transitoin.active = true;
                    }
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
            auto forward = getForward();

            auto [min, max] = model->getWorldSpaceAABB();
            // Compute the offset direction (modify as needed based on your coordinate system)
            glm::vec3 offset_dir = {forward.y, -forward.x, forward.z};  // Your existing transformation
            offset_dir = glm::normalize(offset_dir);                    // Normalize to get a unit vector

            // Apply the user-defined distance and any additional offset (e.g., vertical adjustment)
            glm::vec3 camera_offset = offset_dir * targetDistance + (-targetOffset);

            // Compute the camera position by subtracting the offset from the actor's position
            glm::vec3 cam_pos = model->mTransform.getPosition() - camera_offset + glm::vec3{0.0, 0.0, (max.z - min.z)};

            if (model->mBehaviour) {
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
            if (space_value) {
                state = Jumping;
                isInJump = true;
            } else if (state == Jumping) {
                state = Falling;
            }

            if (InputManager::keys[GLFW_KEY_1]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "pistol") {
                        if (weapon != nullptr)
                            static_cast<WeaponBehaviour*>(weapon->mBehaviour)
                                ->onUnequip(weapon, static_cast<Model*>(model));
                        weapon = m;
                        idleAction = "Pistol_Idle_Loop";
                        attackAction = "Pistol_Shoot";

                        static_cast<WeaponBehaviour*>(weapon->mBehaviour)->onEquip(weapon, static_cast<Model*>(model));

                        return;
                    }
                }
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
            if (InputManager::keys[GLFW_KEY_SPACE]) {
                if (weapon != nullptr && weapon->getName() == "jp") {
                    physicalCharacter->SetLinearVelocity({0.0, 1.0, 0.0});
                }
            }
        }

        glm::vec3 processInput() {
            glm::vec3 moveDir(0.0f);  // Movement direction relative to front vector
            bool noKey = true;
            if (InputManager::isKeyDown(GLFW_KEY_SPACE) && state != Jumping) {
                if (weapon != nullptr && weapon->getName() == "jp") {
                } else {
                    state = Jumping;
                    moveDir.z += 1;
                    noKey = false;
                }
            }

            if (InputManager::isKeyDown(GLFW_KEY_W)) {
                if (state != Jumping) {
                    state = Running;
                }
                moveDir += front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_S)) {
                state = Running;
                moveDir -= front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_A)) {
                state = Running;
                moveDir += glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_D)) {
                state = Running;
                moveDir -= glm::normalize(glm::cross(front, up));
                noKey = false;
            }
            if (noKey) {
                state = Idle;
            }

            return moveDir;
        }

        void decideCurrentAnimation(Model* model, float dt) {
            static std::string last_clip;
            std::string clip_name;
            bool loop;
            model->getAnimation()->isEnded();
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
                        clip_name = "Idle_Loop";
                        loop = true;
                        break;
                    }
                    case Running: {
                        if (!isInJump) {
                            // clip_name = "Jog_Fwd_Loop";
                            clip_name = "Walk_Loop";
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
                        clip_name = "Idle_Loop";
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

            auto move_amount = movement * speed * dt;

            JPH::Vec3 current_vel = physicalCharacter->GetLinearVelocity();
            JPH::Vec3 jolt_movement = {move_amount.x, current_vel.GetY(), move_amount.y};
            auto ground = physicalCharacter->GetGroundVelocity();
            auto glm_ground = glm::vec3{ground.GetX(), ground.GetY(), ground.GetZ()};

            if (physicalCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
                isInJump = false;
                if (state == Jumping) {
                    jolt_movement.SetY(3.0);
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

            physicalCharacter->SetLinearVelocity(jolt_movement);
            physics::updateCharacter(physicalCharacter, dt, {});
            if (contactmodel != nullptr) {
                contactmodel->mPhysicComponent->onContactAdded(contactmodel, static_cast<Model*>(model));
                contactmodel = nullptr;
            }

            JPH::RVec3 new_position = physicalCharacter->GetPosition();
            JPH::Quat new_rotation = physicalCharacter->GetRotation();
            model->moveTo({new_position.GetX(), new_position.GetZ(), new_position.GetY()});

            glm::quat desired_rotation;
            desired_rotation.x = new_rotation.GetX();
            desired_rotation.y = new_rotation.GetY();
            desired_rotation.z = new_rotation.GetZ();
            desired_rotation.w = new_rotation.GetW();
            model->rotate(desired_rotation);

            if (transitoin.active) {
                transitoin.timeLeft -= dt;
                auto t = glm::clamp(transitoin.timeLeft, 0.0f, 1.0f);

                float eased_t = t * t * (3.0f - 2.0f * t);
                targetDistance = glm::mix(transitoin.from, transitoin.to, 1 - eased_t);

                if (transitoin.timeLeft <= 0) {
                    transitoin.active = false;
                }
            }
        }

        glm::vec3 getForward() override { return glm::normalize(glm::cross(front, up)); }

        void onLoad(Model* model) override {
            std::cout << "Character Human just created\n";
            physicalCharacter = physics::createCharacter(physics::createCapsuleShape(0.3f, 0.05f), {22, 3, 12});

            static auto listener = new MyCharacterContactListener{};
            listener->character = static_cast<Model*>(model);
            physicalCharacter->SetListener(listener);
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
            delete model->mPhysicComponent;
            model->mPhysicComponent = cube_phy;
            std::cout << "Cube model loooaded " << (model->mPhysicComponent == nullptr) << std::endl;
        }
};

CubeBehaviour cubebehaviour{"cube"};

struct TreeBehaviour : public InputHandler {};
