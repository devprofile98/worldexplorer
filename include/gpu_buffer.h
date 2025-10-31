#ifndef WEBGPUTEST_GPU_BUFFER_H
#define WEBGPUTEST_GPU_BUFFER_H

#include <string>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "rendererResource.h"

class Application;

// enum BufferUsage { VERTEX = 0, UNIFORM, COPY_DEST };

class Buffer {
    public:
        Buffer();

        // Setters
        Buffer& setUsage(WGPUBufferUsage usage);
        Buffer& setMappedAtCraetion(bool mappedAtCreation = false);
        Buffer& setLabel(const std::string& label);
        Buffer& setSize(uint64_t size);
        WGPUBuffer create(Application* app);

        // Getters
        WGPUBuffer getBuffer();

    private:
        WGPUBufferDescriptor mBufferDescriptor;
        WGPUBuffer mBuffer;
        std::string mName;
        RendererResource* mResources;
};

#endif  // !WEBGPUTEST_GPU_BUFFER_H
