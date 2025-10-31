#include "gpu_buffer.h"

#include "application.h"
#include "rendererResource.h"

Buffer::Buffer() : mBufferDescriptor({}) {}

// Setters
Buffer& Buffer::setUsage(WGPUBufferUsage usage) {
    mBufferDescriptor.usage = usage;
    return *this;
}

Buffer& Buffer::setMappedAtCraetion(bool mappedAtCreation) {
    mBufferDescriptor.mappedAtCreation = mappedAtCreation;
    return *this;
}

Buffer& Buffer::setLabel(const std::string& label) {
    mName = label;
    mBufferDescriptor.label = {label.c_str(), label.size()};
    return *this;
}

Buffer& Buffer::setSize(uint64_t size) {
    mBufferDescriptor.size = size;
    return *this;
}

WGPUBuffer Buffer::create(Application* app) {
    mBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &mBufferDescriptor);
    mResources = &app->getRendererResource();
    return mBuffer;
}

WGPUBuffer Buffer::getBuffer() { return mBuffer; }
