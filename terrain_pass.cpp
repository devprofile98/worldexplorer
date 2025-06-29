
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
    setDefaultActiveStencil(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->setDepthStencilState(mRenderPipeline->getDepthStencilState());
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}
