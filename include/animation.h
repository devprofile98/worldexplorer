
#ifndef WORLD_EXPLORER_ANIMATION_H
#define WORLD_EXPLORER_ANIMATION_H

#include <glm/fwd.hpp>
#include <string>
#include <vector>

#include "assimp/scene.h"
#include "glm/gtc/type_ptr.hpp"

struct Bone {
        std::string mName;
        glm::mat4 mOffsetMatrix;
        glm::mat4 mLocalTransformatoin;
        std::vector<Bone*> mChildrens;

        glm::mat4 getFinalTransform();
};

struct Animation {
        float mDuration;
        float mTickPerSec;

        static Bone parseAnimationData(aiScene scene);
};

#endif  //! WORLD_EXPLORER_ANIMATION_H
