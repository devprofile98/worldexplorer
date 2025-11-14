
#include "animation.h"

#include <cstring>
#include <functional>
#include <iostream>

#include "assimp/anim.h"
#include "assimp/scene.h"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"

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

size_t findPositionKey(double time, const Bone* bone) {
    const auto& t = bone->channel.translations;
    for (size_t i = 0; i < t.size(); i++) {
        if (time < t[i].time) {
            return i;
        }
    }
    return t.size() - 1;
}

size_t findScaleKey(double time, const Bone* bone) {
    const auto& s = bone->channel.scales;
    for (size_t i = 0; i < s.size(); i++) {
        if (time < s[i].time) {
            return i;
        }
    }
    return s.size() - 1;
}

size_t findRotationKey(double time, const Bone* bone) {
    const auto& r = bone->channel.quats;
    for (size_t i = 0; i < r.size(); ++i) {
        if (time < r[i].time) {
            return i;
        }
    }
    return r.size() - 1;
}

static void decompose(const aiMatrix4x4& m, glm::vec3& t, glm::quat& r, glm::vec3& s) {
    aiVector3D aiS, aiT;
    aiQuaternion aiR;
    m.Decompose(aiS, aiR, aiT);
    t = glm::vec3(aiT.x, aiT.y, aiT.z);
    r = glm::quat(aiR.w, aiR.x, aiR.y, aiR.z);
    s = glm::vec3(aiS.x, aiS.y, aiS.z);
}

glm::vec3 calculateInterpolatedPosition(double time, const Bone* bone, const aiNode* node) {
    glm::vec3 t;
    glm::quat r;
    glm::vec3 s{};
    decompose(node->mTransformation, t, r, s);

    if (!bone) {
        return t;
    }

    if (bone->channel.translations.size() == 0) {
        return t;
    }

    if (bone->channel.translations.size() == 1) {
        return bone->channel.translations[0].value;
    }

    size_t pose_idx = findPositionKey(time, bone);
    if (pose_idx == bone->channel.translations.size() - 1) {
        auto v = bone->channel.translations[pose_idx].value;

        return v;
    }

    double deltatime = bone->channel.translations[pose_idx + 1].time - bone->channel.translations[pose_idx].time;
    double factor = (time - bone->channel.translations[pose_idx].time) / deltatime;
    const glm::vec3& start = bone->channel.translations[pose_idx].value;
    const glm::vec3& end = bone->channel.translations[pose_idx + 1].value;

    return glm::mix(start, end, factor);
}

glm::vec3 calculateInterpolatedScale(double time, const Bone* bone, const aiNode* node) {
    glm::vec3 t;
    glm::quat r;
    glm::vec3 s{};
    decompose(node->mTransformation, t, r, s);

    if (!bone) {
        return s;
    }

    auto& scales = bone->channel.scales;
    if (scales.size() == 0) {
        return glm::vec3(1.0f);  // default scale
    }

    if (scales.size() == 1) {
        return scales[0].value;
    }

    size_t pose_idx = findScaleKey(time, bone);
    if (pose_idx == scales.size() - 1) {
        return scales[pose_idx].value;
    }

    double deltatime = scales[pose_idx + 1].time - scales[pose_idx].time;
    double factor = (time - scales[pose_idx].time) / deltatime;
    const glm::vec3& start = scales[pose_idx].value;
    const glm::vec3& end = scales[pose_idx + 1].value;
    return glm::mix(start, end, factor);
}

glm::quat CalcInterpolatedRotation(double time, const Bone* bone, const aiNode* node) {
    glm::vec3 t;
    glm::quat r{};
    glm::vec3 s;
    decompose(node->mTransformation, t, r, s);

    if (!bone) {
        return r;
    }

    auto& quats = bone->channel.quats;
    if (quats.size() == 0) {
        return r;
    }

    if (quats.size() == 1) {
        return quats[0].value;
    }

    unsigned int idx = findRotationKey(time, bone);
    if (idx == quats.size() - 1) {
        auto v = quats[idx].value;
        // std::cout << "here " << glm::to_string(v) << std::endl;
        return v;
    }

    double deltaTime = quats[idx + 1].time - quats[idx].time;
    double factor = (time - quats[idx].time) / deltaTime;
    auto start = quats[idx].value;
    auto end = quats[idx + 1].value;
    glm::quat out = glm::slerp(start, end, (float)factor);

    return out;
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

glm::mat4 Animation::getLocalTransformAtTime(const aiNode* node, double time) {
    const Bone* bone = nullptr;
    auto* action = getActiveAction();
    if (action->Bonemap.contains(node->mName.C_Str())) {
        bone = action->Bonemap[node->mName.C_Str()];
    }

    glm::vec3 pos = calculateInterpolatedPosition(time, bone, node);

    // if (std::strcmp(node->mName.C_Str(), "Upper_Arm.L") == 0 ||
    //     std::strcmp(node->mName.C_Str(), "Armature_Upper_Arm_L") == 0) {
    //     std::cout << "For pos2 is " << glm::to_string(pos2) << " and channel is null?" << (channel == nullptr)
    //               << std::endl;
    //     std::cout << "For pos is " << glm::to_string(pos) << std::endl;
    //     std::cout << " ---------------------------------------- " << std::endl;
    // }

    glm::quat rotation = CalcInterpolatedRotation(time, bone, node);
    glm::vec3 scale = calculateInterpolatedScale(time, bone, node);

    glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 rotation_mat = glm::mat4(rotation);
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

    return translation_mat * rotation_mat * scale_mat;
}

void Animation::computeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                                        std::map<std::string, glm::mat4>& outGlobalMap) {
    glm::mat4 local = getLocalTransformAtTime(node, time);
    glm::mat4 global = parentGlobal * local;

    outGlobalMap[node->mName.C_Str()] = global;

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        computeGlobalTransforms(node->mChildren[i], global, time, outGlobalMap);
    }
}

