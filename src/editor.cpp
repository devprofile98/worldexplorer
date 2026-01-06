
#include "editor.h"

#include <optional>
#include <random>
#include <variant>

#include "GLFW/glfw3.h"
#include "application.h"
#include "camera.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/gtx/string_cast.hpp"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "physics.h"
#include "utils.h"
#include "world.h"

BaseModel* GizmoElement::testSelection(Camera& camera, size_t width, size_t height,
                                       std::pair<size_t, size_t> mouseCoord) {
    auto models = {center, z, y, x};
    return testIntersection2(camera, width, height, mouseCoord, models);
}

void GizmoElement::moveTo(const glm::vec3 position) {
    center->moveTo(position);

    x->moveTo(position);
    y->moveTo(position);
    z->moveTo(position);
}

void GizmoElement::onMouseClick(MouseEvent event) {
    auto click = std::get<Click>(event);
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(click.window));

    if (app == nullptr || app->mWorld->actor != nullptr) {
        return;
    }
    if (click.click == GLFW_MOUSE_BUTTON_LEFT) {
        if (click.action == GLFW_PRESS) {
            if (mSelectedPart == center || mSelectedPart == x || mSelectedPart == y || mSelectedPart == z) {
                mIsLocked = true;
                std::cout << "Gizmo is locked in event handlerln " << click.click << std::endl;
                return;
            }

            if (app->mSelectedModel != nullptr) {
                if (z != nullptr && x != nullptr && y != nullptr && GizmoElement::center != nullptr) {
                    auto [min, max] = app->mSelectedModel->getWorldSpaceAABB();
                    auto center = (min + max) / glm::vec3{2.0f};
                    GizmoElement::moveTo(center);
                } else {
                    std::cout << "Couldnt find the gizmo model " << app->mSelectedModel->getName() << std::endl;
                }
            }
        } else if (click.action == GLFW_RELEASE) {
            GizmoElement::mIsLocked = false;
        }
    }
}

// template <typename T>
std::optional<glm::vec3> processGizmoMove(float scalar, const glm::vec3& pos, Camera& camera, Move& move, size_t width,
                                          size_t height) {
    bool happend = false;
    glm::vec3 move_data;
    if (GizmoElement::mSelectedPart == GizmoElement::x) {
        // a hypo YZ wall  orthogonal to axis X to capture moves along X on a plane
        auto [_, data] = testIntersectionWithBox(camera, width, height, {move.xPos, move.yPos},
                                                 {-scalar, pos.y, -scalar},     // wall min
                                                 {scalar, pos.y + 1.0, scalar}  // wall max
        );
        move_data = std::move(data);
        happend = true;
    } else if (GizmoElement::mSelectedPart == GizmoElement::y) {
        // a hypo XZ wall  orthogonal to axis Y to capture moves along Y on a plane
        auto [_, data] = testIntersectionWithBox(camera, width, height, {move.xPos, move.yPos},
                                                 {pos.x, -scalar, -scalar},   // wall min
                                                 {pos.x + 1, scalar, scalar}  // wall max
        );
        move_data = std::move(data);
        happend = true;
    } else if (GizmoElement::mSelectedPart == GizmoElement::z) {
        // a hypo XY wall  orthogonal to axis Z to capture moves along Z on a plane
        auto [_, data] = testIntersectionWithBox(camera, width, height, {move.xPos, move.yPos},
                                                 {-scalar, pos.y, -scalar},     // wall min
                                                 {scalar, pos.y + 1.0, scalar}  // wall max
        );
        move_data = std::move(data);
        happend = true;
    }
    if (happend) {
        auto& which_gizmo = GizmoElement::mSelectedPart;
        glm::vec3 new_pos = {
            which_gizmo == GizmoElement::x ? move_data.x : pos.x,
            which_gizmo == GizmoElement::y ? move_data.y : pos.y,
            which_gizmo == GizmoElement::z ? move_data.z : pos.z,
        };
        return new_pos;
    }
    return std::nullopt;
}

