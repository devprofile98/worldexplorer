#include "instance.h"

#include "application.h"
#include "glm/ext/quaternion_transform.hpp"
#include "glm/trigonometric.hpp"

InstanceManager::InstanceManager(Application* app, size_t bufferSize, size_t maxInstancePerModel) {
    (void)maxInstancePerModel;
    mOffsetBuffer.setSize(bufferSize)
        .setLabel("Instancing Shader Storage Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setMappedAtCraetion()
        .create(app);

    mBufferSize = bufferSize;
}

Buffer& InstanceManager::getInstancingBuffer() { return mOffsetBuffer; }

// Instance::Instance(std::vector<glm::mat4>& instanceBuffer) { mInstanceBuffer = instanceBuffer; }
Instance::Instance(std::vector<glm::vec3> positions, glm::vec3 rotationAxis, std::vector<float> degree,
                   std::vector<glm::vec3> scales, const glm::vec4&& minAABB, const glm::vec4&& maxAABB) {
    for (size_t i = 0; i < positions.size(); i++) {
        auto trans = glm::translate(glm::mat4{1.0f}, positions[i]);
        auto rotate = glm::rotate(glm::mat4{1.0f}, degree[i], rotationAxis);
        auto scale = glm::scale(glm::mat4{1.0f}, scales[i]);
        auto model_matrix = trans * rotate * scale;
        model_matrix = glm::rotate(model_matrix, glm::radians(180.0f), glm::vec3{1.0, 0.0, 0.0});
        mInstanceBuffer.push_back({model_matrix, model_matrix * minAABB, model_matrix * maxAABB});
    }
}

size_t Instance::getInstanceCount() { return mInstanceBuffer.size(); }

uint16_t Instance::getInstanceID() { return mOffsetID; }
