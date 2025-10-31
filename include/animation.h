
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
// struct Bone {
//         std::string mName;
//         glm::mat4 mOffsetMatrix;
//         glm::mat4 mLocalTransformation;
//         std::vector<Bone*> mChildrens;
//
//         glm::mat4 getFinalTransform();
// };
//
// struct Animation {
//         float mDuration;
//         float mTickPerSec;
//
//         static Bone parseAnimationData(aiScene scene);
//         std::vector<glm::mat4> mFinalTransformations;
//         std::map<std::string, size_t> mBoneToIdx;
//         double mAnimationRunningSecond;
//         void update(double delta);
// };

struct Animation {
        std::unordered_map<std::string, glm::mat4> mOffsetMatrixCache;
        std::vector<glm::mat4> mFinalTransformations;
        std::map<std::string, size_t> boneToIdx;

        std::map<std::string, glm::mat4> globalMap;
        std::unordered_set<std::string> uniqueBones;
        std::map<std::string, aiNodeAnim*> channelMap;
        // size_t mAnimationPoseCounter = 0;
        double mAnimationSecond = 0.0;
        double mAnimationDuration = 0.0;
        // WGPUBindGroup mSkiningBindGroup = {};
        //

        glm::mat4 getLocalTransformAtTime(const aiNode* node, double time,
                                          const std::map<std::string, aiNodeAnim*>& channelMap);

        void computeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                                     const std::map<std::string, aiNodeAnim*>& channelMap,
                                     std::map<std::string, glm::mat4>& outGlobalMap);

        bool initAnimation(const aiScene* scene);
        void update(aiNode* root);
};

#endif  //! WORLD_EXPLORER_ANIMATION_H
