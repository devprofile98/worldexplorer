
#include "terrain_pass.h"

#include "application.h"
#include "webgpu.h"

// ifdef WEBGPU_BACKEND

TerrainPass::TerrainPass(Application* app) { mApp = app; }

void TerrainPass::createRenderPass(WGPUTextureFormat textureFormat) {
    auto* layouts = mApp->getBindGroupLayouts();
    mRenderPipeline = new Pipeline{mApp, {layouts[0], layouts[1], layouts[2]}};
    mRenderPipeline->defaultConfiguration(mApp, textureFormat);
    mRenderPipeline->setShader(RESOURCE_DIR "/terrain.wgsl");
    mRenderPipeline->createPipeline(mApp);
}

Pipeline* TerrainPass::create(WGPUTextureFormat textureFormat) {
    createRenderPass(textureFormat);
    return mRenderPipeline;
}
