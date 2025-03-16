#include "shadow_pass.h"

#include "application.h"
#include "glm/fwd.hpp"
#include "gpu_buffer.h"
#include "model.h"
#include "webgpu.h"

ShadowPass::ShadowPass(Application* app) { mApp = app; }

void ShadowPass::createRenderPass() {
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor shadow_depth_texture_descriptor = {};
    shadow_depth_texture_descriptor.label = "Shadow Mapping Target Texture :)";
    shadow_depth_texture_descriptor.nextInChain = nullptr;
    shadow_depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    shadow_depth_texture_descriptor.format = depth_texture_format;
    shadow_depth_texture_descriptor.mipLevelCount = 1;
    shadow_depth_texture_descriptor.sampleCount = 1;
    shadow_depth_texture_descriptor.size = {static_cast<uint32_t>(2048), static_cast<uint32_t>(2048), 1};
    shadow_depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    shadow_depth_texture_descriptor.viewFormatCount = 1;
    shadow_depth_texture_descriptor.viewFormats = &depth_texture_format;
    mShadowDepthTexture = wgpuDeviceCreateTexture(mApp->getRendererResource().device, &shadow_depth_texture_descriptor);

    WGPUTextureViewDescriptor sdtv_desc = {};  // shadow depth texture view
    sdtv_desc.label = "SDTV desc";
    sdtv_desc.aspect = WGPUTextureAspect_DepthOnly;
    sdtv_desc.baseArrayLayer = 0;
    sdtv_desc.arrayLayerCount = 1;
    sdtv_desc.baseMipLevel = 0;
    sdtv_desc.mipLevelCount = 1;
    sdtv_desc.dimension = WGPUTextureViewDimension_2D;
    sdtv_desc.format = depth_texture_format;
    mShadowDepthTextureView = wgpuTextureCreateView(mShadowDepthTexture, &sdtv_desc);

    render_target = new Texture{mApp->getRendererResource().device, 2048, 2048, TextureDimension::TEX_2D,
                                WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment};
    render_target->createView();

    mRenderPassColorAttachment.view = render_target->getTextureView();
    mRenderPassColorAttachment.resolveTarget = nullptr;
    mRenderPassColorAttachment.loadOp = WGPULoadOp_Load;
    mRenderPassColorAttachment.storeOp = WGPUStoreOp_Discard;
    mRenderPassColorAttachment.clearValue = WGPUColor{0.02, 0.80, 0.92, 1.0};

    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = &mRenderPassColorAttachment;

    static WGPURenderPassDepthStencilAttachment shadow_pass_depth_stencil_attachment;
    shadow_pass_depth_stencil_attachment.view = mShadowDepthTextureView;
    shadow_pass_depth_stencil_attachment.depthClearValue = 1.0f;
    shadow_pass_depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
    shadow_pass_depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
    shadow_pass_depth_stencil_attachment.depthReadOnly = false;

    shadow_pass_depth_stencil_attachment.stencilClearValue = 0.0f;
    shadow_pass_depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
    shadow_pass_depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
    shadow_pass_depth_stencil_attachment.stencilReadOnly = true;

    mRenderPassDesc.depthStencilAttachment = &shadow_pass_depth_stencil_attachment;
    mRenderPassDesc.timestampWrites = nullptr;

    // creating pipeline
    // for projection
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(Scene));
    mBindingGroup.addBuffer(1,  //
                            BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                            mApp->mInstanceManager->mBufferSize);

    mBindingGroup.addBuffer(2, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                            sizeof(ObjectInfo));

    mBindingGroup.addTexture(3,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);

    mBindingGroup.addSampler(4,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");

    mRenderPipeline = new Pipeline{mApp, {bind_group_layout}};
    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout
                                   .addAttribute(0, 0, WGPUVertexFormat_Float32x3)  // for position
                                   .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
                                   .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
                                   .addAttribute(offsetof(VertexAttributes, uv), 3, WGPUVertexFormat_Float32x2)
                                   .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);

    static WGPUFragmentState mFragmentState = {};
    mFragmentState.nextInChain = nullptr;
    mFragmentState.module = nullptr;
    mFragmentState.entryPoint = nullptr;
    mFragmentState.constants = nullptr;
    mFragmentState.constantCount = 0;
    mFragmentState.targetCount = 0;
    mFragmentState.targets = nullptr;
    (void)mFragmentState;

    // fill bindgroup
    mBindingData.reserve(5);

    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = nullptr;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(Scene);

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = mApp->mInstanceManager->mBufferSize;

    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].buffer = nullptr;
    mBindingData[2].offset = 0;
    mBindingData[2].size = sizeof(ObjectInfo);

    mBindingData[3] = {};
    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].textureView = nullptr;

    mBindingData[4] = {};
    mBindingData[4].binding = 4;
    mBindingData[4].sampler = mApp->getDefaultSampler();

    mRenderPipeline->setShader(RESOURCE_DIR "/shadow.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState()
        .setPrimitiveState()
        .setDepthStencilState(true, 0xFF, 0xFF)
        .setBlendState()
        .setColorTargetState(WGPUTextureFormat_RGBA8Unorm)
        .setFragmentState();

    /*mRenderPipeline->getDescriptorPtr()->fragment = nullptr;*/

    mRenderPipeline->setMultiSampleState().createPipeline(mApp);

    // creating and writing to buffer
    mSceneUniformBuffer.setLabel("shadow map pass scene buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Scene))
        .setMappedAtCraetion()
        .create(mApp);
}

glm::mat4 createProjectionFromFrustumCorner(const std::vector<glm::vec4>& corners, const glm::mat4& lightView) {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners) {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }
    constexpr float zMult = 2.0f;
    if (minZ < 0) {
        minZ *= zMult;
    } else {
        minZ /= zMult;
    }
    if (maxZ < 0) {
        maxZ /= zMult;
    } else {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    return lightProjection;
}

void ShadowPass::setupScene(const std::vector<glm::vec4>& corners) {
    /*float near_plane = 0.1f, far_plane = 10.5f;*/
    /*glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);*/
    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners) {
        center += glm::vec3(v);
    }
    center /= corners.size();
    /*sphere4.moveTo(center);*/

    this->center = center;

    auto view = glm::lookAt(this->center + this->lightPos, this->center, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 projection = createProjectionFromFrustumCorner(corners, view);

    mScene.projection = projection;
    mScene.view = view;
}

void ShadowPass::setupScene(const glm::vec3 lightPos) {
    (void)lightPos;
    float near_plane = 0.1f, far_plane = 10.5f;
    glm::mat4 projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);

    auto view = glm::lookAt(center + this->lightPos, center, glm::vec3{0.0f, 0.0f, 1.0f});
    mScene.projection = projection;
    mScene.view = view;
}

