
#include "hdr_pass.h"

#include "application.h"
#include "imgui.h"
#include "webgpu/webgpu.h"

// simple quad for full screen effect processing
static const std::array<float, 30> QuadVertices = {
    // Position (x, y, z)          // UV (u, v)
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
    1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // bottom-right
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // top-right

    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // top-right
    -1.0f, 1.0f,  0.0f, 0.0f, 1.0f   // top-left
};

HDRPipeline::HDRPipeline(Application* app, const std::string& name) : mApp(app), mName(name) {
    auto& resource = mApp->getRendererResource();

    mRenderPass = new NewRenderPass{"HDR pass"};
    mRenderPass->setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mRenderPass->setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    mRenderPass->init();

    WGPUBindGroupLayout bind_group_layout =
        mBindGroup.addSampler(0, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering)
            .addTexture(1, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::VIEW_2D)
            .createLayout(resource, "hdr system bg");

    WGPUBindGroupLayout hdr_settings_bg =
        mSettingsBG
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(HDRSettings))
            .createLayout(resource, "hdr system settings bg");

    mPipeline = new Pipeline{app, {bind_group_layout, hdr_settings_bg}, "hdr system pipeline"};

    mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
        .addAttribute(sizeof(glm::vec3), 1, WGPUVertexFormat_Float32x2)
        .configure(sizeof(glm::vec3) + sizeof(glm::vec2), VertexStepMode::VERTEX);

    mPipeline->setVertexBufferLayout(mVertexBufferLayout.getLayout())
        .setShader(mApp->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / "shaders/hdr.wgsl", resource)
        .setVertexState()
        // .setBlendState(blend_state)
        .setPrimitiveState()
        .setColorTargetState(WGPUTextureFormat_BGRA8UnormSrgb)
        // .setColorTargetState(WGPUTextureFormat_RGBA16Float)
        .setDepthStencilState(false, 0xFF, 0xFF, WGPUTextureFormat_Depth24PlusStencil8)
        .setFragmentState()
        .createPipeline(resource);

    mVertexBuffer.setSize(sizeof(QuadVertices))
        .setLabel("A Simple Reactangle")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(&resource);

    mSettingsBuffer.setSize(sizeof(HDRSettings))
        .setLabel("HDR Pass setting uniform buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(&resource);

    mSettingsBuffer.queueWrite(0, &mHDRSettings, sizeof(HDRSettings));
    mVertexBuffer.queueWrite(0, QuadVertices.data(), sizeof(QuadVertices));

    mBindingData.reserve(2);
    mBindingData.resize(2);

    mBindingData[0] = {};
    mBindingData[0].binding = 0;
    mBindingData[0].sampler = mApp->mDefaultSampler;

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = mApp->mHDRTexture->getTextureView();

    mBindGroup.create(resource, mBindingData);

    mSettingsBGData.reserve(1);
    mSettingsBGData.resize(1);
    mSettingsBGData[0] = {};
    mSettingsBGData[0].nextInChain = nullptr;
    mSettingsBGData[0].binding = 0;
    mSettingsBGData[0].buffer = mSettingsBuffer.getBuffer();
    mSettingsBGData[0].size = mSettingsBuffer.getBufferSize();

    mSettingsBG.create(resource, mSettingsBGData);
}

void HDRPipeline::onResize() {
    auto& resource = mApp->getRendererResource();
    if (mBindGroup.getBindGroup() != nullptr) {
        wgpuBindGroupRelease(mBindGroup.getBindGroup());
        mBindGroup.getBindGroup() = nullptr;
    }

    mBindingData.clear();

    mBindingData.reserve(2);
    mBindingData.resize(2);

    mBindingData[0] = {};
    mBindingData[0].binding = 0;
    mBindingData[0].sampler = mApp->mDefaultSampler;

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = mApp->mHDRTexture->getTextureView();

    mBindGroup.create(resource, mBindingData);
}

void HDRPipeline::executePass() {
    mRenderPass->setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mRenderPass->setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    mRenderPass->init();

    WGPURenderPassEncoder encoder =
        wgpuCommandEncoderBeginRenderPass(mApp->getRendererResource().commandEncoder, &mRenderPass->mRenderPassDesc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindGroup.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(encoder, 1, mSettingsBG.getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetPipeline(encoder, mPipeline->getPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0, sizeof(QuadVertices));
    wgpuRenderPassEncoderDraw(encoder, 6, 1, 0, 0);

    wgpuRenderPassEncoderEnd(encoder);
    wgpuRenderPassEncoderRelease(encoder);
}

const std::string_view HDRPipeline::getName() const { return mName; }

void HDRPipeline::userInterface() {
    if (ImGui::DragFloat("Exposure value", &mHDRSettings.exposure, 0.01, 0, 5)) {
        mSettingsBuffer.queueWrite(0, &mHDRSettings, sizeof(HDRSettings));
    }

    bool is_active = mHDRSettings.isActive != 0;
    if (ImGui::Checkbox("Tone mapping atcive", &is_active)) {
        mHDRSettings.isActive = is_active ? 1 : 0;
        mSettingsBuffer.queueWrite(0, &mHDRSettings, sizeof(HDRSettings));
    }
}