void GizmoElement::onMouseMove(MouseEvent event) {
    auto move = std::get<Move>(event);
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(move.window));

    // if (that != nullptr) {
    //     DragState& drag_state = that->getCamera().getDrag();
    //     if (drag_state.active) that->getCamera().processMouse(move.xPos, move.yPos);
    // }

    auto [width, height] = that->getWindowSize();
    auto intersected_model = GizmoElement::testSelection(that->getCamera(), width, height, {move.xPos, move.yPos});
    static glm::vec3 first_click_pos;

    auto& editor_selected = that->mEditor->mSelectedObject;
    // if (std::holds_alternative<BaseModel*>(editor_selected)) {
    if (!GizmoElement::mIsLocked) {
        if (intersected_model != nullptr) {
            if (GizmoElement::mSelectedPart != nullptr) {
                GizmoElement::mSelectedPart->selected(false);
            }
            GizmoElement::mSelectedPart = intersected_model;
            GizmoElement::mSelectedPart->selected(true);
            // std::cout << std::format("the ray {} intersect with {}\n", "", intersected_model->getName());
        } else {
            if (GizmoElement::mSelectedPart != nullptr) {
                GizmoElement::mSelectedPart->selected(false);
                GizmoElement::mSelectedPart = nullptr;
            }
        }
    } else if (GizmoElement::mSelectedPart != nullptr) {
        if (std::holds_alternative<BaseModel*>(editor_selected)) {
            BaseModel* selected_entity = std::get<BaseModel*>(editor_selected);
            auto& pos = selected_entity->mTransform.getPosition();
            auto new_pos = processGizmoMove(100, pos, that->getCamera(), move, width, height);
            if (new_pos.has_value()) {
                selected_entity->moveTo(new_pos.value());
                auto [min, max] = selected_entity->getWorldSpaceAABB();
                GizmoElement::moveTo((max + min) / glm::vec3{2.0});
            }

        } else if (std::holds_alternative<physics::BoxCollider*>(editor_selected)) {
            physics::BoxCollider* selected_entity = std::get<physics::BoxCollider*>(editor_selected);
            auto& pos = selected_entity->mCenter;
            auto new_pos = processGizmoMove(100, pos, that->getCamera(), move, width, height);
            if (new_pos.has_value()) {
                selected_entity->mCenter = new_pos.value();
                GizmoElement::moveTo(selected_entity->mCenter);
            }
        } else if (std::holds_alternative<DebugBox*>(editor_selected)) {
            std::cout << "Debug Box\n";
            DebugBox* selected_entity = std::get<DebugBox*>(editor_selected);
            auto& pos = selected_entity->center;
            auto new_pos = processGizmoMove(100, pos, that->getCamera(), move, width, height);
            if (new_pos.has_value()) {
                selected_entity->center = new_pos.value();
                GizmoElement::moveTo(selected_entity->center);
                selected_entity->update();
            }
        }
    }
}

