

#include <cstdint>
#include <random>

#include "GLFW/glfw3.h"
#include "animation.h"
#include "application.h"
#include "editor.h"
#include "glm/common.hpp"
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
#include "world.h"

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

namespace {

struct CameraTransition {
        bool active;
        glm::vec3 from;
        glm::vec3 to;
        float timeLeft;
};
}  // namespace

enum class DragonState {
    Idle = 0,
    Running,
    Walking,
    Jumping,
    Falling,
    Aiming,
    Flapping,
};

struct DragonInputHandler : public InputHandler, public PawnBehaviour {
        std::string name;
        glm::vec3 front;  // Character's forward direction
        glm::vec3 up;     // Up vector (typically {0, 1, 0} for world up)
        glm::vec3 targetDistance;
        glm::vec3 targetOffset;
        float roll;     // Horizontal rotation angle (degrees)
        float yaw;      // Horizontal rotation angle (degrees)
        float speed;    // Movement speed (units per second)
        bool isMoving;  // Track movement state for animation
        bool isAiming;
        bool isShooting;
        bool isInJump;
        bool isGrounded;
        float cameraOffset;
        float lastX;
        float lastY;
        DragonState state;
        ::CameraTransition transitoin;
        float groundLevel = -3.262;
        glm::vec3 velocity{0.0};
        Model* weapon;
        ParticleSystem* particleSystem = nullptr;
        std::string idleAction;
        JPH::CharacterVirtual* physicalCharacter;

        DragonInputHandler(std::string name)
            : name(name),
              front(0.0f, 1.0f, 0.0f),  // Forward after 90-degree X rotation
              up(0.0f, 0.0f, -1.0f),    // Up after 90-degree X rotation
              targetDistance(0.3),
              targetOffset(0.0, 0.0, 0.2),
              roll(0.0f),
              yaw(90.0f),
              speed(20.0f),
              isMoving(false),
              isAiming(false),
              isShooting(false),
              isInJump(false),
              isGrounded(true),
              cameraOffset(0.0),
              lastX(0),
              lastY(0),
              state(DragonState::Idle),
              transitoin({false, glm::vec3{0.0}, glm::vec3{0.0}, 0.0}),
              weapon(nullptr),
              // attackAction("Punch_Jab"),
              idleAction("Idle"),
              physicalCharacter(nullptr) {
            ModelRegistry::instance().registerInputHandler(name, this);
            ModelRegistry::instance().registerBehaviour(name, this);
        }

        bool Grounded() { return isGrounded; }
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

            glm::vec3 base_front(0.0f, -1.0f, 0.0f);  // Model's forward after initial X rotation

