
#include "animation.h"

#include <functional>
#include <iostream>

#include "assimp/anim.h"
#include "assimp/scene.h"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"

// void getFinalTransformationList

// Bone Animation::parseAnimationData(aiScene scene) {
//     if (!scene.HasAnimations()) {
//         return Bone{};
//     }
//
//     Bone root_bone{};
//
//     const aiAnimation* anim = scene.mAnimations[0];
//     for (size_t i = 0; i < anim->mNumChannels; ++i) {
//         const aiNodeAnim* channel = anim->mChannels[i];
//         // positions
//         for (size_t j = 0; j < channel->mNumPositionKeys; ++j) {
//         }
//         // rotations
//         for (size_t j = 0; j < channel->mNumRotationKeys; ++j) {
//         }
//         // scales
//         for (size_t j = 0; j < channel->mNumScalingKeys; ++j) {
//         }
//     }
//     return root_bone;
// }

glm::mat4 AiToGlm(const aiMatrix4x4& aiMat) {
    return glm::mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3,
                     aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
}

void printAnimationInfos(const std::string& name, const aiScene* scene) {
    if (scene->mAnimations != nullptr) {
        std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ There are animations " << name << " " << scene->mNumAnimations
                  << std::endl;
        for (size_t i = 0; i < scene->mNumAnimations; i++) {
            const aiAnimation* anim = scene->mAnimations[i];
            std::cout << "@@@@@@@@@@@@@@@@@@@@@ " << anim->mName.C_Str() << " " << anim->mNumChannels << std::endl;
            for (size_t j = 0; j < anim->mNumChannels; j++) {
                const aiNodeAnim* nodeAnim = anim->mChannels[j];
                std::cout << "--------------------------------" << nodeAnim->mNodeName.C_Str() << " "
                          << nodeAnim->mNumPositionKeys << " " << nodeAnim->mNumRotationKeys << " "
                          << nodeAnim->mNumScalingKeys << std::endl;
                for (size_t k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                    std::cout << std::format("at: {}\n", nodeAnim->mPositionKeys[k].mTime / anim->mTicksPerSecond);
                }
            }
        }
    }
}

inline glm::vec3 assimpToGlmVec3(aiVector3D vec) { return glm::vec3(vec.x, vec.y, vec.z); }

inline glm::quat assimpToGlmQuat(aiQuaternion quat) {
    glm::quat q;
    q.x = quat.x;
    q.y = quat.y;
    q.z = quat.z;
    q.w = quat.w;

    return q;
}

size_t findPositionKey(double time, const aiNodeAnim* channel) {
    for (size_t i = 0; i < channel->mNumPositionKeys; i++) {
        if (time < channel->mPositionKeys[i].mTime) {
            return i;
        }
    }
    return channel->mNumPositionKeys - 1;
}

size_t findScaleKey(double time, const aiNodeAnim* channel) {
    for (size_t i = 0; i < channel->mNumScalingKeys; i++) {
        if (time < channel->mScalingKeys[i].mTime) {
            return i;
        }
    }
    return channel->mNumScalingKeys - 1;
}

size_t findRotationKey(double time, const aiNodeAnim* channel) {
    for (size_t i = 0; i < channel->mNumRotationKeys; ++i) {
        if (time < channel->mRotationKeys[i].mTime) {
            return i;
        }
    }
    return channel->mNumRotationKeys - 1;
}

static void decompose(const aiMatrix4x4& m, glm::vec3& t, glm::quat& r, glm::vec3& s) {
    aiVector3D aiS, aiT;
    aiQuaternion aiR;
    m.Decompose(aiS, aiR, aiT);
    t = glm::vec3(aiT.x, aiT.y, aiT.z);
    r = glm::quat(aiR.w, aiR.x, aiR.y, aiR.z);
    s = glm::vec3(aiS.x, aiS.y, aiS.z);
}

