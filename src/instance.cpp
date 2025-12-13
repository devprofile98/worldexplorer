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

Instance::Instance(std::vector<glm::vec3> positions, glm::vec3 rotationAxis, std::vector<float> degree,
                   std::vector<glm::vec3> scales, const glm::vec4&& minAABB, const glm::vec4&& maxAABB) {
    for (size_t i = 0; i < positions.size(); i++) {
        auto trans = glm::translate(glm::mat4{1.0f}, positions[i]);
        auto rotate = glm::rotate(glm::mat4{1.0f}, degree[i], rotationAxis);
        auto scale = glm::scale(glm::mat4{1.0f}, scales[i]);
        auto model_matrix = trans * rotate * scale;
        // model_matrix = glm::rotate(model_matrix, glm::radians(180.0f), glm::vec3{1.0, 0.0, 0.0});
        mInstanceBuffer.push_back({model_matrix, model_matrix * minAABB, model_matrix * maxAABB});
    }
}

// uint16_t Instance::addNewInstance(const glm::vec3& positoin, const glm::vec3& scale, const glm::quat& rot) { return
// 0; }

uint16_t Instance::duplicateLastInstance(const glm::vec3& posOffset, const glm::vec3& min, const glm::vec3& max) {
    size_t new_idx = mPositions.size();
    auto new_pos = mPositions.back() + posOffset;
    auto new_scale = mScale.back();

    mPositions.push_back(new_pos);
    mScale.push_back(new_scale);

    glm::mat4 t = glm::translate(glm::mat4{1.0}, new_pos);
    t = glm::rotate(t, 0.f, {1.0, 0.0, 0.0});
    t = glm::scale(t, new_scale);
    std::cout << "ADding here" << '\n';

    mInstanceBuffer.push_back({t, t * glm::vec4{min, 1.0f}, t * glm::vec4{max, 1.0f}});

    // wgpuQueueWriteBuffer(mApp->getRendererResource().queue,
    // mApp->mInstanceManager->getInstancingBuffer().getBuffer(),
    //                      new_idx * sizeof(InstanceData), &instance->mInstanceBuffer[new_idx], sizeof(InstanceData));
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
    // "instance": [
    //   {
    //     "position": [-2.5, 1.144, -2.947],
    //     "scale": [0.05, 0.05, 0.05],
    //     "rotation": [0.0, 0.0, 0.0]
    //   },
}

size_t Instance::getInstanceCount() { return mInstanceBuffer.size(); }

uint16_t Instance::getInstanceID() { return mOffsetID; }
