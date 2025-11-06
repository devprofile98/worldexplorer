
#ifndef WORLD_EXPLORER_WORLD_H
#define WORLD_EXPLORER_WORLD_H

#include <vector>

#include "model.h"

struct World {
        std::vector<Model*> rootContainer;
        Model* makeChild(Model* parent, Model* child);
        void removeParent(Model* child);
};

#endif  //! WORLD_EXPLORER_WORLD_H
