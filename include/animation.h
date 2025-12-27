
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
#include "model.h"

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
        bool hasSkining = false;
};

struct Action {
        std::unordered_map<std::string, glm::mat4> calculatedTransform;
        std::unordered_map<std::string, glm::mat4> localTransformation;
        std::unordered_map<std::string, Bone*> Bonemap;
        double mAnimationSecond = 0.0;
        double mAnimationDuration = 0.0;
        bool loop = false;
        bool hasSkining = false;
};

struct Animation {
        std::vector<glm::mat4> mFinalTransformations;
        std::unordered_map<std::string, Action*> actions;
        size_t activeActionIdx;
        Action* activeAction = nullptr;

        glm::mat4 getLocalTransformAtTime(const aiNode* node, double time);

        void computeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                                     std::unordered_map<std::string, glm::mat4>& outGlobalMap,
                                     std::unordered_map<std::string, glm::mat4>& outLocalMap);

        bool initAnimation(const aiScene* scene, std::string name);
        void update(aiNode* root);
        Action* getActiveAction();
        Action* getAction(const std::string& actionName);
        void playAction(const std::string& name, bool loop = false);
        bool isEnded() const;
};

struct BoneSocket {
        Model* model;
        std::string boneName;
        glm::vec3 positionOffset;
        glm::vec3 scaleOffset;
        glm::quat rotationOffset;
};

#endif  //! WORLD_EXPLORER_ANIMATION_H
