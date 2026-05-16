

#ifndef WEBGPUTEST_MODEL_REGISTERY_H
#define WEBGPUTEST_MODEL_REGISTERY_H

#include <functional>
#include <future>
#include <glm/fwd.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "input_manager.h"

class Model;      // Forward declaration
class BaseModel;  // Forward declaration
class Application;
class Camera;

/* Base class for element behaviours, for example: models can inherit from it or have a component of this type to be
 * able to handle Input events */
class InputHandler {
    public:
        virtual void handleKey(BaseModel* model, KeyEvent event, float dt);
        virtual void handleMouseMove(BaseModel* model, MouseEvent event);
        virtual void handleMouseClick(BaseModel* model, MouseEvent event);
        virtual void handleMouseScroll(BaseModel* model, MouseEvent event);
        virtual void handleAttachedCamera(BaseModel* model, Camera* camera);
        // virtual void update(BaseModel* model, float dt);
        virtual glm::vec3 getForward();
        virtual BaseModel* getWeapon();

        std::string name;
        Application* app;
};

class PawnBehaviour {
    public:
        virtual void onLoad(Model* model);
        virtual void onEquip(Model* weapon, Model* target);
        virtual void onUnequip(Model* weapon, Model* target);
        virtual void onPick(Model* weapon, Model* target);
        virtual void onTick(Model*, float dt);
        virtual void handleKey(BaseModel* model, KeyEvent event, float dt);

        std::string name;
        Application* app;

        // the maximum distance that the user should be able to pick, negative for not pickable
        float maxPickDistance = -1.0;
        Model* model = nullptr;
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
        void registerInputHandler(const std::string& name, InputHandler* inputHandler);
        void registerBehaviour(const std::string& name, PawnBehaviour* behaviour);
        void tick(Application* app);

        std::unordered_map<std::string, FactoryFunc> factories;
        ModelContainer& getLoadedModel(ModelVisibility visibility);

    private:
        ModelContainer mUserLoadedModel;
        ModelContainer mEditorLoadedModel;
        std::vector<std::future<LoadModelResult>> futures;
        std::unordered_map<std::string, InputHandler*> inputHandlerMap;
        std::unordered_map<std::string, PawnBehaviour*> pawnBehaviourMap;
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
