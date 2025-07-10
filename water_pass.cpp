
#include "water_pass.h"

#include "application.h"
#include "webgpu.h"

WaterReflectionPass::WaterReflectionPass(Application* app) : mApp(app) {
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

WaterRefractionPass::WaterRefractionPass(Application* app) : mApp(app) {
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
}

void WaterRefractionPass::createRenderPass(WGPUTextureFormat textureFormat) {
    (void)textureFormat;

    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], layouts[3] /*, mLayerThree*/}, "Water Render Pass1"};
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

WaterPass::WaterPass(Application* app, Texture* renderTarget, Texture* refractionTarget) : mApp(app) {
    mWaterTextureBindGroup.addTexture(0,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                      TextureViewDimension::VIEW_2D);
    mWaterTextureBindGroup.addTexture(1,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                      TextureViewDimension::VIEW_2D);

    mWaterTextureBindngData[0] = {};
    mWaterTextureBindngData[0].nextInChain = nullptr;
    mWaterTextureBindngData[0].binding = 0;
    mWaterTextureBindngData[0].textureView = renderTarget->getTextureView();

    mWaterTextureBindngData[1] = {};
    mWaterTextureBindngData[1].nextInChain = nullptr;
    mWaterTextureBindngData[1].binding = 1;
    mWaterTextureBindngData[1].textureView = refractionTarget->getTextureView();

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
                                   .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    setDefault(mRenderPipeline->getDepthStencilState());

    mRenderPipeline->setShader(RESOURCE_DIR "/water.wgsl")
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
