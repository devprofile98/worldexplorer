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

Instance::Instance(std::vector<glm::mat4>& instanceBuffer) { mInstanceBuffer = instanceBuffer; }

size_t Instance::getInstanceCount() { return mInstanceBuffer.size(); }

uint16_t Instance::getInstanceID() { return mOffsetID; }
