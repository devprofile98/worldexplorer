#ifndef WEBGPUTEST_GPU_BUFFER_H
#define WEBGPUTEST_GPU_BUFFER_H

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

// enum BufferUsage { VERTEX = 0, UNIFORM, COPY_DEST };

class Buffer {
    public:
        Buffer();

        // Setters
        Buffer& setUsage(WGPUBufferUsageFlags usage);
        Buffer& setMappedAtCraetion(bool mappedAtCreation = false);
        Buffer& setLabel(const char* label);
        Buffer& setSize(uint64_t size);
        WGPUBuffer create(Application* app);

        // Getters
        WGPUBuffer getBuffer();

    private:
        WGPUBufferDescriptor mBufferDescriptor;
        WGPUBuffer mBuffer;
};

#endif  // !WEBGPUTEST_GPU_BUFFER_H