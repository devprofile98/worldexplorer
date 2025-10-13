
#include "water_pass.h"

#include <webgpu/webgpu.h>

#include "application.h"
#include "rendererResource.h"
#include "renderpass.h"
#include "skybox.h"
#include "texture.h"
#include "tracy/Tracy.hpp"

WaterReflectionPass::WaterReflectionPass(Application* app, const std::string& name) : RenderPass(name), mApp(app) {
    // create render target
    mRenderTarget = new Texture{mApp->getRendererResource().device,
                                static_cast<uint32_t>(1920),
                                static_cast<uint32_t>(1022),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                WGPUTextureFormat_BGRA8UnormSrgb,
                                1,
                                "Water render target"};

    // Create the view of the RenderTarget manipulated by the rasterizer
    mRenderTargetView = mRenderTarget->createView();

    // create depth target
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24PlusStencil8;
    mDepthTexture = new Texture{mApp->getRendererResource().device,
                                static_cast<uint32_t>(1920),
                                static_cast<uint32_t>(1022),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                depth_texture_format,
                                1,
                                "Water depth texture"};

    // Create the view of the depth texture manipulated by the rasterizer
    mDepthTextureView = mDepthTexture->createViewDepthStencil();

    // Create Render pipeline
    mDefaultCameraIndex.setLabel("defualt camera index buffer")
        .setMappedAtCraetion()
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mApp);

    static uint32_t cidx = 1;
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mDefaultCameraIndex.getBuffer(), 0, &cidx,
                         sizeof(uint32_t));

    mDefaultCameraIndexBindgroup.addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                           sizeof(uint32_t));

    mDefaultCameraIndexBindingData[0] = {};
    mDefaultCameraIndexBindingData[0].nextInChain = nullptr;
    mDefaultCameraIndexBindingData[0].binding = 0;
    mDefaultCameraIndexBindingData[0].buffer = mDefaultCameraIndex.getBuffer();
    mDefaultCameraIndexBindingData[0].size = sizeof(uint32_t);

    layout = mDefaultCameraIndexBindgroup.createLayout(mApp, "water refletcion bind group layout");

    mDefaultCameraIndexBindgroup.create(mApp, mDefaultCameraIndexBindingData);

    // clip plane bind group configuration
    mDefaultClipPlaneBG.addBuffer(0,  //
                                  BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                  sizeof(glm::vec4));
    mDefaultClipPlaneBG.createLayout(mApp, "water clip plane clip plane bindgroup layout");

    mDefaultClipPlaneBuf.setMappedAtCraetion()
        .setLabel("water reflection cilp plane buffer")
        .setSize(sizeof(glm::vec4))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mApp);

    // glm::vec4 default_clip_plane{0.0, 0.0, 1.0, 100};
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mDefaultClipPlaneBuf.getBuffer(), 0,
                         glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));

    mDefaultClipPlaneBGData[0].nextInChain = nullptr;
    mDefaultClipPlaneBGData[0].binding = 0;
    mDefaultClipPlaneBGData[0].buffer = mDefaultClipPlaneBuf.getBuffer();
    mDefaultClipPlaneBGData[0].offset = 0;
    mDefaultClipPlaneBGData[0].size = sizeof(glm::vec4);

    mDefaultClipPlaneBG.create(mApp, mDefaultClipPlaneBGData);
}

void WaterReflectionPass::createRenderPass(WGPUTextureFormat textureFormat) {
    (void)textureFormat;

    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{
        mApp,
        {layouts[0], layouts[1], layouts[2], layout, layouts[4], layouts[5] /*, layouts[6] , mLayerThree*/},
        "Water Render Pass1"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/water_reflection.wgsl");
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    mRenderPipeline->getDepthStencilState().depthWriteEnabled = WGPUOptionalBool_True;
    mRenderPipeline->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CW, WGPUCullMode_Back);

    mRenderPipeline->createPipeline(mApp);

    setColorAttachment({mRenderTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    setDepthStencilAttachment(
        {mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Store, LoadOp::Load, false, 1.0});
    init();
}

void WaterReflectionPass::execute(WGPUCommandEncoder encoder) {
    // water pass

    WGPURenderPassEncoder pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, getRenderPassDescriptor());

    wgpuRenderPassEncoderSetPipeline(pass_encoder, getPipeline()->getPipeline());
    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

    {
        for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
            model->draw(mApp, pass_encoder);
        }
    }

    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);
}

