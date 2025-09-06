
#include "water_pass.h"

#include <webgpu/webgpu.h>

#include "application.h"
#include "renderpass.h"

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
}

WaterPass::WaterPass(Application* app, Texture* renderTarget, Texture* refractionTarget, const std::string& name)
    : RenderPass(name), mApp(app) {
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
