#include "binding_group.h"

#include <webgpu/webgpu.h>

#include "application.h"
#include "wgpu_utils.h"

static void setDefaultValue(WGPUBindGroupLayoutEntry& bindingLayout) {
    bindingLayout = {};
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = WGPUBufferBindingType_BindingNotUsed;
    bindingLayout.buffer.hasDynamicOffset = false;
    bindingLayout.buffer.minBindingSize = 100;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_BindingNotUsed;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_BindingNotUsed;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_BindingNotUsed;
    bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

WGPUTextureSampleType sampleTypeFrom(TextureSampleType type) {
    WGPUTextureSampleType ret = WGPUTextureSampleType_BindingNotUsed;
    switch (type) {
        case TextureSampleType::FLAOT:
            /* code */
            ret = WGPUTextureSampleType_Float;
            break;

        case TextureSampleType::DEPTH:
            /* code */
            ret = WGPUTextureSampleType_Depth;
            break;
        default:
            break;
    }

    return ret;
}

WGPUTextureViewDimension viewDimensionFrom(TextureViewDimension dim) {
    WGPUTextureViewDimension ret = WGPUTextureViewDimension_Undefined;
    switch (dim) {
        case TextureViewDimension::VIEW_2D:
            ret = WGPUTextureViewDimension_2D;
            break;
        case TextureViewDimension::CUBE:
            ret = WGPUTextureViewDimension_Cube;
            break;
        case TextureViewDimension::ARRAY_2D:
            ret = WGPUTextureViewDimension_2DArray;
            break;
        default:
            break;
    }

    return ret;
}

WGPUBufferBindingType bufferTypeFrom(BufferBindingType type) {
    WGPUBufferBindingType ret = WGPUBufferBindingType_Undefined;
    switch (type) {
        case BufferBindingType::UNIFORM:
            ret = WGPUBufferBindingType_Uniform;
            break;
        case BufferBindingType::STORAGE:
            ret = WGPUBufferBindingType_Storage;
            break;
        case BufferBindingType::STORAGE_READONLY:
            ret = WGPUBufferBindingType_ReadOnlyStorage;
            break;
        default:
            ret = WGPUBufferBindingType_Undefined;
            break;
    }

    return ret;
}

WGPUSamplerBindingType sampleTypeFrom(SampleType type) {
    WGPUSamplerBindingType ret = WGPUSamplerBindingType_Undefined;
    switch (type) {
        case SampleType::Filtering:
            ret = WGPUSamplerBindingType_Filtering;
            break;
        case SampleType::Compare:
            ret = WGPUSamplerBindingType_Comparison;
            break;
        default:
            break;
    }

    return ret;
}

WGPUShaderStage visibilityFrom(BindGroupEntryVisibility visibleTo) {
    WGPUShaderStage visible_to = WGPUShaderStage_None;
    if (visibleTo == BindGroupEntryVisibility::FRAGMENT) {
        visible_to = WGPUShaderStage_Fragment;
    } else if (visibleTo == BindGroupEntryVisibility::VERTEX) {
        visible_to = WGPUShaderStage_Vertex;
    } else if (visibleTo == BindGroupEntryVisibility::VERTEX_FRAGMENT) {
        visible_to = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    } else if (visibleTo == BindGroupEntryVisibility::COMPUTE) {
        visible_to = WGPUShaderStage_Compute;
    }

    return visible_to;
}

BindingGroup::BindingGroup(/* args */) { mEntries.reserve(20); }

BindingGroup::~BindingGroup() {}

void BindingGroup::add(WGPUBindGroupLayoutEntry entry) { mEntries.push_back(entry); }

void BindingGroup::addTexture(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, TextureSampleType sampleType,
                              TextureViewDimension viewDim) {
    WGPUBindGroupLayoutEntry entry_layout = {};
    setDefaultValue(entry_layout);
    entry_layout.binding = bindingNumber;
    entry_layout.visibility = visibilityFrom(visibleTo);
    entry_layout.texture.sampleType = sampleTypeFrom(sampleType);
    entry_layout.texture.viewDimension = viewDimensionFrom(viewDim);
    std::cout << "binding at " << bindingNumber << " " << entry_layout.buffer.type << std::endl;
    this->add(entry_layout);
}

void BindingGroup::addBuffer(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, BufferBindingType type,
                             uint64_t minBindingSize) {
    WGPUBindGroupLayoutEntry entry_layout = {};
    setDefaultValue(entry_layout);
    entry_layout.binding = bindingNumber;
    entry_layout.visibility = visibilityFrom(visibleTo);
    entry_layout.buffer.type = bufferTypeFrom(type);
    entry_layout.buffer.minBindingSize = minBindingSize;
    this->add(entry_layout);
}

void BindingGroup::addSampler(uint32_t bindingNumber, BindGroupEntryVisibility visibleTo, SampleType type) {
    WGPUBindGroupLayoutEntry entry_layout = {};
    setDefaultValue(entry_layout);
    entry_layout.binding = bindingNumber;
    entry_layout.visibility = visibilityFrom(visibleTo);
    entry_layout.sampler.type = sampleTypeFrom(type);
    this->add(entry_layout);
}

WGPUBindGroupLayout BindingGroup::createLayout(Application* app, const char* label) {
    mBindGroupLayoutDesc = {};
    mBindGroupLayoutDesc.nextInChain = nullptr;
    mBindGroupLayoutDesc.label = {"test test", WGPU_STRLEN};
    mBindGroupLayoutDesc.entryCount = this->getEntryCount();
    mBindGroupLayoutDesc.entries = this->getEntryData();

    mBindGroupLayout = wgpuDeviceCreateBindGroupLayout(app->getRendererResource().device, &mBindGroupLayoutDesc);

    return mBindGroupLayout;
}

size_t BindingGroup::getEntryCount() const { return mEntries.size(); };

WGPUBindGroupLayoutEntry* BindingGroup::getEntryData() { return mEntries.data(); };

WGPUBindGroup& BindingGroup::getBindGroup() { return mBindGroup; }

void BindingGroup::create(Application* app, std::vector<WGPUBindGroupEntry>& bindingData) {
    std::cout << "Passed here " << mBindGroupLayoutDesc.entryCount << " " << bindingData.size() << "\n ";
    mBindGroupDesc = {};
    mBindGroupDesc.nextInChain = nullptr;
    mBindGroupDesc.layout = mBindGroupLayout;
    mBindGroupDesc.entryCount = mBindGroupLayoutDesc.entryCount;
    mBindGroupDesc.entries = bindingData.data();
    mBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mBindGroupDesc);
};

WGPUBindGroup BindingGroup::createNew(Application* app, std::vector<WGPUBindGroupEntry>& bindingData) {
    mBindGroupDesc = {};
    mBindGroupDesc.nextInChain = nullptr;
    mBindGroupDesc.layout = mBindGroupLayout;
    mBindGroupDesc.entryCount = mBindGroupLayoutDesc.entryCount;
    mBindGroupDesc.entries = bindingData.data();

    return wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mBindGroupDesc);
}

WGPUBindGroupDescriptor& BindingGroup::getDescriptor() { return mBindGroupDesc; }
