#include "shadow_pass.h"

#include <cstring>
#include <format>

#include "application.h"
#include "glm/ext.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "gpu_buffer.h"
#include "model.h"
#include "webgpu.h"

bool should = false;

ShadowPass::ShadowPass(Application* app) { mApp = app; }

void ShadowPass::createRenderPassDescriptor2() {
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor shadow_depth_texture_descriptor = {};
    shadow_depth_texture_descriptor.label = "Shadow Mapping Target Texture 22:)";
    shadow_depth_texture_descriptor.nextInChain = nullptr;
    shadow_depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    shadow_depth_texture_descriptor.format = depth_texture_format;
    shadow_depth_texture_descriptor.mipLevelCount = 1;
    shadow_depth_texture_descriptor.sampleCount = 1;
    shadow_depth_texture_descriptor.size = {static_cast<uint32_t>(2048), static_cast<uint32_t>(2048), 1};
    shadow_depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    shadow_depth_texture_descriptor.viewFormatCount = 1;
    shadow_depth_texture_descriptor.viewFormats = &depth_texture_format;
    mShadowDepthTexture2 =
        wgpuDeviceCreateTexture(mApp->getRendererResource().device, &shadow_depth_texture_descriptor);

    WGPUTextureViewDescriptor sdtv_desc = {};  // shadow depth texture view
    sdtv_desc.label = "SDTV desc 2";
    sdtv_desc.aspect = WGPUTextureAspect_DepthOnly;
    sdtv_desc.baseArrayLayer = 0;
    sdtv_desc.arrayLayerCount = 1;
    sdtv_desc.baseMipLevel = 0;
    sdtv_desc.mipLevelCount = 1;
    sdtv_desc.dimension = WGPUTextureViewDimension_2D;
    sdtv_desc.format = depth_texture_format;
    mShadowDepthTextureView2 = wgpuTextureCreateView(mShadowDepthTexture2, &sdtv_desc);

    /*render_target = new Texture{mApp->getRendererResource().device, 2048, 2048, TextureDimension::TEX_2D,*/
    /*                            WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment};*/
    /*render_target->createView();*/

    mRenderPassColorAttachment2.view = render_target->getTextureView();
    mRenderPassColorAttachment2.resolveTarget = nullptr;
    mRenderPassColorAttachment2.loadOp = WGPULoadOp_Load;
    mRenderPassColorAttachment2.storeOp = WGPUStoreOp_Discard;
    mRenderPassColorAttachment2.clearValue = WGPUColor{0.02, 0.80, 0.92, 1.0};

    mRenderPassDesc2 = {};
    mRenderPassDesc2.nextInChain = nullptr;
    mRenderPassDesc2.colorAttachmentCount = 1;
    mRenderPassDesc2.colorAttachments = &mRenderPassColorAttachment2;

    static WGPURenderPassDepthStencilAttachment shadow_pass_depth_stencil_attachment;
    shadow_pass_depth_stencil_attachment.view = mShadowDepthTextureView2;
    shadow_pass_depth_stencil_attachment.depthClearValue = 1.0f;
    shadow_pass_depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
    shadow_pass_depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
    shadow_pass_depth_stencil_attachment.depthReadOnly = false;

    shadow_pass_depth_stencil_attachment.stencilClearValue = 0.0f;
    shadow_pass_depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
    shadow_pass_depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
    shadow_pass_depth_stencil_attachment.stencilReadOnly = true;

    mRenderPassDesc2.depthStencilAttachment = &shadow_pass_depth_stencil_attachment;
    mRenderPassDesc2.timestampWrites = nullptr;
}

void ShadowPass::createRenderPassDescriptor() {
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
}