glm::vec3 calculateInterpolatedPosition(double time, const aiNodeAnim* channel, const aiNode* node) {
    if (!channel) {
        // Static bone: extract translation from rest pose
        return glm::vec3(node->mTransformation.d1, node->mTransformation.d2, node->mTransformation.d3);
    }

    if (channel->mNumPositionKeys == 0) {
        return glm::vec3(node->mTransformation.d1, node->mTransformation.d2, node->mTransformation.d3);
    }

    if (channel->mNumPositionKeys == 1) {
        return assimpToGlmVec3(channel->mPositionKeys[0].mValue);
    }

    size_t pose_idx = findPositionKey(time, channel);
    if (pose_idx == channel->mNumPositionKeys - 1) {
        return assimpToGlmVec3(channel->mPositionKeys[pose_idx].mValue);
    }

    double deltatime = channel->mPositionKeys[pose_idx + 1].mTime - channel->mPositionKeys[pose_idx].mTime;
    double factor = (time - channel->mPositionKeys[pose_idx].mTime) / deltatime;
    const glm::vec3& start = assimpToGlmVec3(channel->mPositionKeys[pose_idx].mValue);
    const glm::vec3& end = assimpToGlmVec3(channel->mPositionKeys[pose_idx + 1].mValue);
    return glm::mix(start, end, factor);
}

glm::vec3 calculateInterpolatedScale(double time, const aiNodeAnim* channel, const aiNode* node) {
    glm::vec3 t;
    glm::quat r;
    glm::vec3 s{};
    decompose(node->mTransformation, t, r, s);

    if (!channel) {
        // aiVector3D scale, pos;
        // aiQuaternion rot;
        // node->mTransformation.Decompose(scale, rot, pos);
        return s;
    }

    if (channel->mNumScalingKeys == 0) {
        return glm::vec3(1.0f);  // default scale
    }

    if (channel->mNumScalingKeys == 1) {
        return assimpToGlmVec3(channel->mScalingKeys[0].mValue);
    }

    size_t pose_idx = findScaleKey(time, channel);
    if (pose_idx == channel->mNumScalingKeys - 1) {
        return assimpToGlmVec3(channel->mScalingKeys[pose_idx].mValue);
    }

    double deltatime = channel->mScalingKeys[pose_idx + 1].mTime - channel->mScalingKeys[pose_idx].mTime;
    double factor = (time - channel->mScalingKeys[pose_idx].mTime) / deltatime;
    const glm::vec3& start = assimpToGlmVec3(channel->mScalingKeys[pose_idx].mValue);
    const glm::vec3& end = assimpToGlmVec3(channel->mScalingKeys[pose_idx + 1].mValue);
    return glm::mix(start, end, factor);
}

glm::quat CalcInterpolatedRotation(double time, const aiNodeAnim* channel, const aiNode* node) {
    glm::vec3 t;
    glm::quat r{};
    glm::vec3 s;
    decompose(node->mTransformation, t, r, s);
    if (!channel) {
        return r;
    }

    if (channel->mNumRotationKeys == 0) {
        return r;
    }

    if (channel->mNumRotationKeys == 1) {
        return assimpToGlmQuat(channel->mRotationKeys[0].mValue);
    }

    unsigned int idx = findRotationKey(time, channel);
    if (idx == channel->mNumRotationKeys - 1) {
        return assimpToGlmQuat(channel->mRotationKeys[idx].mValue);
    }

    double deltaTime = channel->mRotationKeys[idx + 1].mTime - channel->mRotationKeys[idx].mTime;
    double factor = (time - channel->mRotationKeys[idx].mTime) / deltaTime;
    auto start = assimpToGlmQuat(channel->mRotationKeys[idx].mValue);
    auto end = assimpToGlmQuat(channel->mRotationKeys[idx + 1].mValue);
    glm::quat out = glm::slerp(start, end, (float)factor);

    return out;
}

// void Model::buildNodeCache(aiNode* node) {
//     if (node == nullptr) {
//         return;
//     } else {
//         // mNodeCache[node->mName.C_Str()] = node;
//         for (size_t i = 0; i < node->mNumChildren; ++i) {
//             buildNodeCache(node->mChildren[i]);
//         }
//     }
// }
//

const aiNodeAnim* getChannel(const std::map<std::string, aiNodeAnim*>& channelMap, const std::string& name,
                             const aiNode* node)  // node is needed for the rest-pose
{
    auto it = channelMap.find(name);
    if (it != channelMap.end()) return it->second;

    // No channel → bone is static, return nullptr (caller will use node->mTransformation)
    return nullptr;
}

aiMatrix4x4 GetGlobalTransform(aiNode* node) {
    aiMatrix4x4 transform = node->mTransformation;
    aiNode* parent = node->mParent;
    while (parent != nullptr) {
        transform =
            parent->mTransformation * transform;  // Note: Assimp matrices are row-major, multiplication order matters
        parent = parent->mParent;
    }
    return transform;
}