WaterRefractionPass::WaterRefractionPass(Application* app, const std::string& name) : RenderPass(name), mApp(app) {
    // create render target
    mRenderTarget = new Texture{mApp->getRendererResource().device,
                                static_cast<uint32_t>(1920),
                                static_cast<uint32_t>(1022),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                WGPUTextureFormat_BGRA8UnormSrgb,
                                1,
                                "Water render target"};

    // Create the view of the RenderTarget manipulated by the rasterizer
    mRenderTargetView = mRenderTarget->createView();

    // create depth target
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24PlusStencil8;
    mDepthTexture = new Texture{mApp->getRendererResource().device,
                                static_cast<uint32_t>(1920),
                                static_cast<uint32_t>(1022),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                depth_texture_format,
                                1,
                                "Water depth texture"};

    // Create the view of the depth texture manipulated by the rasterizer
    mDepthTextureView = mDepthTexture->createViewDepthStencil();

    // clip plane bind group configuration
    mDefaultClipPlaneBG.addBuffer(0,  //
                                  BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                  sizeof(glm::vec4));
    mDefaultClipPlaneBG.createLayout(mApp, "water refraction clip plane clip plane bindgroup layout");

    mDefaultClipPlaneBuf.setMappedAtCraetion()
        .setLabel("water refraction cilp plane buffer")
        .setSize(sizeof(glm::vec4))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mApp);

    // glm::vec4 default_clip_plane{0.0, 0.0, 1.0, 100};
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mDefaultClipPlaneBuf.getBuffer(), 0,
                         glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));

    mDefaultClipPlaneBGData[0].nextInChain = nullptr;
    mDefaultClipPlaneBGData[0].binding = 0;
    mDefaultClipPlaneBGData[0].buffer = mDefaultClipPlaneBuf.getBuffer();
    mDefaultClipPlaneBGData[0].offset = 0;
    mDefaultClipPlaneBGData[0].size = sizeof(glm::vec4);

    mDefaultClipPlaneBG.create(mApp, mDefaultClipPlaneBGData);
}

void WaterRefractionPass::execute(WGPUCommandEncoder encoder) {
    WGPURenderPassEncoder pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, getRenderPassDescriptor());

    wgpuRenderPassEncoderSetPipeline(pass_encoder, getPipeline()->getPipeline());
    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mApp->mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mApp->mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

    {
        for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
            if (model->mName != "water") {
                model->draw(mApp, pass_encoder);
            }
        }
    }
    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);
}

void WaterRefractionPass::createRenderPass(WGPUTextureFormat textureFormat) {
    (void)textureFormat;

    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{
        mApp,
        {layouts[0], layouts[1], layouts[2], layouts[3], layouts[4], layouts[5] /*, layouts[6] , mLayerThree*/},
        "Water Render Pass1"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    // mRenderPipeline->setShader(RESOURCE_DIR "/editor.wgsl");
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    mRenderPipeline->getDepthStencilState().depthWriteEnabled = WGPUOptionalBool_True;
    mRenderPipeline->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CW, WGPUCullMode_Back);

    mRenderPipeline->createPipeline(mApp);

    setColorAttachment({mRenderTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Clear});
    setDepthStencilAttachment(
        {mDepthTextureView, StoreOp::Store, LoadOp::Clear, false, StoreOp::Store, LoadOp::Load, false, 1.0});
    init();
}

