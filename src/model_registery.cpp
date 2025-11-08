#include "model_registery.h"

#include <chrono>
#include <future>

#include "application.h"
#include "world.h"

ModelRegistry& ModelRegistry::instance() {
    static ModelRegistry inst;
    return inst;
}

void ModelRegistry::registerModel(const std::string& name, FactoryFunc func) { factories[name] = func; }

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
            it = futures.erase(it);
        } else {
            it++;
        }
    }
}
