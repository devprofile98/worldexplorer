#ifndef WEBGPUTEST_BINDING_GROUP_H
#define WEBGPUTEST_BINDING_GROUP_H

#include <vector>

#include "../webgpu/webgpu.h"
#include "rendererResource.h"

class Application;

enum class BindGroupEntryVisibility { UNDEFINED = 0, VERTEX, FRAGMENT, VERTEX_FRAGMENT, COMPUTE };
enum class TextureSampleType { FLAOT = 0, DEPTH };
enum class TextureViewDimension { UNDEFINED = 0, VIEW_2D = 0x2, ARRAY_2D = 0x3, CUBE = 0x4 };
enum class BufferBindingType { UNIFORM = 0, STORAGE, STORAGE_READONLY };
enum class SampleType { Filtering = 0, Compare };

class BindingGroup {
    public:
        BindingGroup(/* args */);
        ~BindingGroup();

        void add(WGPUBindGroupLayoutEntry entry);
        BindingGroup& addTexture(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo,
                                 TextureSampleType sampleType, TextureViewDimension viewDim);
        BindingGroup& addBuffer(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, BufferBindingType type,
                                uint64_t minBindingSize);
        BindingGroup& addSampler(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, SampleType type);

        // --- Getter functions
        size_t getEntryCount() const;
        WGPUBindGroupLayoutEntry* getEntryData();
        WGPUBindGroup& getBindGroup();
        WGPUBindGroupDescriptor& getDescriptor();

        std::vector<WGPUBindGroupLayoutEntry> mEntries{};
        WGPUBindGroupLayout createLayout(const RendererResource& resource, const char* label);
        void create(const RendererResource& resource, std::vector<WGPUBindGroupEntry>& bindingData);
        WGPUBindGroup createNew(const RendererResource& resource, std::vector<WGPUBindGroupEntry>& bindingData);

    private:
        WGPUBindGroup mBindGroup;
        WGPUBindGroupLayout mBindGroupLayout;
        WGPUBindGroupLayoutDescriptor mBindGroupLayoutDesc;
        WGPUBindGroupDescriptor mBindGroupDesc;
};

#endif  // WEBGPUTEST_BINDING_GROUP_H
