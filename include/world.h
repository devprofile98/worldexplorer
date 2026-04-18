
#ifndef WORLD_EXPLORER_WORLD_H
#define WORLD_EXPLORER_WORLD_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "animation.h"
#include "application.h"
#include "glm/ext/vector_float3.hpp"
#include "input_manager.h"
#include "model.h"

using Vec = std::array<float, 3>;
using MaterialList = std::vector<std::pair<std::string, std::string>>;

struct MaterialProperties {
        std::string meshName;
        std::array<float, 2> uv;
};

using MaterialPropsMap = std::unordered_map<std::string, MaterialProperties>;

struct ColliderParams {
        std::string name;
        std::string type;
        std::string shape;
        glm::vec3 center;
        glm::vec3 halfExtent;
};

struct SocketParams {
        std::string name;
        std::string anchor;
        glm::vec3 translate;
        glm::vec3 scale;
        glm::quat rotate;
        bool isValid;
        AnchorType type;
};

enum class PhysicGenMethod {
    AABB = 0,
    MESH = 1,
    CUSTOM = 2,
};

struct PhysicsParams {
        std::string type;
        bool isSensor = false;
        PhysicGenMethod method = PhysicGenMethod::AABB;
};

struct InstanceInfo {
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
        bool hasPhysics;
};

struct ObjectLoaderParam {
        SocketParams socketParam;
        PhysicsParams physicsParams;
        std::vector<InstanceInfo> instanceTransformations;
        int type;
        std::string name;
        std::string path;
        bool animated;
        bool isVisible;
        CoordinateSystem cs;
        glm::vec3 translate;
        glm::vec3 scale;
        glm::vec3 rotate;
        std::vector<std::string> childrens;
        std::string defaultClip;
        bool isDefaultActor = false;
        bool isPhysicEnabled = false;
        MaterialList materialList;
        MaterialPropsMap matPropMap;

        ObjectLoaderParam(int type, std::string name, std::string path, bool animated, bool isVisible,
                          CoordinateSystem cs, Vec translate, Vec scale, Vec rotate, std::vector<std::string> childrens,
                          std::string defaultClip, SocketParams socketParam, MaterialList matList,
                          MaterialPropsMap matPropMap);
};

struct World : public KeyboardListener, MouseMoveListener, MouseButtonListener, MouseScrollListener {
        World(Application* core, const std::string& sceneFilePath);
        Model* makeChild(BaseModel* parent, BaseModel* child);
        void removeParent(BaseModel* child);
        void onNewModel(Model* model);
        BaseModel* actor = nullptr;
        std::string actorName = "";
        float delta = 0.16;

        void togglePlayer();
        void loadWorld();
        void loadModel(const ObjectLoaderParam& param);

        void onKey(KeyEvent event) override;
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
        void onMouseScroll(MouseEvent event) override;
        void exportScene();

        std::vector<BaseModel*> rootContainer;
        std::unordered_map<std::string, ObjectLoaderParam> map;

        Application* app;

    private:
        std::string mSceneFilePath;
};

#endif  //! WORLD_EXPLORER_WORLD_H
