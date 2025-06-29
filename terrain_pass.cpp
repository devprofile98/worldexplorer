
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
    setDefaultActiveStencil(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}



OutlinePass::OutlinePass(Application* app) { mApp = app; }

void OutlinePass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp, {layouts[0], layouts[1], layouts[2]}, "Outline Pass"};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/outline.wgsl");
    setDefaultUseStencil(mRenderPipeline->getDepthStencilState());
    /*setDefaultActiveStencil(mRenderPipeline->getDepthStencilState());*/
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* OutlinePass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}
