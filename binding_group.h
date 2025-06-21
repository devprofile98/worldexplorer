#ifndef WEBGPUTEST_BINDING_GROUP_H
#define WEBGPUTEST_BINDING_GROUP_H

#include <vector>

#include "webgpu/webgpu.h"

class Application;

enum class BindGroupEntryVisibility { FRAGMENT = 0, VERTEX, VERTEX_FRAGMENT, COMPUTE };
enum class TextureSampleType { FLAOT = 0, DEPTH };
enum class TextureViewDimension { VIEW_2D = 0, CUBE, ARRAY_2D };
enum class BufferBindingType { UNIFORM = 0, STORAGE, STORAGE_READONLY };
enum class SampleType { Filtering = 0, Compare };

class BindingGroup {
    public:
        BindingGroup(/* args */);
        ~BindingGroup();

        void add(WGPUBindGroupLayoutEntry entry);
        void addTexture(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, TextureSampleType sampleType,
                        TextureViewDimension viewDim);
        void addBuffer(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, BufferBindingType type,
                       uint64_t minBindingSize);
        void addSampler(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, SampleType type);

        // --- Getter functions
        size_t getEntryCount() const;
        WGPUBindGroupLayoutEntry* getEntryData();
        WGPUBindGroup& getBindGroup();
        WGPUBindGroupDescriptor& getDescriptor();

        std::vector<WGPUBindGroupLayoutEntry> mEntries = {};
        WGPUBindGroupLayout createLayout(Application* app, const char* label);
        void create(Application* app, std::vector<WGPUBindGroupEntry>& bindingData);
        WGPUBindGroup createNew(Application* app, std::vector<WGPUBindGroupEntry>& bindingData);

    private:
        WGPUBindGroup mBindGroup;
        WGPUBindGroupLayout mBindGroupLayout;
        WGPUBindGroupLayoutDescriptor mBindGroupLayoutDesc;
        WGPUBindGroupDescriptor mBindGroupDesc;
};

#endif  // WEBGPUTEST_BINDING_GROUP_H
