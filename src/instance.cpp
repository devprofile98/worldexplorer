#include "instance.h"

#include <strings.h>

#include <atomic>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "rendererResource.h"

InstanceManager::InstanceManager(RendererResource* rc, size_t bufferSize, size_t maxInstancePerModel) {
    (void)maxInstancePerModel;
    mOffsetBuffer.setSize(bufferSize)
        .setLabel("Instancing Shader Storage Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setMappedAtCraetion()
        .create(rc);

    mBufferSize = bufferSize;
}

Buffer& InstanceManager::getInstancingBuffer() { return mOffsetBuffer; }
static std::atomic<size_t> mUsedIds{0};
size_t InstanceManager::getNewId() {
    std::cout << "Instance offset id is at " << mUsedIds.load(std::memory_order_seq_cst) << std::endl;
    return mUsedIds.fetch_add(1, std::memory_order_seq_cst);
}

Instance::Instance(std::vector<glm::vec3> positions, std::vector<glm::vec3> rotation, std::vector<glm::vec3> scales,
                   const glm::vec4&& minAABB, const glm::vec4&& maxAABB) {
    for (size_t i = 0; i < positions.size(); i++) {
        mPositions.push_back(positions[i]);
        mScale.push_back(scales[i]);
        mRotation.push_back(rotation[i]);
        auto trans = glm::translate(glm::mat4{1.0f}, positions[i]);
        auto rotate = glm::toMat4(glm::normalize(glm::quat(glm::radians(rotation[i]))));
        auto scale = glm::scale(glm::mat4{1.0f}, scales[i]);
        auto model_matrix = trans * rotate * scale;
        mInstanceBuffer.emplace_back(model_matrix, model_matrix * minAABB, model_matrix * maxAABB);
    }
}

uint16_t Instance::duplicateLastInstance(const glm::vec3& posOffset, const glm::vec3& min, const glm::vec3& max) {
    size_t new_idx = mPositions.size();
    auto new_pos = new_idx == 0 ? glm::vec3{0.0} : mPositions.back() + posOffset;
    auto new_scale = new_idx == 0 ? glm::vec3{1.0} : mScale.back();
    auto new_rotate = new_idx == 0 ? glm::vec3{0.0} : mRotation.back();

    mPositions.push_back(new_pos);
    mScale.push_back(new_scale);
    mRotation.push_back(new_rotate);

    glm::mat4 t = glm::translate(glm::mat4{1.0}, new_pos);
    t = glm::rotate(t, 0.f, {1.0, 0.0, 0.0});
    t = glm::scale(t, new_scale);
    std::cout << "Adding here\n";

    mInstanceBuffer.push_back({t, t * glm::vec4{min, 1.0f}, t * glm::vec4{max, 1.0f}});

    return new_idx;
}

void Instance::dumpJson() {
    std::cout << "[\n";
    for (size_t i = 0; i < mInstanceBuffer.size(); ++i) {
        std::cout << "{\n";
        std::cout << "\"position\": [" << mPositions[i].x << ", " << mPositions[i].y << ", " << mPositions[i].z
                  << "],\n";
        std::cout << "\"scale\": [" << mScale[i].x << ", " << mScale[i].y << ", " << mScale[i].z << "],\n";
        std::cout << "\"rotation\": [" << 0.0 << ", " << 0.0 << ", " << 0.0 << "]\n";
        std::cout << "},\n";
    }
    std::cout << "]\n";
}

size_t Instance::getInstanceCount() { return mInstanceBuffer.size(); }

uint16_t Instance::getInstanceID() { return mOffsetID; }

SingleInstance Instance::createInstanceWrapper(size_t index) { return SingleInstance(index, this); }

SingleInstance::SingleInstance(size_t idx, Instance* ins) : idx(idx), instance(ins) {}

Transformable& SingleInstance::moveTo(const glm::vec3& to) {
    auto& ins_pos = instance->mPositions[idx];
    ins_pos = to;

    glm::mat4 trans = glm::translate(glm::mat4{1.0}, instance->mPositions[idx]);
    auto r = glm::toMat4(glm::quat(glm::degrees(instance->mRotation[idx])));
    auto s = glm::scale(glm::mat4{1.0}, instance->mScale[idx]);
    auto t = trans * r * s;

    instance->mInstanceBuffer[idx] = {t, t * glm::vec4{instance->parent->min, 1.0f},
                                      t * glm::vec4{instance->parent->max, 1.0f}};

    instance->mManager->getInstancingBuffer().queueWrite(
        ((InstanceManager::MAX_INSTANCE_COUNT * instance->mOffsetID) + idx) * sizeof(InstanceData),
        &instance->mInstanceBuffer[idx], sizeof(InstanceData));

    return *this;
}

Transformable& SingleInstance::rotate(const glm::vec3& to) {
    auto& ins_rot = instance->mRotation[idx];
    ins_rot = to;

    glm::mat4 trans = glm::translate(glm::mat4{1.0}, instance->mPositions[idx]);
    auto r = glm::toMat4(glm::quat(glm::degrees(instance->mRotation[idx])));
    auto s = glm::scale(glm::mat4{1.0}, instance->mScale[idx]);

    auto t = trans * r * s;

    instance->mInstanceBuffer[idx] = {t, t * glm::vec4{instance->parent->min, 1.0f},
                                      t * glm::vec4{instance->parent->max, 1.0f}};

    instance->mManager->getInstancingBuffer().queueWrite(
        ((InstanceManager::MAX_INSTANCE_COUNT * instance->mOffsetID) + idx) * sizeof(InstanceData),
        &instance->mInstanceBuffer[idx], sizeof(InstanceData));

    return *this;
}
