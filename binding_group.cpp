#include "binding_group.h"

#include "application.h"

static void setDefaultValue(WGPUBindGroupLayoutEntry& bindingLayout) {
    bindingLayout = {};
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
    bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

WGPUTextureSampleType sampleTypeFrom(TextureSampleType type) {
    WGPUTextureSampleType ret = WGPUTextureSampleType_Undefined;
    if (type == TextureSampleType::FLAOT) {
        ret = WGPUTextureSampleType_Float;
    }

    return ret;
}

WGPUTextureViewDimension viewDimensionFrom(TextureViewDimension dim) {
    WGPUTextureViewDimension ret = WGPUTextureViewDimension_Undefined;
    if (dim == TextureViewDimension::VIEW_2D) {
        ret = WGPUTextureViewDimension_2D;
    }

    return ret;
}

WGPUBufferBindingType bufferTypeFrom(BufferBindingType type) {
    WGPUBufferBindingType ret = WGPUBufferBindingType_Undefined;
    if (type == BufferBindingType::UNIFORM) {
        ret = WGPUBufferBindingType_Uniform;
    }

    return ret;
}

WGPUSamplerBindingType sampleTypeFrom(SampleType type) {
    WGPUSamplerBindingType ret = WGPUSamplerBindingType_Undefined;
    if (type == SampleType::Filtering) {
        ret = WGPUSamplerBindingType_Filtering;
    }

    return ret;
}

WGPUShaderStageFlags visibilityFrom(BindGroupEntryVisibility visibleTo) {
    WGPUShaderStageFlags visible_to = WGPUShaderStage_None;
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

BindingGroup::BindingGroup(/* args */) { mEntries.reserve(3); }

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
    mBindGroupLayoutDesc.label = label;
    mBindGroupLayoutDesc.entryCount = this->getEntryCount();
    mBindGroupLayoutDesc.entries = this->getEntryData();

    mBindGroupLayout = wgpuDeviceCreateBindGroupLayout(app->getRendererResource().device, &mBindGroupLayoutDesc);

    return mBindGroupLayout;
}

size_t BindingGroup::getEntryCount() const { return mEntries.size(); };

WGPUBindGroupLayoutEntry* BindingGroup::getEntryData() { return mEntries.data(); };

WGPUBindGroup& BindingGroup::getBindGroup() { return mBindGroup; }

void BindingGroup::create(Application* app, std::vector<WGPUBindGroupEntry>& bindingData) {
    mBindGroupDesc.nextInChain = nullptr;
    mBindGroupDesc.layout = mBindGroupLayout;
    mBindGroupDesc.entryCount = mBindGroupLayoutDesc.entryCount;
    mBindGroupDesc.entries = bindingData.data();
    // std::cout << "Waht the fuck is happening!\n";

    mBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mBindGroupDesc);
};

WGPUBindGroupDescriptor& BindingGroup::getDescriptor() { return mBindGroupDesc; }