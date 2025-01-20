#include "shadow_pass.h"

#include "application.h"

ShadowPass::ShadowPass(Application* app) { mApp = app; }

/*
// Sample vertex shader for shadow pass

layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}


*/

void ShadowPass::createRenderPass() {
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor shadow_depth_texture_descriptor = {};
    shadow_depth_texture_descriptor.nextInChain = nullptr;
    shadow_depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    shadow_depth_texture_descriptor.format = depth_texture_format;
    shadow_depth_texture_descriptor.mipLevelCount = 1;
    shadow_depth_texture_descriptor.sampleCount = 1;
    shadow_depth_texture_descriptor.size = {static_cast<uint32_t>(1024), static_cast<uint32_t>(1024), 1};
    shadow_depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment;
    shadow_depth_texture_descriptor.viewFormatCount = 1;
    shadow_depth_texture_descriptor.viewFormats = &depth_texture_format;
    mShadowDepthTexture = wgpuDeviceCreateTexture(mApp->getRendererResource().device, &shadow_depth_texture_descriptor);

    WGPUTextureViewDescriptor sdtv_desc = {};  // shadow depth texture view
    sdtv_desc.label = "test";
    sdtv_desc.aspect = WGPUTextureAspect_DepthOnly;
    sdtv_desc.baseArrayLayer = 0;
    sdtv_desc.arrayLayerCount = 1;
    sdtv_desc.baseMipLevel = 0;
    sdtv_desc.mipLevelCount = 1;
    sdtv_desc.dimension = WGPUTextureViewDimension_2D;
    sdtv_desc.format = depth_texture_format;
    mShadowDepthTextureView = wgpuTextureCreateView(mShadowDepthTexture, &sdtv_desc);

    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.colorAttachmentCount = 0;
    mRenderPassDesc.colorAttachments = nullptr;

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
    // for model
    // mBindingGroup.addBuffer(1, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(glm::mat4));
    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");

    mRenderPipeline = new Pipeline{mApp, {bind_group_layout}};
    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout
                                   .addAttribute(0, 0, WGPUVertexFormat_Float32x3)  // for position
                                   .configure(sizeof(glm::vec3), VertexStepMode::VERTEX);
    (void)d;

    static WGPUFragmentState mFragmentState = {};
    mFragmentState.nextInChain = nullptr;
    mFragmentState.module = nullptr;
    mFragmentState.entryPoint = nullptr;
    mFragmentState.constants = nullptr;
    mFragmentState.constantCount = 0;
    mFragmentState.targetCount = 0;
    mFragmentState.targets = nullptr;
    (void)mFragmentState;

    mRenderPipeline->setShader(RESOURCE_DIR "/shadow.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState()
        .setPrimitiveState()
        .setDepthStencilState(true, 0xFF, 0xFF)
        .setBlendState();
    // .setColorTargetState()
    // .setFragmentState(mFragmentState)

    mRenderPipeline->getDescriptorPtr()->fragment = nullptr;

    mRenderPipeline->setMultiSampleState().createPipeline(mApp);

    // creating and writing to buffer
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.label = "shadow map pass scene buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.size = sizeof(Scene);
    buffer_descriptor.mappedAtCreation = false;
    mSceneUniformBuffer = wgpuDeviceCreateBuffer(mApp->getRendererResource().device, &buffer_descriptor);

    // fill bindgroup
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mSceneUniformBuffer;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(Scene);

    mBindingGroup.create(mApp, mBindingData);
}

void ShadowPass::setupScene(const glm::vec3 lightPos) {
    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

    auto view = glm::lookAt(lightPos, glm::vec3{0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
    mScene.projection = projection * view;
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mSceneUniformBuffer, offsetof(Scene, projection), &mScene,
                         sizeof(glm::mat4));
}

Pipeline* ShadowPass::getPipeline() { return mRenderPipeline; }

WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor() { return &mRenderPassDesc; }

void ShadowPass::render(std::vector<Model*> models, WGPURenderPassEncoder encoder) {
    auto& render_resource = mApp->getRendererResource();
    // auto& uniform_data = mApp->getUniformData();
    for (auto* model : models) {
        // WGPUBindGroup active_bind_group = nullptr;

        // Bind Diffuse texture for the model if existed
        // if (mTexture != nullptr) {
        //     if (mTexture->getTextureView() != nullptr) {
        //         bindingData[1].nextInChain = nullptr;
        //         bindingData[1].binding = 1;
        //         bindingData[1].textureView = mTexture->getTextureView();
        //     }
        //     if (mSpecularTexture != nullptr && mSpecularTexture->getTextureView() != nullptr) {
        //         bindingData[5].nextInChain = nullptr;
        //         bindingData[5].binding = 5;
        //         bindingData[5].textureView = mSpecularTexture->getTextureView();
        //     }
        //     auto& desc = app->getBindingGroup().getDescriptor();
        //     desc.entries = bindingData.data();
        //     mBindGroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
        //     active_bind_group = mBindGroup;
        // } else {
        //     active_bind_group = app->getBindingGroup().getBindGroup();
        // }

        // mScene.projection = glm::mat4{1.0f};
        mScene.model = model->getModelMatrix();
        wgpuQueueWriteBuffer(render_resource.queue, mSceneUniformBuffer, offsetof(Scene, model), &mScene,
                             sizeof(glm::mat4));

        // wgpuQueueWriteBuffer(render_resource.queue, mApp->getUniformBuffer(), offsetof(MyUniform, modelMatrix),
        //                      glm::value_ptr(uniform_data.modelMatrix), sizeof(glm::mat4));

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, model->getVertexBuffer(), 0,
                                             wgpuBufferGetSize(model->getVertexBuffer()));

        wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindingGroup.getBindGroup(), 0, nullptr);

        // wgpuQueueWriteBuffer(render_resource.queue, mUniformBuffer, 0, &mObjectInfo, sizeof(ObjectInfo));
        // createSomeBinding(app);
        // wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);

        wgpuRenderPassEncoderDraw(encoder, model->getVertexCount(), 1, 0, 0);
    }
}