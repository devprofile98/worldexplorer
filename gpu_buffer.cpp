#include "gpu_buffer.h"

#include "application.h"

Buffer::Buffer() : mBufferDescriptor({}) {}

// Setters
Buffer& Buffer::setUsage(WGPUBufferUsageFlags usage) {
    mBufferDescriptor.usage = usage;
    return *this;
}

Buffer& Buffer::setMappedAtCraetion(bool mappedAtCreation) {
    mBufferDescriptor.mappedAtCreation = mappedAtCreation;
    return *this;
}

Buffer& Buffer::setLabel(const char* label) {
    mBufferDescriptor.label = label;

    return *this;
}

Buffer& Buffer::setSize(uint64_t size) {
    mBufferDescriptor.size = size;
    return *this;
}

WGPUBuffer Buffer::create(Application* app) {
    mBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &mBufferDescriptor);
    return mBuffer;
}

WGPUBuffer Buffer::getBuffer() { return mBuffer; }