

#ifndef WEBGPUTEST_MODEL_REGISTERY_H
#define WEBGPUTEST_MODEL_REGISTERY_H

#include <functional>
#include <future>
#include <glm/fwd.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "input_manager.h"

class Model;  // Forward declaration
class Application;
class Camera;

/* Base class for element behaviours, for example: models can inherit from it or have a component of this type to be
 * able to handle Input events */
class Behaviour {
    public:
        virtual void sayHello();
        virtual void handleKey(Model* model, KeyEvent event, float dt);
        virtual void handleMouseMove(Model* model, MouseEvent event);
        virtual void handleMouseClick(Model* model, MouseEvent event);
        virtual void handleMouseScroll(Model* model, MouseEvent event);
        virtual void handleAttachedCamera(Model* model, Camera* camera);
        virtual void update(Model* model, float dt);
        virtual glm::vec3 getForward();

        std::string name;
};

enum ModelVisibility { Visibility_Editor = 0, Visibility_User = 1, Visibility_Other = 100 };

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
        using ModelContainer = std::vector<Model*>;

        static ModelRegistry& instance();
        void registerModel(const std::string& name, FactoryFunc func);
        void registerBehaviour(const std::string& name, Behaviour* behaviour);
        void tick(Application* app);

        std::unordered_map<std::string, FactoryFunc> factories;
        ModelContainer& getLoadedModel(ModelVisibility visibility);

    private:
        ModelContainer mUserLoadedModel;
        ModelContainer mEditorLoadedModel;
        std::vector<std::future<LoadModelResult>> futures;
        std::unordered_map<std::string, Behaviour*> behaviourMap;
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
