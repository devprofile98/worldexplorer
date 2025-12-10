
#include "terrain_pass.h"

#include <webgpu/webgpu.h>

#include <cstdint>

#include "application.h"
#include "profiling.h"
#include "rendererResource.h"
#include "renderpass.h"
#include "texture.h"
#include "utils.h"

// ifdef WEBGPU_BACKEND

TerrainPass::TerrainPass(Application* app, const std::string& name) : RenderPass(name) {
    mApp = app;

    mTexturesBindgroup
        .addTexture(0, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(1, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(3, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D);

    mTextureBindgroupLayout = mTexturesBindgroup.createLayout(app, "Terrain Texture layout bidngroup");

    mGrass =
        new Texture{mApp->getRendererResource().device,
                    std::vector<std::filesystem::path>{RESOURCE_DIR "/mud/diffuse.jpg", RESOURCE_DIR "/mud/normal.jpg",
                                                       RESOURCE_DIR "/mud/roughness.jpg"},
                    WGPUTextureFormat_RGBA8Unorm, 3};
    mGrass->createViewArray(0, 3);
    mGrass->uploadToGPU(mApp->getRendererResource().queue);

    mRock = new Texture{mApp->getRendererResource().device,
                        std::vector<std::filesystem::path>{RESOURCE_DIR "/Rock/Rock060_1K-JPG_Color.jpg",
                                                           RESOURCE_DIR "/Rock/Rock060_1K-JPG_NormalGL.jpg",
                                                           RESOURCE_DIR "/Rock/Rock060_1K-JPG_Roughness.jpg"},
                        WGPUTextureFormat_RGBA8Unorm, 3};
    mRock->createViewArray(0, 3);
    mRock->uploadToGPU(mApp->getRendererResource().queue);

    mSand = new Texture{mApp->getRendererResource().device,
                        std::vector<std::filesystem::path>{RESOURCE_DIR "/aerial/aerial_beach_01_diff_1k.jpg",
                                                           RESOURCE_DIR "/aerial/aerial_beach_01_nor_gl_1k.jpg",
                                                           RESOURCE_DIR "/aerial/aerial_beach_01_rough_1k.jpg"},
                        WGPUTextureFormat_RGBA8Unorm, 3};
    mSand->createViewArray(0, 3);
    mSand->uploadToGPU(mApp->getRendererResource().queue);

    mSnow = new Texture{mApp->getRendererResource().device,
                        std::vector<std::filesystem::path>{RESOURCE_DIR "/snow/snow_02_diff_1k.jpg",
                                                           RESOURCE_DIR "/snow/snow_02_nor_gl_1k.jpg",
                                                           RESOURCE_DIR "/snow/snow_02_rough_1k.jpg"},
                        WGPUTextureFormat_RGBA8Unorm, 3};
    mSnow->createViewArray(0, 3);
    mSnow->uploadToGPU(mApp->getRendererResource().queue);
    mTempTextures = {mGrass, mRock, mSand, mSnow};
}

void TerrainPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline =
        new Pipeline{mApp,
                     {layouts[0], layouts[1], layouts[2], layouts[3], layouts[4], layouts[5], mTextureBindgroupLayout},
                     "Terrain pipeline"};

    mBindingData[0] = {};
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].textureView = mTempTextures[0]->getTextureViewArray();

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = mTempTextures[1]->getTextureViewArray();

    mBindingData[2] = {};
    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].textureView = mTempTextures[2]->getTextureViewArray();

    mBindingData[3] = {};
    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].textureView = mTempTextures[3]->getTextureViewArray();

    mTexturesBindgroup.create(mApp, mBindingData);

    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/terrain.wgsl");
    setDefaultActiveStencil2(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setPrimitiveState(WGPUFrontFace_CCW, WGPUCullMode_Back);
    mRenderPipeline->createPipeline(mApp);

    terrain.generate(200, 8, terrainData).uploadToGpu(mApp);
    std::cout << "Generate is " << terrainData.size() << '\n';
    terrain.createSomeBinding(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    // mTempTextures = textures;
    createRenderPass(textureFormat);
    return mRenderPipeline;
}

void TerrainPass::executePass() {
    ZoneScopedNC("Last Terrain pass", 0xFF00FF);
    terrain.update(mApp, 0.0);

    setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    init();

    WGPURenderPassEncoder terrain_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(mApp->getRendererResource().commandEncoder, getRenderPassDescriptor());
    wgpuRenderPassEncoderSetPipeline(terrain_pass_encoder, getPipeline()->getPipeline());

    wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 3, mApp->mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                      nullptr);

    wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 4, mApp->mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 5, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 6, mTexturesBindgroup.getBindGroup(), 0, nullptr);
    terrain.draw(mApp, terrain_pass_encoder, mBindingData);

    wgpuRenderPassEncoderEnd(terrain_pass_encoder);
    wgpuRenderPassEncoderRelease(terrain_pass_encoder);
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

        model->drawHirarchy(mApp, pass_encoder);
    }

    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);
}
