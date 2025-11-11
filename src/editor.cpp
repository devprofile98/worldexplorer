
#include "editor.h"

#include <random>
#include <variant>

#include "GLFW/glfw3.h"
#include "application.h"
#include "glm/gtx/string_cast.hpp"
#include "input_manager.h"
#include "model_registery.h"
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

void GizmoElement::onMouseMove(MouseEvent event) {
    auto move = std::get<Move>(event);
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(move.window));

    // if (that != nullptr) {
    //     DragState& drag_state = that->getCamera().getDrag();
    //     if (drag_state.active) that->getCamera().processMouse(move.xPos, move.yPos);
    // }

    auto [width, height] = that->getWindowSize();
    auto intersected_model = GizmoElement::testSelection(that->getCamera(), width, height, {move.xPos, move.yPos});

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
    } else if (that->mSelectedModel != nullptr && GizmoElement::mSelectedPart != nullptr) {
        glm::vec3 intersection_box_min;
        glm::vec3 intersection_box_max;
        float scalar = 100.0f;
        auto& pos = that->mSelectedModel->mTransform.getPosition();
        bool happend = false;
        if (GizmoElement::mSelectedPart == GizmoElement::x) {
            intersection_box_min = {-scalar, pos.y, -scalar};
            intersection_box_max = {scalar, pos.y + 1.0, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo({data.x, pos.y, pos.z});
            happend = true;
        } else if (GizmoElement::mSelectedPart == GizmoElement::y) {
            // std::cout << "the Result for box intersection is "<< std::endl;
            intersection_box_min = {pos.x, -scalar, -scalar};
            intersection_box_max = {pos.x + 1, scalar, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo({pos.x, data.y, pos.z});
            happend = true;
        } else if (GizmoElement::mSelectedPart == GizmoElement::z) {
            intersection_box_min = {-scalar, pos.y, -scalar};
            intersection_box_max = {scalar, pos.y + 1.0, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo({pos.x, pos.y, data.z});
            happend = true;
        }

        if (happend) {
            auto [min, max] = that->mSelectedModel->getWorldSpaceAABB();
            GizmoElement::moveTo((max + min) / glm::vec3{2.0});
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
                .scale(glm::vec3{.1f});
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
            auto** gizmo_part = static_cast<BaseModel**>(params);
            *gizmo_part = this->mModel;
        };
};

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
REGISTER_MODEL("bone", BoneModel, Visibility_Editor, &Editor::BoneIndicator);

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

    if (mApp == nullptr) {
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
            auto intersected_model =
                testIntersection(mApp->getCamera(), w_width, w_height, {xpos, ypos},
                                 ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User));
            if (intersected_model != nullptr) {
                if (mApp->mSelectedModel) {
                    mApp->mSelectedModel->selected(false);
                }
                mApp->mSelectedModel = intersected_model;
                mApp->mSelectedModel->selected(true);
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
    }
}