WaterPass::WaterPass(Application* app, const std::string& name) : RenderPass(name), mApp(app) {
    mWaterPass = new WaterReflectionPass{app, "Water Reflection Pass"};
    mWaterPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

    mWaterRefractionPass = new WaterRefractionPass{app, "Water Refraction Pass"};
    mWaterRefractionPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

    mTerrainForRefraction = new NewRenderPass{"Terrain for refraction"};

    mTerrainForRefraction->setColorAttachment({mWaterRefractionPass->mRenderTargetView, nullptr,
                                               WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mTerrainForRefraction->setDepthStencilAttachment({mWaterRefractionPass->mDepthTextureView, StoreOp::Store,
                                                      LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined,
                                                      false});
    mTerrainForRefraction->init();

    //
    mTerrainForReflection = new NewRenderPass{"Terrain for reflection"};
    mTerrainForReflection->setColorAttachment(
        {mWaterPass->mRenderTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mTerrainForReflection->setDepthStencilAttachment({mWaterPass->mDepthTextureView, StoreOp::Store, LoadOp::Load,
                                                      false, StoreOp::Undefined, LoadOp::Undefined, false});
    mTerrainForReflection->init();
    /////////////////

    mWaterTextureBindGroup.addTexture(0,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                      TextureViewDimension::VIEW_2D);
    mWaterTextureBindGroup.addTexture(1,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                      TextureViewDimension::VIEW_2D);

    mWaterTextureBindngData[0] = {};
    mWaterTextureBindngData[0].nextInChain = nullptr;
    mWaterTextureBindngData[0].binding = 0;
    mWaterTextureBindngData[0].textureView = mWaterPass->mRenderTarget->getTextureView();

    mWaterTextureBindngData[1] = {};
    mWaterTextureBindngData[1].nextInChain = nullptr;
    mWaterTextureBindngData[1].binding = 1;
    mWaterTextureBindngData[1].textureView = mWaterRefractionPass->mRenderTarget->getTextureView();

    mBindGroupLayout = mWaterTextureBindGroup.createLayout(mApp, "water refletcion bind group layout");

    mWaterTextureBindGroup.create(mApp, mWaterTextureBindngData);
}

void WaterPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp,
                                   {layouts[0], layouts[1], layouts[2], layouts[3], mBindGroupLayout /*, mLayerThree*/},
                                   "Water Render Pass"};

    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout
                                   .addAttribute(0, 0, WGPUVertexFormat_Float32x3)  // for position
                                   .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
                                   .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
                                   .addAttribute(9 * sizeof(float), 3, WGPUVertexFormat_Float32x3)
                                   .addAttribute(12 * sizeof(float), 4, WGPUVertexFormat_Float32x3)
                                   .addAttribute(offsetof(VertexAttributes, uv), 5, WGPUVertexFormat_Float32x2)
                                   .addAttribute(offsetof(VertexAttributes, boneIds), 6, WGPUVertexFormat_Sint32x4)
                                   .addAttribute(offsetof(VertexAttributes, weights), 7, WGPUVertexFormat_Float32x4)
                                   .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);

    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());

    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/water.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState()
        .setPrimitiveState(WGPUFrontFace_CW, WGPUCullMode_Back)
        .setDepthStencilState(true, 0xFF, 0xFF, WGPUTextureFormat_Depth24PlusStencil8)
        .setBlendState()
        .setColorTargetState(textureFormat)
        .setFragmentState();

    mRenderPipeline->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    mRenderPipeline->setMultiSampleState().createPipeline(mApp);
    mRenderPipeline->createPipeline(mApp);
}

