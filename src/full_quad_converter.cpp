
#include "full_quad_converter.h"

#include "application.h"
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

void FullQuadConverter::initialize(Application* app, WGPUTextureView sourceTexture, WGPUSampler sampler) {
    mApp = app;

    auto& resource = mApp->getRendererResource();

    mRenderPass = new NewRenderPass{"Full Quad pass"};
    // mRenderPass->setColorAttachment(
    //     {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    // mRenderPass->setDepthStencilAttachment(
    //     {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined,
    //     false});
    // mRenderPass->init();

    WGPUBindGroupLayout bind_group_layout =
        mBindGroup.addSampler(0, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering)
            .addTexture(1, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH, TextureViewDimension::VIEW_2D)
            .createLayout(resource, "fq converter bg");

    mPipeline = new Pipeline{app, {bind_group_layout}, "fq converter pipeline"};

    mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
        .addAttribute(sizeof(glm::vec3), 1, WGPUVertexFormat_Float32x2)
        .configure(sizeof(glm::vec3) + sizeof(glm::vec2), VertexStepMode::VERTEX);

    mPipeline->setVertexBufferLayout(mVertexBufferLayout.getLayout())
        .setShader(mApp->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / "shaders/fqconverter.wgsl", resource)
        .setVertexState()
        .setPrimitiveState()
        .setColorTargetState(WGPUTextureFormat_BGRA8Unorm)
        // .setDepthStencilState(false, 0xFF, 0xFF, WGPUTextureFormat_Undefined)
        .setDepthStencilState(nullptr)
        .setFragmentState();

    mPipeline->createPipeline(resource);

    mVertexBuffer.setSize(sizeof(QuadVertices))
        .setLabel("A Simple Reactangle")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(&resource);

    mVertexBuffer.queueWrite(0, QuadVertices.data(), sizeof(QuadVertices));

    mBindingData.reserve(2);
    mBindingData.resize(2);

    mBindingData[0] = {};
    mBindingData[0].binding = 0;
    mBindingData[0].sampler = mApp->mDefaultSampler;

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = sourceTexture;

    mBindGroup.create(resource, mBindingData);
}

Pipeline* FullQuadConverter::getPipeline() { return mPipeline; }

void FullQuadConverter::executePass(Texture* target) {
    mRenderPass->setColorAttachment(
        {target->getTextureView(), nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mRenderPass->setDepthStencilAttachment(
        // {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined,
        // false});
        {nullptr, StoreOp::Discard, LoadOp::Undefined, true, StoreOp::Undefined, LoadOp::Undefined, true});
    mRenderPass->mRenderPassDesc.depthStencilAttachment = nullptr;
    mRenderPass->init()->depthStencilAttachment = nullptr;

    WGPURenderPassEncoder encoder =
        wgpuCommandEncoderBeginRenderPass(mApp->getRendererResource().commandEncoder, &mRenderPass->mRenderPassDesc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindGroup.getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetPipeline(encoder, mPipeline->getPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0, sizeof(QuadVertices));
    wgpuRenderPassEncoderDraw(encoder, 6, 1, 0, 0);

    wgpuRenderPassEncoderEnd(encoder);
    wgpuRenderPassEncoderRelease(encoder);
}
