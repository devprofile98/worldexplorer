
#include "terrain_pass.h"

#include "application.h"

WGPURenderPassDescriptor* RenderPass::getRenderPassDescriptor() { return &mRenderPassDesc; }
void RenderPass::setRenderPassDescriptor(WGPURenderPassDescriptor desc) { mRenderPassDesc = desc; }
Pipeline* RenderPass::getPipeline() { return mRenderPipeline; }

TerrainPass::TerrainPass(Application* app) { mApp = app; }

void TerrainPass::createRenderPass(WGPUTextureFormat textureFormat) {
    /*auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");*/
    /*auto texture_bind_group_layout = mTextureBindingGroup.createLayout(mApp, "shadow pass pipeline");*/

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