Scene& ShadowPass::getScene() { return mScene; }

Pipeline* ShadowPass::getPipeline() { return mRenderPipeline; }

WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor() { return &mRenderPassDesc; }

void ShadowPass::render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder) {
    /*auto& render_resource = mApp->getRendererResource();*/
    for (auto* model : models) {
        for (auto& [mat_id, mesh] : model->mMeshes) {
            Buffer modelUniformBuffer = {};
            modelUniformBuffer.setLabel("Model Uniform Buffer")
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .setSize(sizeof(mScene))
                .setMappedAtCraetion()
                .create(mApp);

            mScene.model = model->getTranformMatrix();
            mBindingData[0].buffer = modelUniformBuffer.getBuffer();
            mBindingData[1].buffer = mApp->mInstanceManager->getInstancingBuffer().getBuffer();
            mBindingData[2].buffer = model->getUniformBuffer().getBuffer();
            if (mesh.mTexture != nullptr && mesh.mTexture->getTextureView() != nullptr) {
                mBindingData[3].textureView = model->getDiffuseTexture()->getTextureView();
            } else {
                mBindingData[3].textureView = mApp->mDefaultDiffuse->getTextureView();
            }
            mBindingData[4].sampler = mApp->getDefaultSampler();

            auto bindgroup = mBindingGroup.createNew(mApp, mBindingData);
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, modelUniformBuffer.getBuffer(), 0, &mScene,
                                 sizeof(mScene));

            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                                 wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));

            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);

            wgpuRenderPassEncoderDraw(encoder, mesh.mVertexData.size(),
                                      model->instance == nullptr ? 1 : model->instance->getInstanceCount(), 0, 0);

            wgpuBufferRelease(modelUniformBuffer.getBuffer());
            wgpuBindGroupRelease(bindgroup);
        }
    }
}

WGPUTextureView ShadowPass::getShadowMapView() { return mShadowDepthTextureView; }

void printMatrix(const glm::mat4& matrix) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            std::cout << matrix[row][col] << " ";
        }
        std::cout << std::endl;
    }
}
