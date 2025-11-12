#include "model_registery.h"

#include <chrono>
#include <future>

#include "application.h"
#include "camera.h"
#include "world.h"

void Behaviour::sayHello() { std::cout << "hello from " << name << '\n'; }
void Behaviour::handleKey(Model* model, KeyEvent event, float dt) {}
void Behaviour::handleMouseMove(Model* model, MouseEvent event) {}
void Behaviour::handleMouseClick(Model* model, MouseEvent event) {}
void Behaviour::handleMouseScroll(Model* model, MouseEvent event) {}
void Behaviour::handleAttachedCamera(Model* model, Camera* camera) {}
void Behaviour::update(float dt) {}
glm::vec3 Behaviour::getForward() { return glm::vec3{0.0}; }

ModelRegistry& ModelRegistry::instance() {
    static ModelRegistry inst;
    return inst;
}

void ModelRegistry::registerModel(const std::string& name, FactoryFunc func) { factories[name] = func; }

void ModelRegistry::registerBehaviour(const std::string& name, Behaviour* behaviour) {
    std::cout << "Addinng new behaviour for " << name << '\n';

    behaviourMap[name] = behaviour;
}

ModelRegistry::ModelContainer& ModelRegistry::getLoadedModel(ModelVisibility visibility) {
    if (visibility == Visibility_User) {
        return mUserLoadedModel;
    } else {
        return mEditorLoadedModel;
    }
}

void ModelRegistry::tick(Application* app) {
    if (factories.size() > 0) {
        auto it = factories.begin();
        futures.push_back(std::async(std::launch::async, it->second, app));
        factories.erase(it);
    }

    for (auto it = futures.begin(); it != futures.end();) {
        if (it->wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready) {
            LoadModelResult model = it->get();
            std::cout << "Model loaded with visibility " << model.visibility << " " << std::endl;
            if (model.visibility == Visibility_User) {
                mUserLoadedModel.push_back(model.model);
            } else if (model.visibility == Visibility_Editor) {
                mEditorLoadedModel.push_back(model.model);
            } else if (model.visibility == Visibility_Other) {
                // Do not add this model to any container
            }
            if (model.visibility != Visibility_Editor) {
                app->mWorld->onNewModel(model.model);
            }
            std::cout << "Searching for Behaviour for: " << model.model->mName << '\n';
            if (behaviourMap.contains(model.model->mName)) {
                std::cout << "Behaviour for this  exists " << model.model->mName << '\n';
                model.model->mBehaviour = behaviourMap[model.model->mName];
            }
            it = futures.erase(it);
        } else {
            it++;
        }
    }
}
