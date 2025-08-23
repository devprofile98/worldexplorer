
#include <animation.h>

#include "assimp/anim.h"
#include "assimp/scene.h"

// void getFinalTransformationList

Bone Animation::parseAnimationData(aiScene scene) {
    if (!scene.HasAnimations()) {
        return Bone{};
    }

    Bone root_bone{};

    const aiAnimation* anim = scene.mAnimations[0];
    for (size_t i = 0; i < anim->mNumChannels; ++i) {
        const aiNodeAnim* channel = anim->mChannels[i];
        // positions
        for (size_t j = 0; j < channel->mNumPositionKeys; ++j) {
        }
        // rotations
        for (size_t j = 0; j < channel->mNumRotationKeys; ++j) {
        }
        // scales
        for (size_t j = 0; j < channel->mNumScalingKeys; ++j) {
        }
    }
    return root_bone;
}
