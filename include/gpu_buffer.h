#ifndef WEBGPUTEST_GPU_BUFFER_H
#define WEBGPUTEST_GPU_BUFFER_H

#include <string>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "rendererResource.h"

class Buffer {
    public:
        Buffer();

        // Setters
        Buffer& setUsage(WGPUBufferUsage usage);
        Buffer& setMappedAtCraetion(bool mappedAtCreation = false);
        Buffer& setLabel(const std::string& label);
        Buffer& setSize(uint64_t size);
        WGPUBuffer create(RendererResource* resource);
        void queueWrite(uint64_t startOffset, const void* data, size_t writeSize);

        // Getters
        WGPUBuffer getBuffer();

    private:
        WGPUBufferDescriptor mBufferDescriptor;
        WGPUBuffer mBuffer;
        std::string mName;
        RendererResource* mResources;
};

#endif  // !WEBGPUTEST_GPU_BUFFER_H
