#include "model_registery.h"

#include <chrono>
#include <future>

#include "application.h"

ModelRegistry& ModelRegistry::instance() {
    static ModelRegistry inst;
    return inst;
}

void ModelRegistry::registerModel(const std::string& name, FactoryFunc func) { factories[name] = func; }

// std::vector<std::string> ModelRegistry::getRegisteredNames() const {
//     std::vector<std::string> names;
//     for (const auto& kv : factories) names.push_back(kv.first);
//     return names;
// }

// LoadModelResult ModelRegistry::create(Application* app, const std::string& name) const {
//     auto it = factories.find(name);
//     if (it != factories.end()) return it->second(app);
//     return {nullptr, Visibility_User};
// }
//
//

ModelRegistry::ModelContainer& ModelRegistry::getLoadedModel(ModelVisibility visibility) {
    // return mLoadedModels[visibility];
    if (visibility == Visibility_User) {
        return mUserLoadedModel;
    } else /*if (visibility == Visibility_Editor)*/ {
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
        if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            LoadModelResult model = it->get();
            std::cout << "Model loaded with visibility " << model.visibility << " " << std::endl;
            if (model.visibility == Visibility_User) {
                mUserLoadedModel.push_back(model.model);
            } else if (model.visibility == Visibility_Editor) {
                mEditorLoadedModel.push_back(model.model);
            }
            // auto& container = mLoadedModels[model.visibility];
            // container[model.model->getName()] = model.model;
            // container.push_back(model.model);
            it = futures.erase(it);
        } else {
            it++;
        }
    }
}
