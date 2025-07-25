

#ifndef WEBGPUTEST_MODEL_REGISTERY_H
#define WEBGPUTEST_MODEL_REGISTERY_H

#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

class Model;  // Forward declaration
class Application;

enum ModelVisibility { Visibility_Editor = 0, Visibility_User = 1 };

struct LoadModelResult {
        Model* model;
        ModelVisibility visibility;
};

class IModel {
    public:
        virtual Model* getModel() = 0;
        virtual void onLoad(Application* app, void* params = nullptr) = 0;

    protected:
        Model* mModel;
};

class ModelRegistry {
    public:
        using FactoryFunc = std::function<LoadModelResult(Application* app)>;
        using ModelContainer = std::unordered_map<std::string, Model*>;

        static ModelRegistry& instance();
        void registerModel(const std::string& name, FactoryFunc func);
        void tick(Application* app);

        std::vector<std::string> getRegisteredNames() const;
        // LoadModelResult create(Application* app, const std::string& name) const;

        std::unordered_map<std::string, FactoryFunc> factories;
        ModelContainer& getLoadedModel(ModelVisibility visibility);

    private:
        // ModelContainer mLoadedModel;
        std::unordered_map<ModelVisibility, ModelContainer> mLoadedModels;
        std::vector<std::future<LoadModelResult>> futures;
};

#define USER_REGISTER_MODEL(NAME, TYPE) REGISTER_MODEL(NAME, TYPE, Visibility_User, nullptr)
#define REGISTER_MODEL(NAME, TYPE, VISIBILITY, Param)                                                   \
    namespace {                                                                                         \
    struct TYPE##Registrar {                                                                            \
            TYPE##Registrar() {                                                                         \
                ModelRegistry::instance().registerModel(NAME, [](Application* app) -> LoadModelResult { \
                    TYPE model = TYPE{app};                                                             \
                    model.onLoad(app, Param);                                                           \
                    return {model.getModel(), VISIBILITY};                                              \
                });                                                                                     \
            }                                                                                           \
    };                                                                                                  \
    static TYPE##Registrar global_##TYPE##Registrar;                                                    \
    }

#endif  // WEBGPUTEST_MODEL_REGISTERY_H
