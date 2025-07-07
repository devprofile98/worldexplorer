
#include "editor.h"

#include <random>
#include <variant>

#include "application.h"
#include "input_manager.h"
#include "model_registery.h"
#include "utils.h"

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
    if (click.action == GLFW_PRESS) {
        if (GizmoElement::mSelectedPart == GizmoElement::center || GizmoElement::mSelectedPart == GizmoElement::x ||
            GizmoElement::mSelectedPart == GizmoElement::y || GizmoElement::mSelectedPart == GizmoElement::z) {
            GizmoElement::mIsLocked = true;
            std::cout << "Gizmo is locked in event handlerln " << click.click << std::endl;
            return;
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
        if (GizmoElement::mSelectedPart == GizmoElement::x) {
            intersection_box_min = {-scalar, that->mSelectedModel->getPosition().y, -scalar};
            intersection_box_max = {scalar, that->mSelectedModel->getPosition().y + 1.0, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo(
                {data.x, that->mSelectedModel->getPosition().y, that->mSelectedModel->getPosition().z});
        } else if (GizmoElement::mSelectedPart == GizmoElement::y) {
            // std::cout << "the Result for box intersection is "<< std::endl;
            intersection_box_min = {that->mSelectedModel->getPosition().x, -scalar, -scalar};
            intersection_box_max = {that->mSelectedModel->getPosition().x + 1, scalar, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo(
                {that->mSelectedModel->getPosition().x, data.y, that->mSelectedModel->getPosition().z});
        } else if (GizmoElement::mSelectedPart == GizmoElement::z) {
            intersection_box_min = {-scalar, that->mSelectedModel->getPosition().y, -scalar};
            intersection_box_max = {scalar, that->mSelectedModel->getPosition().y + 1.0, scalar};
            auto [res, data] = testIntersectionWithBox(that->getCamera(), width, height, {move.xPos, move.yPos},
                                                       intersection_box_min, intersection_box_max);
            that->mSelectedModel->moveTo(
                {that->mSelectedModel->getPosition().x, that->mSelectedModel->getPosition().y, data.z});
        }

        auto [min, max] = that->mSelectedModel->getWorldSpaceAABB();
        GizmoElement::moveTo((max + min) / glm::vec3{2.0});
    }
}

struct GizmoModel : public IModel {
        GizmoModel(Application* app) {
            mModel = new Model{};

            mModel->load("gizmo", app, RESOURCE_DIR "/gizmo/gizmo_up.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                //
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->getTranformMatrix();
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
                //
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->getTranformMatrix();
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

struct GizmoModelX : public IModel {
        GizmoModelX(Application* app) {
            mModel = new Model{};

            mModel->load("gizmo_x", app, RESOURCE_DIR "/gizmo/gizmo_x.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                //
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->getTranformMatrix();
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
                //
                .scale(glm::vec3{0.1f});
            mModel->rotate({90.0f, 0.0, 180.0f}, 0.0f);
            mModel->uploadToGPU(app);
            mModel->getTranformMatrix();
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

Screen::Screen() {}

void Screen::initialize(Application* app) { Screen::instance().mApp = app; }

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
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(click.window));

    mApp = that;
    if (mApp == nullptr) {
        return;
    }
    DragState& drag_state = mApp->getCamera().getDrag();
    if (drag_state.active) mApp->getCamera().processMouse(click.xPos, click.yPos);
    if (click.click == GLFW_MOUSE_BUTTON_RIGHT) {
        switch (click.action) {
            case GLFW_PRESS:
                drag_state.active = true;
                drag_state.firstMouse = true;
                break;
            case GLFW_RELEASE:
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
