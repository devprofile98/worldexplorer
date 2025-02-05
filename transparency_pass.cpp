#include "transparency_pass.h"

#include <cstdint>

#include "application.h"
#include "webgpu.h"
/**/
/*@binding(0) @group(0) var<uniform> uniforms: Uniforms;*/
/*@binding(1) @group(0) var<storage, read_write> heads: Heads;*/
/*@binding(2) @group(0) var<storage, read_write> linkedList: LinkedList;*/
/*@binding(3) @group(0) var opaqueDepthTexture: texture_depth_2d;*/
/*@binding(4) @group(0) var<uniform> sliceInfo: SliceInfo;*/

TransparencyPass::TransparencyPass(Application* app) : mApp(app) {
    // creating bindings for the pass
    // 1 - uniform data
    // 2 - a storage buffer for Heads
    // 3 - a storage buffer for linkedlist
    // 4 - the depth texture from the opaque pass

    mUniformBuffer.setLabel("Transparency Uniform buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(MyUniform))
        .setMappedAtCraetion()
        .create(app);

    // head struct consists of: 1- number of added elems, 2- array<head index, size of screen in pixel>
    size_t head_size = (1920 * 1080) * sizeof(uint32_t) + sizeof(uint32_t);
    mHeadsBuffer.setLabel("Heads buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setSize(head_size)
        .setMappedAtCraetion()
        .create(app);

    // the struct consists of : 1 - color:vec4, 2- depth (f32), 3- next: u32
    size_t linkedlist_size = (1920 * 1080) * (sizeof(glm::vec4) + sizeof(float) + sizeof(uint32_t));
    mLinkedlistBuffer.setLabel("Linkedlist buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setSize(sizeof(linkedlist_size))
        .setMappedAtCraetion()
        .create(app);

    // create the render pass for transparency
    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.label = "transparency render pass descriptor";
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = nullptr;

    // transparency pass depth stencil attachment
    static WGPURenderPassDepthStencilAttachment transparency_pass_dsa;
    transparency_pass_dsa.view = nullptr;
    transparency_pass_dsa.depthClearValue = 1.0f;
    transparency_pass_dsa.depthLoadOp = WGPULoadOp_Clear;
    transparency_pass_dsa.depthStoreOp = WGPUStoreOp_Store;
    transparency_pass_dsa.depthReadOnly = true;

    transparency_pass_dsa.stencilClearValue = 0.0f;
    transparency_pass_dsa.stencilLoadOp = WGPULoadOp_Clear;
    transparency_pass_dsa.stencilStoreOp = WGPUStoreOp_Store;
    transparency_pass_dsa.stencilReadOnly = true;

    mRenderPassDesc.depthStencilAttachment = &transparency_pass_dsa;
    mRenderPassDesc.timestampWrites = nullptr;
}

void TransparencyPass::initializePass() {}

// implement a draw function that calculate the A-Buffer for the scene
