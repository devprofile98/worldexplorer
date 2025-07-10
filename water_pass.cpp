
#include "water_pass.h"

#include "application.h"
#include "webgpu.h"

WaterReflectionPass::WaterReflectionPass(Application* app) : mApp(app) {
    // create render target
    // create depth target
    //

    mRenderTarget = new Texture{mApp->getRendererResource().device,
                                static_cast<uint32_t>(1920),
                                static_cast<uint32_t>(1022),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                WGPUTextureFormat_BGRA8UnormSrgb,
                                1,
                                "Water render target"};

    // Create the view of the depth texture manipulated by the rasterizer
    mRenderTargetView = mRenderTarget->createView();

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

    mDefaultCameraIndex.setLabel("defualt camera index buffer")
        .setMappedAtCraetion()
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mApp);

    static uint32_t cidx = 1;
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mDefaultCameraIndex.getBuffer(), 0, &cidx,
                         sizeof(uint32_t));

    mDefaultCameraIndexBindgroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM,
                                           sizeof(uint32_t));

    mDefaultCameraIndexBindingData[0] = {};
    mDefaultCameraIndexBindingData[0].nextInChain = nullptr;
    mDefaultCameraIndexBindingData[0].binding = 0;
    mDefaultCameraIndexBindingData[0].buffer = mDefaultCameraIndex.getBuffer();
    mDefaultCameraIndexBindingData[0].size = sizeof(uint32_t);

    layout = mDefaultCameraIndexBindgroup.createLayout(mApp, "water refletcion bind group layout");

    mDefaultCameraIndexBindgroup.create(mApp, mDefaultCameraIndexBindingData);
}

void WaterReflectionPass::createRenderPass(WGPUTextureFormat textureFormat) {
    (void)textureFormat;

    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], layout /*, mLayerThree*/}, "Water Render Pass1"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    // mRenderPipeline->setShader(RESOURCE_DIR "/editor.wgsl");
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    mRenderPipeline->getDepthStencilState().depthWriteEnabled = true;
    mRenderPipeline->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CW, WGPUCullMode_Back);

    mRenderPipeline->createPipeline(mApp);
}

WaterPass::WaterPass(Application* app) : mApp(app) {}

void WaterPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], layouts[3] /*, mLayerThree*/}, "Water Render Pass"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    // mRenderPipeline->setShader(RESOURCE_DIR "/editor.wgsl");
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    mRenderPipeline->getDepthStencilState().depthWriteEnabled = true;
    mRenderPipeline->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CW, WGPUCullMode_Back);

    mRenderPipeline->createPipeline(mApp);
}
