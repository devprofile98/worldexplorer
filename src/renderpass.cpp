
#include "renderpass.h"

#include "wgpu_utils.h"

WGPULoadOp from(LoadOp op) { return static_cast<WGPULoadOp>(op); }
WGPUStoreOp from(StoreOp op) { return static_cast<WGPUStoreOp>(op); }

ColorAttachment::ColorAttachment(WGPUTextureView target, WGPUTextureView resolve, WGPUColor clearColor, StoreOp storeOp,
                                 LoadOp loadOp) {
    mAttachment = {};
    mAttachment.nextInChain = nullptr;
    mAttachment.view = target;
    mAttachment.resolveTarget = resolve;
    mAttachment.loadOp = from(loadOp);
    mAttachment.storeOp = from(storeOp);
    mAttachment.clearValue = clearColor;
#ifndef WEBGPU_BACKEND_WGPU
    mAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU
}

WGPURenderPassColorAttachment* ColorAttachment::get() { return &mAttachment; }

DepthStencilAttachment::DepthStencilAttachment(WGPUTextureView target, StoreOp depthStoreOp, LoadOp depthLoadOp,
                                               bool depthReadOnly, StoreOp stencilStoreOp, LoadOp stencilLoadOp,
                                               bool stencilReadOnly, float c) {
    mAttachment = {};
    mAttachment.view = target;
    mAttachment.depthClearValue = c;
    mAttachment.depthLoadOp = from(depthLoadOp);
    mAttachment.depthStoreOp = from(depthStoreOp);
    mAttachment.depthReadOnly = depthReadOnly;
    mAttachment.stencilClearValue = 0;
    mAttachment.stencilLoadOp = from(stencilLoadOp);
    mAttachment.stencilStoreOp = from(stencilStoreOp);
    mAttachment.stencilReadOnly = stencilReadOnly;
}

WGPURenderPassDepthStencilAttachment* DepthStencilAttachment::get() { return &mAttachment; }

RenderPass::RenderPass(const std::string& name) : mName(name) {}

WGPURenderPassDescriptor* RenderPass::getRenderPassDescriptor() { return &mRenderPassDesc; }

void RenderPass::setRenderPassDescriptor(WGPURenderPassDescriptor desc) { mRenderPassDesc = desc; }

Pipeline* RenderPass::getPipeline() { return mRenderPipeline; }

RenderPass& RenderPass::setColorAttachment(const ColorAttachment& attachment) {
    mColorAttachment = attachment;
    return *this;
}

RenderPass& RenderPass::setDepthStencilAttachment(const DepthStencilAttachment& attachment) {
    mDepthStencilAttachment = attachment;
    return *this;
}

WGPURenderPassDescriptor* RenderPass::init() {
    // std::cout << "if you go away !" << mName.c_str() << std::endl;
    mRenderPassDesc.label = createStringViewC(mName.c_str());
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = mColorAttachment.get();

    mRenderPassDesc.depthStencilAttachment = mDepthStencilAttachment.get();
    mRenderPassDesc.timestampWrites = nullptr;
    mRenderPassDesc.occlusionQuerySet = nullptr;
    return &mRenderPassDesc;
}
