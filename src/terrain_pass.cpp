
#include "terrain_pass.h"

#include <webgpu/webgpu.h>

#include <cstdint>

#include "application.h"
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
}

Pipeline* ViewPort3DPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}
