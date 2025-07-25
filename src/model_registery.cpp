#include "model_registery.h"

#include <chrono>
#include <future>

#include "application.h"

ModelRegistry& ModelRegistry::instance() {
    static ModelRegistry inst;
    return inst;
}

void ModelRegistry::registerModel(const std::string& name, FactoryFunc func) { factories[name] = func; }

std::vector<std::string> ModelRegistry::getRegisteredNames() const {
    std::vector<std::string> names;
    for (const auto& kv : factories) names.push_back(kv.first);
    return names;
}

// LoadModelResult ModelRegistry::create(Application* app, const std::string& name) const {
//     auto it = factories.find(name);
//     if (it != factories.end()) return it->second(app);
//     return {nullptr, Visibility_User};
// }
//
//

ModelRegistry::ModelContainer& ModelRegistry::getLoadedModel(ModelVisibility visibility) {
    return mLoadedModels[visibility];
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
            auto &container = mLoadedModels[model.visibility];
	    std::cout << "Model loaded with visibility " << model.visibility <<" " << container.size()<< std::endl;
            container[model.model->getName()] = model.model;
            it = futures.erase(it);
        } else {
            it++;
        }
    }
}
