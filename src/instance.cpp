#include "instance.h"

#include "application.h"

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
    (void)positions;
    (void)rotationAxis;
    (void)degree;
    (void)scales;

    for (size_t i = 0; i < positions.size(); i++) {
        auto trans = glm::translate(glm::mat4{1.0f}, positions[i]);
        auto rotate = glm::rotate(glm::mat4{1.0f}, degree[i], glm::vec3{0.0, 0.0, 1.0});
        auto scale = glm::scale(glm::mat4{1.0f}, scales[i]);
        auto model_matrix = trans * rotate * scale;
        mInstanceBuffer.push_back({model_matrix, model_matrix * minAABB, model_matrix * maxAABB});
    }
}

size_t Instance::getInstanceCount() { return mInstanceBuffer.size(); }

uint16_t Instance::getInstanceID() { return mOffsetID; }
