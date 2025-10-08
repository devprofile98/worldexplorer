
#include "terrain_pass.h"

#include <webgpu/webgpu.h>

#include <cstdint>

#include "application.h"
#include "renderpass.h"
#include "utils.h"

// ifdef WEBGPU_BACKEND

TerrainPass::TerrainPass(Application* app, const std::string& name) : RenderPass(name) { mApp = app; }

void TerrainPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], layouts[3], layouts[4]}, "Terrain pipeline"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/terrain.wgsl");
    setDefaultActiveStencil2(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CCW, WGPUCullMode_Back);
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}

OutlinePass::OutlinePass(Application* app, const std::string& name) : RenderPass(name) {
    mApp = app;

    mDepthTextureBindgroup.addTexture(0,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH,
                                      TextureViewDimension::VIEW_2D);

    mLayerThree = mDepthTextureBindgroup.createLayout(app, "layer three bidngroup");
}

void OutlinePass::createRenderPass(WGPUTextureFormat textureFormat) {
    mDepthTextureBindgroup.create(mApp, mOutlineSpecificBindingData);
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], mLayerThree, layouts[3], layouts[5]}, "Outline Pass"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/outline.wgsl");
    setDefaultUseStencil(mRenderPipeline->getDepthStencilState());
    /*setDefaultActiveStencil(mRenderPipeline->getDepthStencilState());*/
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->createPipeline(mApp);
}

void OutlinePass::createSomeBinding() {
    mOutlineSpecificBindingData[0] = {};
    mOutlineSpecificBindingData[0].nextInChain = nullptr;
    mOutlineSpecificBindingData[0].binding = 0;
    mOutlineSpecificBindingData[0].textureView = mTextureView;

    mDepthTextureBindgroup.create(mApp, mOutlineSpecificBindingData);
}

Pipeline* OutlinePass::create(WGPUTextureFormat textureFormat, WGPUTextureView textureview) {
    mTextureView = textureview;
    mOutlineSpecificBindingData[0] = {};
    mOutlineSpecificBindingData[0].nextInChain = nullptr;
    mOutlineSpecificBindingData[0].binding = 0;
    mOutlineSpecificBindingData[0].textureView = mTextureView;
    createRenderPass(textureFormat);
    return mRenderPipeline;
}

// ----------------------------------- 3D viewport item render pass
ViewPort3DPass::ViewPort3DPass(Application* app, const std::string& name) : RenderPass(name) {
    mApp = app;
    mLayerThreeBindgroup.addBuffer(0,  //
                                   BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                   sizeof(int32_t));

    mLayerThree = mLayerThreeBindgroup.createLayout(app, "layer three bidngroup");
    mRenderPipeline = nullptr;
}

void ViewPort3DPass::initTargets() {
    setColorAttachment(
        {mApp->getColorTarget(), nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    setDepthStencilAttachment({mApp->getDepthStencilTarget(), StoreOp::Undefined, LoadOp::Undefined, false,
                               StoreOp::Undefined, LoadOp::Undefined, false, 0.0});
    init();
}

void ViewPort3DPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp, {layouts[0], layouts[1], layouts[2] /*, mLayerThree*/}, "3D ViewPort Pass"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/editor.wgsl");
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    mRenderPipeline->createPipeline(mApp);
    initTargets();
}

Pipeline* ViewPort3DPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}

static uint32_t stencilRefValue = 240;
void ViewPort3DPass::execute(WGPUCommandEncoder encoder) {
    initTargets();
    WGPURenderPassEncoder pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, getRenderPassDescriptor());
    wgpuRenderPassEncoderSetStencilReference(pass_encoder, stencilRefValue);

    for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_Editor)) {
        wgpuRenderPassEncoderSetPipeline(pass_encoder, getPipeline()->getPipeline());
        model->draw(mApp, pass_encoder);
    }

    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);
}

void drawWater(Application* app) {
    // ---------------- 2 - begining of the opaque object color pass ---------------

    // inverting pitch and reflect camera based on the water plane
    // auto water_plane = ModelRegistry::instance().getLoadedModel(Visibility_User).find("water");
    for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
        if (model->mName == "water") {
            // if (model != ModelRegistry::instance().getLoadedModel(Visibility_User).end()) {
            static Camera camera = app->getCamera();
            camera = app->getCamera();
            static CameraInfo muniform = app->getUniformData();
            float diff = 2 * (camera.getPos().z - model->mTransform.getPosition().z);
            auto new_pos = camera.getPos();
            new_pos.z -= diff;
            camera.setPosition(new_pos);
            camera.mPitch *= -1.0;
            camera.updateCamera();
            muniform.setCamera(camera);
            wgpuQueueWriteBuffer(app->getRendererResource().queue, app->getUniformBuffer().getBuffer(),
                                 sizeof(CameraInfo), &muniform, sizeof(CameraInfo));
            glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(muniform.viewMatrix));
            glm::mat4 mvp = muniform.projectMatrix * viewNoTranslation;
            auto reflected_camera = mvp;
            wgpuQueueWriteBuffer(app->getRendererResource().queue, app->mSkybox->mReflectedCameraMatrix.getBuffer(), 0,
                                 &reflected_camera, sizeof(glm::mat4));
            break;
        }
    }

    {
        WGPURenderPassDescriptor render_pass_descriptor = {};

        {
            render_pass_descriptor.nextInChain = nullptr;

            static WGPURenderPassColorAttachment color_attachment = {};
            color_attachment.view = app->mWaterPass->mRenderTargetView;
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
            depth_stencil_attachment.view = app->mWaterPass->mDepthTextureView;
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
            wgpuCommandEncoderBeginRenderPass(app->getRendererResource().commandEncoder, &render_pass_descriptor);
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(app->getUniformData().viewMatrix));
        glm::mat4 mvp = app->getUniformData().projectMatrix * viewNoTranslation;
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, app->mSkybox->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, app->mSkybox->mReflectedBindingGroup.getBindGroup(),
                                          0, nullptr);
        app->mSkybox->draw(app, render_pass_encoder, mvp);

        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }

    app->mWaterPass->execute(app->getRendererResource().commandEncoder);

    // ---------- Terrain Render Pass for Water Reflection
    NewRenderPass::beginPass(
        app->mTerrainForReflection, app->getRendererResource().commandEncoder, [&](WGPURenderPassEncoder pass_encoder) {
            wgpuRenderPassEncoderSetPipeline(pass_encoder, app->mTerrainPass->getPipeline()->getPipeline());
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3,
                                              app->mWaterPass->mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);

            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, app->mWaterPass->mDefaultClipPlaneBG.getBindGroup(), 0,
                                              nullptr);
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, app->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

            app->terrain.draw(app, pass_encoder, app->mBindingData);

            wgpuRenderPassEncoderEnd(pass_encoder);
        });

    //----------------------------  Water Refraction Pass
    app->mWaterRefractionPass->execute(app->getRendererResource().commandEncoder);
    // ----------------------------------------------

    // ---------- Terrain Render Pass for Water Refraction

    NewRenderPass::beginPass(
        app->mTerrainForRefraction, app->getRendererResource().commandEncoder, [&](WGPURenderPassEncoder pass_encoder) {
            wgpuRenderPassEncoderSetPipeline(pass_encoder, app->mTerrainPass->getPipeline()->getPipeline());
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, app->mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                              nullptr);
            wgpuRenderPassEncoderSetBindGroup(
                pass_encoder, 4, app->mWaterRefractionPass->mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, app->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

            app->terrain.draw(app, pass_encoder, app->mBindingData);

            wgpuRenderPassEncoderEnd(pass_encoder);
        });
}
