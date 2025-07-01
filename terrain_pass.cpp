
#include "terrain_pass.h"

#include "application.h"
#include "utils.h"
#include "webgpu.h"

// ifdef WEBGPU_BACKEND

TerrainPass::TerrainPass(Application* app) { mApp = app; }

void TerrainPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp, {layouts[0], layouts[1], layouts[2]}, "Terrain pipeline"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/terrain.wgsl");
    /*setDefaultUseStencil(mRenderPipeline->getDepthStencilState());*/
    setDefaultActiveStencil2(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}

OutlinePass::OutlinePass(Application* app) {
    mApp = app;

    mDepthTextureBindgroup.addTexture(0,  //
                                      BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH,
                                      TextureViewDimension::VIEW_2D);

    mLayerThree = mDepthTextureBindgroup.createLayout(app, "layer three bidngroup");
}

void OutlinePass::createRenderPass(WGPUTextureFormat textureFormat) {
    mDepthTextureBindgroup.create(mApp, mOutlineSpecificBindingData);
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp, {layouts[0], layouts[1], layouts[2], mLayerThree}, "Outline Pass"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/outline.wgsl");
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
