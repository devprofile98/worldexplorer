

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

struct HumanBehaviour : public Behaviour {
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

        HumanBehaviour(std::string name)
            : name(name),
              front(0.0f, 1.0f, 0.0f),  // Forward after 90-degree X rotation
              up(0.0f, 0.0f, -1.0f),    // Up after 90-degree X rotation
              targetDistance(0.3),
              targetOffset(0.0, 0.0, 0.2),
              yaw(90.0f),
              speed(30.0f),
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
            ModelRegistry::instance().registerBehaviour(name, this);
        }

        void onModelLoad(Model* model) override {
            std::cout << "Character Human just created\n";
            physicalCharacter = physics::createCharacter();
        }

        void sayHello() override { std::cout << "Hello from " << name << '\n'; }

        // Handle mouse movement for rotation
        void handleMouseMove(Model* model, MouseEvent event) override {
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
            // glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f,
            // 0.0f)); glm::mat4 totalRotation = yawRotation * initialRotation;  // Combine yaw and initial X rotation

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

        Model* getWeapon() override { return weapon; }

        void handleMouseClick(Model* model, MouseEvent event) override {
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
                isShooting = true;
            }
        }
        void handleMouseScroll(Model* model, MouseEvent event) override {
            auto scroll = std::get<Scroll>(event);
            cameraOffset += scroll.yOffset < 0 ? 0.1 : -0.1;
            targetDistance += scroll.yOffset < 0 ? 0.1 : -0.1;
        }

        void handleAttachedCamera(Model* model, Camera* camera) override {
            auto forward = getForward();

            // Compute the offset direction (modify as needed based on your coordinate system)
            glm::vec3 offset_dir = {forward.y, -forward.x, forward.z};  // Your existing transformation
            offset_dir = glm::normalize(offset_dir);                    // Normalize to get a unit vector

            // Apply the user-defined distance and any additional offset (e.g., vertical adjustment)
            glm::vec3 camera_offset = offset_dir * targetDistance + (-targetOffset);

            // Compute the camera position by subtracting the offset from the actor's position
            glm::vec3 cam_pos = model->mTransform.getPosition() - camera_offset;

            if (model->mBehaviour) {
                camera->mCameraUp = glm::vec3{0, 0, 1};
                // Set the camera position
                camera->setPosition(cam_pos);
                // Set the camera to look at the actor's position
                camera->setTarget(model->mTransform.getPosition() + targetOffset - cam_pos);
            }
        }

        // Handle keyboard input for movement
        void handleKey(Model* model, KeyEvent event, float dt) override {
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
                        std::cout << "find pistol\n";
                        weapon = m;
                        idleAction = "Pistol_Idle_Loop";
                        attackAction = "Pistol_Shoot";
                        return;
                    }
                }
            }
            if (InputManager::keys[GLFW_KEY_2]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "sword") {
                        std::cout << "find sword\n";
                        weapon = m;
                        idleAction = "Sword_Idle";
                        attackAction = "Sword_Attack";
                        return;
                    }
                }
            }
        }

        glm::vec3 processInput() {
            glm::vec3 moveDir(0.0f);  // Movement direction relative to front vector
            bool noKey = true;
            if (InputManager::isKeyDown(GLFW_KEY_SPACE) && state != Jumping) {
                state = Jumping;
                moveDir.z += 1;
                noKey = false;
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
            model->anim->isEnded();
            if (isAiming) {
                if (isShooting && weapon != nullptr) {
                    if (!model->anim->isEnded()) {
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
                            clip_name = "Jog_Fwd_Loop";
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
                model->anim->playAction(clip_name, loop);
                last_clip = clip_name;
            }
        }

        void update(Model* model, float dt) override {
            if (physicalCharacter == nullptr) return;
            auto movement = processInput();

            decideCurrentAnimation(model, dt);

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

            JPH::RVec3 new_position = physicalCharacter->GetPosition();
            JPH::Quat new_rotation = physicalCharacter->GetRotation();
            model->moveTo({new_position.GetX(), new_position.GetZ(), new_position.GetY()});

            glm::quat desired_rotation;
            desired_rotation.x = new_rotation.GetX();
            desired_rotation.y = new_rotation.GetY();
            desired_rotation.z = new_rotation.GetZ();
            desired_rotation.w = new_rotation.GetW();
            model->rotate(desired_rotation);

            // if (model->mTransform.mPosition.z < groundLevel) {
            //     model->mTransform.mPosition.z = groundLevel;
            //     velocity.z = 0;
            //     state = Idle;
            //     isInJump = false;
            // }

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
        //
        glm::vec3 getForward() override { return glm::normalize(glm::cross(front, up)); }
};

HumanBehaviour humanbehaviour{"human"};