            // Apply yaw rotation around the world Y-axis (since yaw is horizontal)
            glm::mat4 yaw_rotation = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 0.0f, 1.0f));
            front = glm::normalize(glm::vec3(yaw_rotation * glm::vec4(base_front, 0.0f)));
            // std::cout << glm::to_string(front) << std::endl;

            // 1. +90° rotation around X (converts Y-up → Z-up orientation)
            JPH::Quat rot90_x = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(0.0f));

            // 2. Yaw rotation around world Y (Jolt's up axis)
            JPH::Quat yaw_quat = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), JPH::DegreesToRadians(-yaw));
            JPH::Quat final_rot = yaw_quat * rot90_x;

            auto y = final_rot.GetY();
            final_rot.SetY(final_rot.GetZ());
            final_rot.SetZ(-y);

            physicalCharacter->SetRotation(final_rot);
        }

        BaseModel* getWeapon() override { return weapon; }

        void handleMouseClick(BaseModel* model, MouseEvent event) override {
            auto mouse = std::get<Click>(event);
            if (mouse.click == GLFW_MOUSE_BUTTON_RIGHT) {
                // if (mouse.action == GLFW_PRESS) {
                //     state = Aiming;
                //     isAiming = true;
                //
                //     if (!transitoin.active) {
                //         transitoin.from = cameraOffset + glm::vec3{0.4};
                //         transitoin.to = glm::vec3{0.3};
                //         transitoin.active = true;
                //         transitoin.timeLeft = 0.5;
                //     }
                // } else if (mouse.action == GLFW_RELEASE) {
                //     state = Idle;
                //     isAiming = false;
                //     if (!transitoin.active) {
                //         transitoin.from = glm::vec3{0.3};
                //         transitoin.to = cameraOffset + glm::vec3{0.4};
                //         transitoin.timeLeft = 0.5;
                //         transitoin.active = true;
                //     }
                // }
            } else if (mouse.click == GLFW_MOUSE_BUTTON_LEFT) {
                if (weapon != nullptr) {
                }
                if (mouse.action == GLFW_PRESS || mouse.action == GLFW_REPEAT) {
                    particleSystem->setActive(true);
                } else if (mouse.action == GLFW_RELEASE) {
                    particleSystem->setActive(false);
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
            // Compute the offset direction
            glm::vec3 offset_dir = {forward.y, -forward.x, forward.z};
            offset_dir = glm::normalize(offset_dir);

            glm::vec3 camera_offset = offset_dir * targetDistance + (-targetOffset);

            if (model->mBehaviour) {
                // Compute the camera position by subtracting the offset from the actor's position
                auto roll_factor_z = -glm::sin(glm::radians(roll)) * 10.0f;
                auto roll_factor_x = glm::cos(glm::radians(roll));
                auto cp = model->mTransform.getPosition() - camera_offset +
                          glm::vec3{0.0, 0.0, (max.z - min.z) + roll_factor_z};

                auto ddd = cp - model->mTransform.getPosition();
                auto rx = glm::dot(ddd, model->mTransform.getPosition() - camera_offset);

                // std::cout << glm::radians(roll) << " : " << roll_factor_x << std::endl;
                glm::vec3 cam_pos =
                    cp;  // + glm::vec3{(model->mTransform.getPosition() - camera_offset).x * roll_factor_x, 0.0, 0.0};
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
                state = DragonState::Flapping;
                isInJump = true;
            } else if (state == DragonState::Flapping) {
                state = DragonState::Falling;
            }

            if (InputManager::keys[GLFW_KEY_SPACE] && InputManager::keys[GLFW_KEY_W]) {
                physicalCharacter->SetLinearVelocity({1.0, 2.0, 0.0});
            } else if (InputManager::keys[GLFW_KEY_SPACE]) {
                physicalCharacter->SetLinearVelocity({0.0, 3.0, 0.0});
            } else if (!Grounded() && InputManager::keys[GLFW_KEY_W]) {
                physicalCharacter->SetLinearVelocity({2.0, 0.0, 0.0});
            }
        }

        glm::vec3 processInput() {
            glm::vec3 moveDir(0.0f);  // Movement direction relative to front vector
            bool noKey = true;
            if (InputManager::isKeyDown(GLFW_KEY_SPACE) && state != DragonState::Flapping) {
                state = DragonState::Flapping;
                moveDir.z += 8;
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_W)) {
                if (state != DragonState::Flapping) {
                    state = DragonState::Running;
                }
                moveDir += front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_S)) {
                state = DragonState::Running;
                moveDir -= front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_A)) {
                state = DragonState::Running;
                moveDir += glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_D)) {
                state = DragonState::Running;
                moveDir -= glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (!Grounded()) {
            } else if (noKey) {
                state = DragonState::Idle;
            }

            return moveDir;
        }

        void decideCurrentAnimation(Model* model, float dt) {
            static std::string last_clip;
            std::string clip_name;
            bool loop;
            model->getAnimation()->isEnded();
            // if (isAiming) {
            // if (isShooting && weapon != nullptr) {
            //     if (!model->getAnimation()->isEnded()) {
            //         clip_name = attackAction;
            //         loop = false;
            //     } else {
            //         clip_name = idleAction;
            //         loop = true;
            //         isShooting = false;
            //     }
            // } else {
            // isShooting = false;
            // clip_name = idleAction;
            // loop = true;
            // }
            // } else {
            switch (state) {
                case DragonState::Idle: {
                    clip_name = "Idle";
                    loop = true;
                    break;
                }
                case DragonState::Running: {
                    if (!isInJump) {
                        clip_name = "Walk";
                        loop = true;
                    }
                    break;
                }
                case DragonState::Falling: {
                    clip_name = "Flap";
                    loop = false;
                    break;
                }
                case DragonState::Flapping: {
                    clip_name = "Flap";
                    loop = true;
                    break;
                }
                default:
                    clip_name = "Idle";
                    loop = true;
                    break;
            }
            // }
            if (last_clip != clip_name) {
                model->getAnimation()->playAction(clip_name, loop);
                last_clip = clip_name;
            }
        }

        void onTick(Model* model, float dt) override {
            if (physicalCharacter == nullptr) return;

            // particleSystem->updateParticleSystem(dt, true);
            auto movement = processInput();

            decideCurrentAnimation(static_cast<Model*>(model), dt);

            auto move_amount = movement * speed * dt;

            JPH::Vec3 current_vel = physicalCharacter->GetLinearVelocity();
            JPH::Vec3 jolt_movement = {move_amount.x, current_vel.GetY(), move_amount.y};
            auto ground = physicalCharacter->GetGroundVelocity();
            auto glm_ground = glm::vec3{ground.GetX(), ground.GetY(), ground.GetZ()};

            if (physicalCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
                isInJump = false;
                isGrounded = true;
                if (state == DragonState::Flapping) {
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
                isGrounded = false;
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

        static Particle spawnParticle(ParticleSystem* system, const glm::vec3& emitterPosition) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            auto& settings = system->getSettings();
            glm::vec2& speed_range = settings.speedRange;
            std::uniform_real_distribution<float> dist_for_speed(speed_range.x, speed_range.y);
            std::uniform_real_distribution<float> dist_for_rotation(1.0, 90.0);
            std::uniform_real_distribution<float> dist_for_scale(settings.scaleRange.x, settings.scaleRange.y);

            Particle particle;
            particle.position = emitterPosition;
            particle.life = settings.mLifeTime;
            particle.speed = -dist_for_speed(gen);
            particle.rotate = glm::vec3{dist_for_rotation(gen), dist_for_rotation(gen), dist_for_rotation(gen)};
            particle.scale = dist_for_scale(gen);
            return particle;
        }

        static void simulateParticles(ParticleSystem* system, float dt, std::vector<Particle>& particles,
                                      std::vector<ParticleData>& particlesData) {
            for (size_t i = 0; i < particles.size(); ++i) {
                particles[i].position += glm::vec3{system->getSettings().getSpeedDirection() * particles[i].speed};
                particles[i].life -= dt;
            }
            for (size_t i = 0; i < particles.size(); ++i) {
                auto translation = glm::translate(glm::mat4{1.0f}, particles[i].position);
                auto rotation = glm::normalize(glm::quat(particles[i].rotate));
                auto scale = glm::scale(glm::mat4{1.0}, glm::vec3{particles[i].scale});
                particlesData[i].transformation = translation * glm::mat4_cast(rotation) * scale;
                auto alpha = particles[i].life < 0.0f ? 0.0 : 1.0;
                particlesData[i].props = glm::mix(glm::vec4{0.9, 0.1, 0.0, alpha}, glm::vec4{1.0, 1.0, 0.5, alpha},
                                                  particles[i].life * (1.0f / system->getSettings().mLifeTime));
                particlesData[i].uvoffset = particles[i].uvoffset;
            }
        }

        void onLoad(Model* model) override {
            std::cout << "Character Dragon just created\n";
            physicalCharacter = physics::createCharacter(physics::createCapsuleShape(0.3f, 0.05f), {22, 3, 12});

            static auto listener = new MyCharacterContactListener{};
            listener->character = static_cast<Model*>(model);
            physicalCharacter->SetListener(listener);

            ParticleSystemSetting settings{
                {0.0, 0.0, -1.0}, spawnParticle, simulateParticles, {0.240, 0.370}, {0.018, 0.039}, 0.4f, 0.01, 20};

            particleSystem = new ParticleSystem{PawnBehaviour::app, "dragon fire", settings};

            particleSystem->setEmitterSocket(new BoneSocket{model, "head", glm::vec3{0.0, 0.190, 0.060}, glm::vec3{1.0},
                                                            glm::vec3{90.0, 0.0, 0.0}, AnchorType::Bone});
            particleSystem->setActive(false);
            PawnBehaviour::app->mParticleSystemsManager->registerSystem(particleSystem);
        }
};

DragonInputHandler dragoninputhandler{"dragon"};
