
#ifndef WORLD_EXPLORER_WORLD_H
#define WORLD_EXPLORER_WORLD_H

#include <unordered_map>
#include <vector>

#include "model.h"

using Vec = std::array<float, 3>;
struct ObjectLoaderParam {
        std::string name;
        std::string path;
        bool animated;
        CoordinateSystem cs;
        glm::vec3 translate;
        glm::vec3 scale;
        glm::vec3 rotate;
        std::vector<std::string> childrens;

        ObjectLoaderParam(std::string name, std::string path, bool animated, CoordinateSystem cs, Vec translate,
                          Vec scale, Vec rotate, std::vector<std::string> childrens);
};

struct World {
        Model* makeChild(Model* parent, Model* child);
        void removeParent(Model* child);
        void onNewModel(Model* model);

        void loadWorld();

        std::vector<Model*> rootContainer;
        std::unordered_map<std::string, ObjectLoaderParam> map;
};

#endif  //! WORLD_EXPLORER_WORLD_H