glm::mat4 Animation::getLocalTransformAtTime(const aiNode* node, double time,
                                             const std::map<std::string, aiNodeAnim*>& channelMap) {
    // auto it = channelMap.find(node->mName.C_Str());
    // if (it != channelMap.end()) {
    // find a pose and use it
    // aiNodeAnim* channel = it->second;
    const aiNodeAnim* channel = getChannel(channelMap, node->mName.C_Str(), node);

    glm::vec3 pos = calculateInterpolatedPosition(time, channel, node);
    glm::quat rotation = CalcInterpolatedRotation(time, channel, node);
    glm::vec3 scale = calculateInterpolatedScale(time, channel, node);

    glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 rotation_mat = glm::mat4(rotation);
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

    return translation_mat * rotation_mat * scale_mat;
    // }
    // return AiToGlm(node->mTransformation);
}

void Animation::computeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                                        const std::map<std::string, aiNodeAnim*>& channelMap,
                                        std::map<std::string, glm::mat4>& outGlobalMap) {
    glm::mat4 local = getLocalTransformAtTime(node, time, channelMap);
    glm::mat4 global = parentGlobal * local;
    // if (!std::isfinite(local[0][0])) {
    //     std::cerr << "  local:" << glm::to_string(local) << "\n";
    //     std::cerr << "  global:" << glm::to_string(global) << "\n";
    //     std::cerr << "  time : " << time << "\n";
    //     // std::cerr << "  rot:\n" << rot << "\n";
    //     // std::cerr << "  temp1 (global*offset):\n" << temp1 << "\n";
    //     // std::cerr << "  final (temp1*rot):\n" << final << "\n";
    //     // } else {
    //     //     anim.mFinalTransformations[anim.boneToIdx[boneName]] = final;
    // }
    outGlobalMap[node->mName.C_Str()] = global;

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        computeGlobalTransforms(node->mChildren[i], global, time, channelMap, outGlobalMap);
    }
}

bool Animation::initAnimation(const aiScene* scene) {
    // mNodeCache.clear();
    mFinalTransformations.reserve(100);
    mFinalTransformations.resize(100);
    // buildNodeCache(mScene->mRootNode);
    // printAnimationInfos("", scene);
    if (scene->HasAnimations()) {
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            aiMesh* mesh = scene->mMeshes[m];
            for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                aiBone* bone = mesh->mBones[b];
                std::string boneName = bone->mName.C_Str();

                if (uniqueBones.find(boneName) != uniqueBones.end()) continue;
                uniqueBones.insert(boneName);

                // if (mOffsetMatrixCache.find(boneName) != mOffsetMatrixCache.end()) continue;
                mOffsetMatrixCache[boneName] = AiToGlm(bone->mOffsetMatrix);
            }
        }
        for (const auto& [name, mat] : globalMap) {
            if (!std::isfinite(mat[0][0])) {  // Check any element
                std::cerr << "NaN in globalMap for bone: " << name << std::endl;
            }
        }
        for (const auto& [name, mat] : mOffsetMatrixCache) {
            if (!std::isfinite(mat[0][0])) {
                std::cerr << "NaN in offset for bone: " << name << std::endl;
            }
        }

        std::function<void(const aiNode*)> visit = [&](const aiNode* n) {
            std::string name = n->mName.C_Str();
            if (uniqueBones.count(name) == 0) {
                uniqueBones.insert(name);
                // offset matrix is identity for non-skinned nodes – you can keep it empty
            }
            for (unsigned i = 0; i < n->mNumChildren; ++i) visit(n->mChildren[i]);
        };
        visit(scene->mRootNode);

        /* Create a mapping from [name] -> [channel data]*/
        aiAnimation* anim = scene->mAnimations[0];
        std::cout << "-----+++++++ Model: " << scene->mNumAnimations << " Animations " << std::endl;
        mAnimationDuration = anim->mDuration / anim->mTicksPerSecond;
        for (size_t i = 0; i < anim->mNumChannels; ++i) {
            aiNodeAnim* channel = anim->mChannels[i];
            channelMap[channel->mNodeName.C_Str()] = channel;
        }
        return true;
    }
    return false;
}

void Animation::update(aiNode* root) {
    const aiMatrix4x4 rootTransform = root->mTransformation;

    // 2. Convert to glm and invert
    // Use a helper function for conversion from Assimp's aiMatrix4x4 to glm::mat4
    glm::mat4 globalInverseMatrix = AiToGlm(rootTransform);
    globalInverseMatrix = glm::inverse(globalInverseMatrix);

    computeGlobalTransforms(root, glm::mat4{1.0}, mAnimationSecond, channelMap, globalMap);
}
