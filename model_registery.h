

#ifndef WEBGPUTEST_MODEL_REGISTERY_H
#define WEBGPUTEST_MODEL_REGISTERY_H

#include <functional>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>

class Model;  // Forward declaration
class Application;

class IModel {
    public:
        virtual Model* getModel() = 0;
        virtual void onLoad(Application* app) = 0;

    protected:
        Model* mModel;
};

class ModelRegistry {
    public:
        using FactoryFunc = std::function<Model*(Application* app)>;

        static ModelRegistry& instance();
        void registerModel(const std::string& name, FactoryFunc func);
        void tick(Application* app);

        std::vector<std::string> getRegisteredNames() const;
        Model* create(Application* app, const std::string& name) const;

        std::unordered_map<std::string, FactoryFunc> factories;

    private:
        std::unordered_map<std::string, Model*> mLoadedModel;
        std::vector<std::future<Model*>> futures;
};

#define REGISTER_MODEL(NAME, TYPE)                                                             \
    namespace {                                                                                \
    struct TYPE##Registrar {                                                                   \
            TYPE##Registrar() {                                                                \
                ModelRegistry::instance().registerModel(NAME, [](Application* app) -> Model* { \
                    TYPE model = TYPE{app};                                                    \
                    model.onLoad(app);                                                         \
                    return model.getModel();                                                   \
                });                                                                            \
            }                                                                                  \
    };                                                                                         \
    static TYPE##Registrar global_##TYPE##Registrar;                                           \
    }

#endif  // WEBGPUTEST_MODEL_REGISTERY_H
