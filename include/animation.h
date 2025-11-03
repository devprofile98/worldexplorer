
#ifndef WORLD_EXPLORER_ANIMATION_H
#define WORLD_EXPLORER_ANIMATION_H

#include <glm/fwd.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "glm/gtc/type_ptr.hpp"

glm::mat4 AiToGlm(const aiMatrix4x4& aiMat);

template <typename T>
struct Keyframe {
        float time;
        T value;  // could be vec3, quat
};

struct AnimationChannel {
        std::vector<Keyframe<glm::vec3>> translations;
        std::vector<Keyframe<glm::quat>> quats;
        std::vector<Keyframe<glm::vec3>> scales;
};

struct Bone {
        int id;
        std::string name;
        glm::mat4 offsetMatrix;
        int parentIndex = -1;
        std::vector<int> childrens;
        AnimationChannel channel;
};

struct Action {
        std::map<std::string, glm::mat4> calculatedTransform;
        std::map<std::string, Bone*> Bonemap;
        double mAnimationSecond = 0.0;
        double mAnimationDuration = 0.0;
};

struct Animation {
        std::vector<glm::mat4> mFinalTransformations;
        std::vector<Action*> actions;
        size_t activeActionIdx;

        glm::mat4 getLocalTransformAtTime(const aiNode* node, double time);

        void computeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                                     std::map<std::string, glm::mat4>& outGlobalMap);

        bool initAnimation(const aiScene* scene);
        void update(aiNode* root);
        Action* getActiveAction();
        Action* nextAction();
};

#endif  //! WORLD_EXPLORER_ANIMATION_H
