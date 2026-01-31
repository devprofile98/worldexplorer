#include "gpu_buffer.h"

#include <cstdint>

// #include "application.h"
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

WGPUBuffer Buffer::create(RendererResource* resource) {
    mBuffer = wgpuDeviceCreateBuffer(resource->device, &mBufferDescriptor);
    mResources = resource;
    return mBuffer;
}

WGPUBuffer Buffer::getBuffer() { return mBuffer; }

void Buffer::queueWrite(uint64_t startOffset, const void* data, size_t writeSize) {
    wgpuQueueWriteBuffer(mResources->queue, mBuffer, startOffset, data, writeSize);
}