struct GizmoModel : public IModel {
        GizmoModel(Application* app) {
            mModel = new Model{};

            mModel
                ->load("gizmo", app, RESOURCE_DIR "/gizmo/gizmo_up.glb", app->getObjectBindGroupLayout())

                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->mTransform.getLocalTransform();
            mModel->setTransparent(false);

            // if mesh in node animated
            mModel->mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
                .setSize(20 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(app);

            auto& databuffer = mModel->mGlobalMeshTransformationData;
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mGlobalMeshTransformationBuffer.getBuffer(),
                                 0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            auto** gizmo_part = static_cast<BaseModel**>(params);
            *gizmo_part = this->mModel;
            (void)gizmo_part;  // Suppress unused variable warning
        };
};

struct GizmoModelY : public IModel {
        GizmoModelY(Application* app) {
            mModel = new Model{};

            mModel->load("gizmo_y", app, RESOURCE_DIR "/gizmo/gizmo_y.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->mTransform.getLocalTransform();
            mModel->setTransparent(false);

            // if mesh in node animated
            mModel->mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
                .setSize(20 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(app);

            auto& databuffer = mModel->mGlobalMeshTransformationData;
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mGlobalMeshTransformationBuffer.getBuffer(),
                                 0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            auto** gizmo_part = static_cast<BaseModel**>(params);
            *gizmo_part = this->mModel;
            (void)gizmo_part;  // Suppress unused variable warning
        };
};

struct BoneModel : public IModel {
        BoneModel(Application* app) {
            mModel = new Model{};

            mModel->load("bone", app, RESOURCE_DIR "/sphere.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1, 1, 1})
                .scale(glm::vec3{.01f});
            // mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->mTransform.getLocalTransform();
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            auto** indicator_bone = static_cast<BaseModel**>(params);
            *indicator_bone = this->mModel;

            std::vector<glm::vec3> positions;
            std::vector<float> degrees;
            std::vector<glm::vec3> scales;
            positions.reserve(5);
            degrees.reserve(5);
            scales.reserve(5);

            for (size_t i = 0; i < 5; i++) {
                positions.emplace_back(glm::vec3{i, i, i});
                degrees.emplace_back(glm::radians(0.0));
                scales.emplace_back(glm::vec3{0.9f});
            }

            // std::cout << mModel->getName() << positions[0];
            // positions[1] = mModel->mTransform.getPosition();
            auto* ins = new Instance{positions, glm::vec3{0.0, 0.0, 1.0},     degrees,
                                     scales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(), 0,
                                 ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size() - 1));

            // std::cout << "(((((((((((((((( in mesh " << mModel->mMeshes.size() << std::endl;

            mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for bone indicator "))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                          WGPUBufferUsage_CopyDst)
                .setSize(sizeof(DrawIndexedIndirectArgs))
                .setMappedAtCraetion()
                .create(app);

            auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer.getBuffer(), 0,
                                 &indirect, sizeof(DrawIndexedIndirectArgs));

            for (auto& [mat_id, mesh] : mModel->mFlattenMeshes) {
                mesh.mIndirectDrawArgsBuffer.setLabel(("indirect_draw_args_mesh_ " + mModel->getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(app);

                std::cout << ")))))))))))) For " << mModel->getName() << &mesh << " Index count is "
                          << static_cast<uint32_t>(mesh.mIndexData.size()) << std::endl;
                auto indirect = DrawIndexedIndirectArgs{static_cast<uint32_t>(mesh.mIndexData.size()), 0, 0, 0, 0};
                wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0,
                                     &indirect, sizeof(DrawIndexedIndirectArgs));
            }

            mModel->mTransform.mObjectInfo.instanceOffsetId = 0;
            mModel->setInstanced(ins);

            std::cout << "--------------------Barrier reached ----------------- for tree"
                      << mModel->instance->getInstanceCount() << '\n';
        };
};

;

struct GizmoModelX : public IModel {
        GizmoModelX(Application* app) {
            mModel = new Model{};

            mModel->load("gizmo_x", app, RESOURCE_DIR "/gizmo/gizmo_x.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                //
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->mTransform.getLocalTransform();
            mModel->setTransparent(false);

            // if mesh in node animated
            mModel->mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
                .setSize(20 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(app);

            auto& databuffer = mModel->mGlobalMeshTransformationData;
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mGlobalMeshTransformationBuffer.getBuffer(),
                                 0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            auto** gizmo_part = static_cast<BaseModel**>(params);
            *gizmo_part = this->mModel;
            (void)gizmo_part;  // Suppress unused variable warning
        };
};

struct GizmoModelCenter : public IModel {
        GizmoModelCenter(Application* app) {
            mModel = new Model{};

            mModel->load("gizmo_center", app, RESOURCE_DIR "/gizmo/gizmo_center.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->mTransform.getLocalTransform();
            mModel->setTransparent(false);

            // if mesh in node animated
            mModel->mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
                .setSize(20 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(app);

            auto& databuffer = mModel->mGlobalMeshTransformationData;
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mGlobalMeshTransformationBuffer.getBuffer(),
                                 0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            auto** gizmo_part = static_cast<BaseModel**>(params);
            *gizmo_part = this->mModel;
            (void)gizmo_part;  // Suppress unused variable warning
        };
};

REGISTER_MODEL("gizmo_center", GizmoModelCenter, Visibility_Editor, &GizmoElement::center);
REGISTER_MODEL("gizmo", GizmoModel, Visibility_Editor, &GizmoElement::z);
REGISTER_MODEL("gizmo_x", GizmoModelX, Visibility_Editor, &GizmoElement::x);
REGISTER_MODEL("gizmo_y", GizmoModelY, Visibility_Editor, &GizmoElement::y);
// REGISTER_MODEL("bone", BoneModel, Visibility_Editor, &Editor::BoneIndicator);

void Editor::showBoneAt(const glm::mat4& transformation) {
    auto [trans, scale, rot] = decomposeTransformation(transformation);
    BoneIndicator->moveTo(trans);
    // BoneIndicator->scale(scale);
    BoneIndicator->rotate(rot);
}

Screen::Screen() {}

void Screen::initialize(Application* app) {
    Screen::instance().mApp = app;
    auto& input_manager = InputManager::instance();
    input_manager.mMouseMoveListeners.push_back(&Screen::instance());
    input_manager.mMouseButtonListeners.push_back(&Screen::instance());
    input_manager.mMouseScrollListeners.push_back(&Screen::instance());
    input_manager.mKeyListener.push_back(&Screen::instance());
}

Screen& Screen::instance() {
    static Screen instance;
    return instance;
}

void Screen::onMouseMove(MouseEvent event) {
    auto move = std::get<Move>(event);
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(move.window));

    mApp = that;
    if (that->mWorld->actor != nullptr) {
        return;
    }
    if (mApp != nullptr) {
        DragState& drag_state = mApp->getCamera().getDrag();
        if (drag_state.active) mApp->getCamera().processMouse(move.xPos, move.yPos);
    }

    auto [window_width, window_height] = that->getWindowSize();
    if (move.yPos <= 0) {
        InputManager::instance().setCursorPosition(move.window, move.xPos, window_height - 1);
        if (that != nullptr) that->getCamera().updateCursor(move.xPos, window_height - 1);
    } else if (move.yPos > window_height - 20) {
        InputManager::instance().setCursorPosition(move.window, move.xPos, 1);
        if (that != nullptr) that->getCamera().updateCursor(move.xPos, 1);
    }
    if (move.xPos <= 0) {
        InputManager::instance().setCursorPosition(move.window, window_width - 1, move.yPos);
        if (that != nullptr) that->getCamera().updateCursor(window_width - 1, move.yPos);
    } else if (move.xPos >= window_width - 1) {
        InputManager::instance().setCursorPosition(move.window, 1, move.yPos);
        if (that != nullptr) that->getCamera().updateCursor(1, move.yPos);
    }
}

void Screen::onMouseClick(MouseEvent event) {
    auto click = std::get<Click>(event);
    mApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(click.window));

    if (mApp == nullptr || mApp->mWorld->actor != nullptr) {
        return;
    }

    DragState& drag_state = mApp->getCamera().getDrag();
    // if (drag_state.active) mApp->getCamera().processMouse(click.xPos, click.yPos);

    if (click.click == GLFW_MOUSE_BUTTON_LEFT) {
        // calculating the NDC for x and y
        auto [w_width, w_height] = mApp->getWindowSize();
        double xpos, ypos;
        glfwGetCursorPos(click.window, &xpos, &ypos);

        if (click.action == GLFW_PRESS) {
            if (GizmoElement::mSelectedPart == GizmoElement::center || GizmoElement::mSelectedPart == GizmoElement::x ||
                GizmoElement::mSelectedPart == GizmoElement::y || GizmoElement::mSelectedPart == GizmoElement::z) {
                GizmoElement::mIsLocked = true;
                return;
            }
            auto intersected_model = testIntersection(
                mApp->getCamera(), w_width, w_height, {xpos, ypos},
                ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User), {&mApp->debugbox});
            if (!std::holds_alternative<std::monostate>(intersected_model)) {
                if (std::holds_alternative<BaseModel*>(intersected_model)) {
                    if (mApp->mSelectedModel) {
                        mApp->mSelectedModel->selected(false);
                    }
                    mApp->mSelectedModel = std::get<BaseModel*>(intersected_model);
                    mApp->mSelectedModel->selected(true);
                    mApp->mEditor->mSelectedObject = mApp->mSelectedModel;
                } else if (std::holds_alternative<DebugBox*>(intersected_model)) {
                    mApp->mEditor->mSelectedObject = std::get<DebugBox*>(intersected_model);
                }
                auto& editor_selected = mApp->mEditor->mSelectedObject;

                if (std::holds_alternative<BaseModel*>(editor_selected)) {
                    if (GizmoElement::z != nullptr && GizmoElement::x != nullptr && GizmoElement::y != nullptr &&
                        GizmoElement::center != nullptr) {
                        auto [min, max] = std::get<BaseModel*>(editor_selected)->getWorldSpaceAABB();
                        auto center = (min + max) / glm::vec3{2.0f};
                        GizmoElement::moveTo(center);
                    } else {
                        std::cout << "Couldnt find the gizmo model " << mApp->mSelectedModel->getName() << std::endl;
                    }
                } else if (std::holds_alternative<physics::BoxCollider*>(editor_selected)) {
                } else if (std::holds_alternative<DebugBox*>(editor_selected)) {
                    auto* box = std::get<DebugBox*>(intersected_model);
                    GizmoElement::moveTo(box->center);
                }
            }
        }
    } else if (click.click == GLFW_MOUSE_BUTTON_RIGHT) {
        switch (click.action) {
            case GLFW_PRESS: {
                drag_state.active = true;
                drag_state.firstMouse = true;
                break;
            }
            case GLFW_RELEASE: {
                drag_state.active = false;
                drag_state.firstMouse = false;
                /*if (mSelectedModel != nullptr) {*/
                /*    mSelectedModel->selected(false);*/
                /*    mSelectedModel = nullptr;*/
                /*}*/
                break;
            }
        }
    }
}

void Screen::onMouseScroll(MouseEvent event) {
    auto scroll = std::get<Scroll>(event);
    mApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(scroll.window));
    if (mApp != nullptr) mApp->getCamera().processScroll(scroll.yOffset);
}

void Screen::onKey(KeyEvent event) {
    auto key = std::get<Keyboard>(event);

    mApp = reinterpret_cast<Application*>(glfwGetWindowUserPointer(key.window));
    mApp->getCamera().processInput(key.key, key.scancode, key.action, key.mods);

    if (GLFW_KEY_KP_0 == key.key && GLFW_PRESS == key.action) {
    } else if (GLFW_KEY_KP_1 == key.key && GLFW_PRESS == key.action) {
        // look_as_light = !look_as_light;

    } else if (GLFW_KEY_KP_2 == key.key && key.action == GLFW_PRESS) {
    } else if (GLFW_KEY_KP_3 == key.key && key.action == GLFW_PRESS) {
    } else if (GLFW_KEY_ESCAPE == key.key && key.action == GLFW_PRESS) {
        std::cout << "escaped pressed!\n";
        mApp->mWorld->togglePlayer();
        if (mApp->mWorld->actor != nullptr) {
            glfwSetInputMode(key.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(key.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        } else {
            glfwSetInputMode(key.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported()) glfwSetInputMode(key.window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
    }
}