void WaterPass::drawWater() {
    // ---------------- 2 - begining of the opaque object color pass ---------------

    // inverting pitch and reflect camera based on the water plane
    // auto water_plane = ModelRegistry::instance().getLoadedModel(Visibility_User).find("water");
    for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
        if (model->mName == "water") {
            // if (model != ModelRegistry::instance().getLoadedModel(Visibility_User).end()) {
            static Camera camera = mApp->getCamera();
            camera = mApp->getCamera();
            static CameraInfo muniform = mApp->getUniformData();
            float diff = 2 * (camera.getPos().z - model->mTransform.getPosition().z);
            auto new_pos = camera.getPos();
            new_pos.z -= diff;
            camera.setPosition(new_pos);
            camera.mPitch *= -1.0;
            camera.updateCamera();
            muniform.setCamera(camera);
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->getUniformBuffer().getBuffer(),
                                 sizeof(CameraInfo), &muniform, sizeof(CameraInfo));
            glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(muniform.viewMatrix));
            glm::mat4 mvp = muniform.projectMatrix * viewNoTranslation;
            auto reflected_camera = mvp;
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->mSkybox->mReflectedCameraMatrix.getBuffer(),
                                 0, &reflected_camera, sizeof(glm::mat4));
            break;
        }
    }

    {
        WGPURenderPassDescriptor render_pass_descriptor = {};

        {
            render_pass_descriptor.nextInChain = nullptr;

            static WGPURenderPassColorAttachment color_attachment = {};
            color_attachment.view = mWaterPass->mRenderTargetView;
            color_attachment.resolveTarget = nullptr;
            color_attachment.loadOp = WGPULoadOp_Clear;
            color_attachment.storeOp = WGPUStoreOp_Store;
            color_attachment.clearValue = WGPUColor{0.52, 0.80, 0.92, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
            color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

            render_pass_descriptor.colorAttachmentCount = 1;
            render_pass_descriptor.colorAttachments = &color_attachment;

            static WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
            depth_stencil_attachment.view = mWaterPass->mDepthTextureView;
            depth_stencil_attachment.depthClearValue = 1.0f;
            depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
            depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
            depth_stencil_attachment.depthReadOnly = false;
            depth_stencil_attachment.stencilClearValue = 0;
            depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
            depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
            depth_stencil_attachment.stencilReadOnly = false;
            render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
            render_pass_descriptor.timestampWrites = nullptr;
        }

        WGPURenderPassEncoder render_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(mApp->getRendererResource().commandEncoder, &render_pass_descriptor);
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(mApp->getUniformData().viewMatrix));
        glm::mat4 mvp = mApp->getUniformData().projectMatrix * viewNoTranslation;
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mApp->mSkybox->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mApp->mSkybox->mReflectedBindingGroup.getBindGroup(),
                                          0, nullptr);
        mApp->mSkybox->draw(mApp, render_pass_encoder, mvp);

        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }

    mWaterPass->execute(mApp->getRendererResource().commandEncoder);

    // ---------- Terrain Render Pass for Water Reflection
    NewRenderPass::beginPass(
        mTerrainForReflection, mApp->getRendererResource().commandEncoder, [&](WGPURenderPassEncoder pass_encoder) {
            wgpuRenderPassEncoderSetPipeline(pass_encoder, mApp->mTerrainPass->getPipeline()->getPipeline());
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mWaterPass->mDefaultCameraIndexBindgroup.getBindGroup(),
                                              0, nullptr);

            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mWaterPass->mDefaultClipPlaneBG.getBindGroup(), 0,
                                              nullptr);
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

            mApp->terrain.draw(mApp, pass_encoder, mApp->mBindingData);

            wgpuRenderPassEncoderEnd(pass_encoder);
        });

    //----------------------------  Water Refraction Pass
    mWaterRefractionPass->execute(mApp->getRendererResource().commandEncoder);
    // ----------------------------------------------

    // ---------- Terrain Render Pass for Water Refraction

    NewRenderPass::beginPass(
        mTerrainForRefraction, mApp->getRendererResource().commandEncoder, [&](WGPURenderPassEncoder pass_encoder) {
            wgpuRenderPassEncoderSetPipeline(pass_encoder, mApp->mTerrainPass->getPipeline()->getPipeline());
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mApp->mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                              nullptr);
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mWaterRefractionPass->mDefaultClipPlaneBG.getBindGroup(),
                                              0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

            mApp->terrain.draw(mApp, pass_encoder, mApp->mBindingData);

            wgpuRenderPassEncoderEnd(pass_encoder);
        });
}

void WaterPass::waterBlend() {
    // {
    ZoneScopedNC("Water pass", 0x00F0FF);
    setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Store, LoadOp::Load, false, 1.0});
    init();

    WGPURenderPassEncoder water_render_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(mApp->getRendererResource().commandEncoder, getRenderPassDescriptor());
    for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
        if (model->mName == "water") {
            wgpuRenderPassEncoderSetPipeline(water_render_pass_encoder, getPipeline()->getPipeline());
            wgpuRenderPassEncoderSetBindGroup(water_render_pass_encoder, 3,
                                              mApp->mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);

            wgpuRenderPassEncoderSetBindGroup(water_render_pass_encoder, 4, mWaterTextureBindGroup.getBindGroup(), 0,
                                              nullptr);
            if (!model->isTransparent() == true) {
                model->draw(mApp, water_render_pass_encoder);
            }
        }
    }
    wgpuRenderPassEncoderEnd(water_render_pass_encoder);
    wgpuRenderPassEncoderRelease(water_render_pass_encoder);
}
