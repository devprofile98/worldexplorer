
#include "editor.h"

#include <random>

#include "application.h"
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