bool Animation::initAnimation(const aiScene* scene) {
    activeActionIdx = 0;
    mFinalTransformations.reserve(100);
    mFinalTransformations.resize(100);

    if (scene->HasAnimations()) {
        for (size_t a = 0; a < scene->mNumAnimations; ++a) {
            Action* action = new Action{};
            aiAnimation* anim = scene->mAnimations[a];
            std::cout << "-----+++++++ Model: " << scene->mNumAnimations << " Animations " << std::endl;
            action->mAnimationDuration = anim->mDuration / anim->mTicksPerSecond;
            for (size_t i = 0; i < anim->mNumChannels; ++i) {
                aiNodeAnim* channel = anim->mChannels[i];

                // std::cout << " ------------------------------------------------------------\n";
                // storing bone datas
                Bone* b = new Bone{};
                for (size_t i = 0; i < channel->mNumPositionKeys; ++i) {
                    auto [time, value, interpolation] = channel->mPositionKeys[i];
                    b->channel.translations.push_back({(float)time, assimpToGlmVec3(value)});
                }
                for (size_t i = 0; i < channel->mNumRotationKeys; ++i) {
                    auto [time, value, interpolation] = channel->mRotationKeys[i];
                    b->channel.quats.push_back({(float)time, assimpToGlmQuat(value)});
                }
                for (size_t i = 0; i < channel->mNumScalingKeys; ++i) {
                    auto [time, value, interpolation] = channel->mScalingKeys[i];
                    b->channel.scales.push_back({(float)time, assimpToGlmVec3(value)});
                }
                b->id = action->Bonemap.size();
                action->Bonemap[channel->mNodeName.C_Str()] = b;
                // std::cout << "\tChannel with name " << channel->mNodeName.C_Str()
                //           << " has data : trans: " << b->channel.translations.size()
                //           << " scales:  " << b->channel.scales.size() << " rotations:  " << b->channel.quats.size()
                //           << std::endl;
                // std::cout << " ------------------------------------------------------------\n";
            }
            std::cout << " Model have: " << scene->mNumMeshes << " Meshes \n";
            for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                aiMesh* mesh = scene->mMeshes[m];
                std::cout << " \tMesh have: " << mesh->mBones << " Bones \n";
                for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                    aiBone* bone = mesh->mBones[b];
                    std::string boneName = bone->mName.C_Str();
                    if (action->Bonemap.contains(boneName)) {
                        action->Bonemap[boneName]->offsetMatrix = AiToGlm(bone->mOffsetMatrix);
                    } else {
                        // action->Bonemap[boneName];
                        std::cout << "Bone with name " << boneName << " doesnt have any data\n";
                    }
                }
            }
            // std::function<void(const aiNode*)> visit = [&](const aiNode* n) {
            //     std::string name = n->mName.C_Str();
            //     if (action->Bonemap.count(name) == 0) {
            //         action->Bonemap[];
            //         // offset matrix is identity for non-skinned nodes – you can keep it empty
            //     }
            //     for (unsigned i = 0; i < n->mNumChildren; ++i) visit(n->mChildren[i]);
            // };
            // visit(scene->mRootNode);

            actions[anim->mName.C_Str()] = action;
            activeAction = action;
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
    auto* action = getActiveAction();

    computeGlobalTransforms(root, globalInverseMatrix, action->mAnimationSecond, action->calculatedTransform);
}

Action* Animation::getActiveAction() { return activeAction; }

Action* Animation::getAction(const std::string& actionName) {
    if (actions.count(actionName) != 0) {
        activeAction = actions[actionName];
    }
    return activeAction;
}