void ShadowPass::createRenderPass() {
    // creating pipeline
    createRenderPassDescriptor();
    createRenderPassDescriptor2();
    // for projection
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(Scene));
    mBindingGroup.addBuffer(1,  //
                            BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                            mApp->mInstanceManager->mBufferSize);

    mBindingGroup.addBuffer(2, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                            sizeof(ObjectInfo));

    mBindingGroup.addSampler(3,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    mTextureBindingGroup.addTexture(0,  //
                                    BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);

    mTextureBindingGroup.addTexture(1,  //
                                    BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);
    mTextureBindingGroup.addTexture(2,  //
                                    BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");
    auto texture_bind_group_layout = mTextureBindingGroup.createLayout(mApp, "shadow pass pipeline");

    mRenderPipeline = new Pipeline{mApp, {bind_group_layout, texture_bind_group_layout}};
    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout
                                   .addAttribute(0, 0, WGPUVertexFormat_Float32x3)  // for position
                                   .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
                                   .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
                                   .addAttribute(9 * sizeof(float), 3, WGPUVertexFormat_Float32x3)
                                   .addAttribute(12 * sizeof(float), 4, WGPUVertexFormat_Float32x3)
                                   .addAttribute(offsetof(VertexAttributes, uv), 5, WGPUVertexFormat_Float32x2)
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
    mBindingData[3].binding = 3;
    mBindingData[3].sampler = mApp->getDefaultSampler();

    mTextureBindingData[0] = {};
    mTextureBindingData[0].nextInChain = nullptr;
    mTextureBindingData[0].binding = 0;
    mTextureBindingData[0].textureView = nullptr;

    mTextureBindingData[1] = {};
    mTextureBindingData[1].nextInChain = nullptr;
    mTextureBindingData[1].binding = 1;
    mTextureBindingData[1].textureView = nullptr;

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

glm::mat4 createProjectionFromFrustumCorner(const std::vector<glm::vec4>& corners, const glm::mat4& lightView,
                                            float* mm, const char* name, float dis) {
    (void)name;
    (void)dis;
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

    /*constexpr float zMult = 10.0f;*/
    /*if (minZ < 0) {*/
    /*    minZ *= zMult;*/
    /*} else {*/
    /*    minZ /= zMult;*/
    /*}*/
    /*if (maxZ < 0) {*/
    /*    maxZ /= zMult;*/
    /*} else {*/
    /*    maxZ *= zMult;*/
    /*}*/
    /*if (should) {*/
    maxZ += 20.0f;
    minZ -= 20.0f;
    /*if (std::strcmp(name, "Near") == 0) {*/
    /*    std::cout << name << " minZ: " << minZ << "  maxZ: " << maxZ << std::endl;*/
    /*}*/

    *mm = minZ;
    return glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
}

std::vector<Scene> ShadowPass::createFrustumSplits(std::vector<glm::vec4>& corners, float length, float far_length,
                                                   float distance, float dd) {
    (void)distance;
    auto middle0 = corners[0] + (glm::normalize(corners[1] - corners[0]) * length);
    auto middle1 = corners[2] + (glm::normalize(corners[3] - corners[2]) * length);
    auto middle2 = corners[4] + (glm::normalize(corners[5] - corners[4]) * length);
    auto middle3 = corners[6] + (glm::normalize(corners[7] - corners[6]) * length);

    auto Far0 = corners[0] + (glm::normalize(corners[1] - corners[0]) * (length + far_length));
    auto Far1 = corners[2] + (glm::normalize(corners[3] - corners[2]) * (length + far_length));
    auto Far2 = corners[4] + (glm::normalize(corners[5] - corners[4]) * (length + far_length));
    auto Far3 = corners[6] + (glm::normalize(corners[7] - corners[6]) * (length + far_length));

    this->corners = corners;
    mNear = corners;
    mFar = corners;

    mMiddle = {middle0, middle1, middle2, middle3};
    // near
    mNear[1] = middle0;
    mNear[3] = middle1;
    mNear[5] = middle2;
    mNear[7] = middle3;

    // far
    mFar[0] = middle0;
    mFar[2] = middle1;
    mFar[4] = middle2;
    mFar[6] = middle3;

    mFar[1] = Far0;
    mFar[3] = Far1;
    mFar[5] = Far2;
    mFar[7] = Far3;

    mScenes.clear();

    auto all_corners = {mNear, mFar};

    (void)dd;
    bool fff = false;
    for (const auto& c : all_corners) {
        glm::vec3 cent = glm::vec3(0, 0, 0);
        for (const auto& v : c) {
            cent += glm::vec3(v);
        }
        cent /= c.size();

        this->center = cent;
        glm::vec3 lightDirection = glm::normalize(-this->lightPos);
        glm::vec3 lightPosition =
            this->center - lightDirection * (!fff ? glm::length(mNear[0] - mNear[7]) / 5.0f
                                                  : glm::length(mFar[0] - mFar[7]) / 5.0f);  // Push light back

        auto view = glm::lookAt(lightPosition, this->center, glm::vec3{0.0f, 0.0f, 1.0f});
        /*std::cout << (!fff ? "Near" : "Far") << " is "*/
        /*          << (!fff ? glm::length(mNear[0] - mNear[7]) : glm::length(mFar[0] - mFar[7])) << '\n';*/
        glm::mat4 projection = createProjectionFromFrustumCorner(c, view, &MinZ, !fff ? "Near" : "Far", distance);
        fff = !fff;
        mScenes.emplace_back(Scene{projection, glm::mat4{1.0}, view});
    }
    return mScenes;
}

Pipeline* ShadowPass::getPipeline() { return mRenderPipeline; }

WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor() { return &mRenderPassDesc; }
WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor2() { return &mRenderPassDesc2; }

void ShadowPass::render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder, size_t which) {
    /*auto& render_resource = mApp->getRendererResource();*/
    for (auto* model : models) {
        for (auto& [mat_id, mesh] : model->mMeshes) {
            Buffer modelUniformBuffer = {};
            modelUniformBuffer.setLabel("Model Uniform Buffer")
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .setSize(sizeof(Scene))
                .setMappedAtCraetion()
                .create(mApp);

            if (which >= mScenes.size()) {
                continue;
            };

            /*if (mBindgroups[which] == nullptr) {*/
            /*    mBindingData[0].buffer = modelUniformBuffer.getBuffer();*/
            /*    mBindingData[1].buffer = mApp->mInstanceManager->getInstancingBuffer().getBuffer();*/
            /*    mBindingData[2].buffer = model->getUniformBuffer().getBuffer();*/
            /*    mBindingData[3].sampler = mApp->getDefaultSampler();*/
            /**/
            /*    mBindgroups[which] = mBindingGroup.createNew(mApp, mBindingData);*/
            /*}*/

            mScenes[which].model = model->getTranformMatrix();
            mBindingData[0].buffer = modelUniformBuffer.getBuffer();
            mBindingData[1].buffer = mApp->mInstanceManager->getInstancingBuffer().getBuffer();
            mBindingData[2].buffer = model->getUniformBuffer().getBuffer();
            mBindingData[3].sampler = mApp->getDefaultSampler();

            auto bindgroup = mBindingGroup.createNew(mApp, mBindingData);
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, modelUniformBuffer.getBuffer(), 0,
                                 &mScenes.at(which), sizeof(Scene));

            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                                 wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));

            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(encoder, 1,
                                              mesh.mTextureBindGroup == nullptr
                                                  ? mApp->mDefaultTextureBindingGroup.getBindGroup()
                                                  : mesh.mTextureBindGroup,
                                              0, nullptr);

            wgpuRenderPassEncoderDraw(encoder, mesh.mVertexData.size(),
                                      model->instance == nullptr ? 1 : model->instance->getInstanceCount(), 0, 0);

            wgpuBufferRelease(modelUniformBuffer.getBuffer());
            wgpuBindGroupRelease(bindgroup);
        }
    }
}

std::vector<Scene>& ShadowPass::getScene() { return mScenes; }

WGPUTextureView ShadowPass::getShadowMapView() { return mShadowDepthTextureView; }
WGPUTextureView ShadowPass::getShadowMapView2() { return mShadowDepthTextureView2; }

void printMatrix(const glm::mat4& matrix) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            std::cout << matrix[row][col] << " ";
        }
        std::cout << std::endl;
    }
}
