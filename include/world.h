
#ifndef WORLD_EXPLORER_WORLD_H
#define WORLD_EXPLORER_WORLD_H

#include <string>
#include <unordered_map>
#include <vector>

#include "application.h"
#include "input_manager.h"
#include "model.h"

using Vec = std::array<float, 3>;

struct SocketParams {
        std::string name;
        std::string bone;
        glm::vec3 translate;
        glm::vec3 scale;
        glm::quat rotate;
        bool isValid;
};

struct PhysicsParams {
        std::string type;
};

struct ObjectLoaderParam {
        SocketParams socketParam;
        PhysicsParams physicsParams;
        std::string name;
        std::string path;
        bool animated;
        CoordinateSystem cs;
        glm::vec3 translate;
        glm::vec3 scale;
        glm::vec3 rotate;
        std::vector<std::string> childrens;
        std::string defaultClip;
        bool isDefaultActor = false;
        bool isPhysicEnabled = false;

        ObjectLoaderParam(std::string name, std::string path, bool animated, CoordinateSystem cs, Vec translate,
                          Vec scale, Vec rotate, std::vector<std::string> childrens, std::string defaultClip,
                          SocketParams socketParam);
};

struct World : public KeyboardListener, MouseMoveListener, MouseButtonListener, MouseScrollListener {
        World(Application* core);
        Model* makeChild(Model* parent, Model* child);
        void removeParent(Model* child);
        void onNewModel(Model* model);
        Model* actor = nullptr;
        std::string actorName = "";
        float delta = 0.16;

        void togglePlayer();
        void loadWorld();

        void onKey(KeyEvent event) override;
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
        void onMouseScroll(MouseEvent event) override;

        std::vector<Model*> rootContainer;
        std::unordered_map<std::string, ObjectLoaderParam> map;

        Application* app;
};

#endif  //! WORLD_EXPLORER_WORLD_H
