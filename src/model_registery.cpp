#include "model_registery.h"

#include <chrono>
#include <future>

#include "application.h"
#include "camera.h"
#include "world.h"

void InputHandler::handleKey(BaseModel*, KeyEvent, float) {}
void InputHandler::handleMouseMove(BaseModel*, MouseEvent) {}
void InputHandler::handleMouseClick(BaseModel*, MouseEvent) {}
void InputHandler::handleMouseScroll(BaseModel*, MouseEvent) {}
void InputHandler::handleAttachedCamera(BaseModel*, Camera*) {}
// void InputHandler::update(BaseModel*, float) {}

void PawnBehaviour::onLoad(Model*) {}
void PawnBehaviour::onEquip(Model*, Model*) {}
void PawnBehaviour::onUnequip(Model*, Model*) {}
void PawnBehaviour::onTick(Model*, float dt) { (void)dt; }
void PawnBehaviour::handleKey(BaseModel* model, KeyEvent event, float dt) {}

glm::vec3 InputHandler::getForward() { return glm::vec3{0.0}; }
BaseModel* InputHandler::getWeapon() { return nullptr; }

ModelRegistry& ModelRegistry::instance() {
    static ModelRegistry inst;
    return inst;
}

void ModelRegistry::registerModel(const std::string& name, FactoryFunc func) { factories[name] = func; }

void ModelRegistry::registerInputHandler(const std::string& name, InputHandler* inputHandler) {
    inputHandlerMap[name] = inputHandler;
}

;
void ModelRegistry::registerBehaviour(const std::string& name, PawnBehaviour* behaviour) {
    pawnBehaviourMap[name] = behaviour;
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
            if (inputHandlerMap.contains(model.model->mName)) {
                std::cout << "Behaviour for this  exists " << model.model->mName << '\n';
                model.model->mInputHandler = inputHandlerMap[model.model->mName];
                model.model->mInputHandler->app = app;
                // model.model->mInputHandler->onModelLoad(model.model);
            }
            if (pawnBehaviourMap.contains(model.model->mName)) {
                std::cout << "Behaviour for this  exists " << model.model->mName << '\n';
                model.model->mBehaviour = pawnBehaviourMap[model.model->mName];
                model.model->mBehaviour->app = app;
                model.model->mBehaviour->onLoad(model.model);
                model.model->mBehaviour->model = model.model;
            }
            it = futures.erase(it);
        } else {
            it++;
        }
    }
}
