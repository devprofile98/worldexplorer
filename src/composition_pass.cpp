#include "composition_pass.h"

#include <webgpu/webgpu.h>

#include <cstdint>

#include "application.h"
#include "wgpu_utils.h"

CompositionPass::CompositionPass(Application* app) : mApp(app) {
    // 1 - create buffers
    /*mUniformBuffer.setLabel("Transparency Uniform buffer")*/
    /*    .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)*/
    /*    .setSize(sizeof(MyUniform))*/
    /*    .setMappedAtCraetion()*/
    /*    .create(app);*/
    /**/
    // head struct consists of: 1- number of added elems, 2- array<head index, size of screen in pixel>
    /*size_t head_size = (1920 * 1080) * sizeof(uint32_t) + sizeof(uint32_t);*/
    /*mHeadsBuffer.setLabel("Heads buffer")*/
    /*    .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)*/
    /*    .setSize(head_size)*/
    /*    .setMappedAtCraetion()*/
    /*    .create(app);*/
    /**/
    /*// the struct consists of : 1 - color:vec4, 2- depth (f32), 3- next: u32*/
    /*size_t linkedlist_size = (1920 * 1080 * 2) * (sizeof(glm::vec4) + sizeof(float) + sizeof(uint32_t));*/
    /*mLinkedlistBuffer.setLabel("Linkedlist buffer")*/
    /*    .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)*/
    /*    .setSize(linkedlist_size)*/
    /*    .setMappedAtCraetion()*/
    /*    .create(app);*/
}

void CompositionPass::initializePass() {
    // create the render pass for transparency
    mRenderPassColorAttachment = {};
    mRenderPassColorAttachment.view = nullptr;
    mRenderPassColorAttachment.resolveTarget = nullptr;
    mRenderPassColorAttachment.loadOp = WGPULoadOp_Load;
    mRenderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    mRenderPassColorAttachment.clearValue = WGPUColor{0.02, 0.80, 0.92, 1.0};

    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.label = createStringView("composition pass descriptor");
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = nullptr;

    // transparency pass depth stencil attachment
    mRenderPassDepthStencilAttachment.view = nullptr;  // we should set this in Render function, before each draw call
    mRenderPassDepthStencilAttachment.depthClearValue = 1.0f;
    mRenderPassDepthStencilAttachment.depthLoadOp = WGPULoadOp_Load;
    mRenderPassDepthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    mRenderPassDepthStencilAttachment.depthReadOnly = true;

    mRenderPassDepthStencilAttachment.stencilClearValue = 0.0f;
    mRenderPassDepthStencilAttachment.stencilLoadOp = WGPULoadOp_Load;
    mRenderPassDepthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
    mRenderPassDepthStencilAttachment.stencilReadOnly = true;

    mRenderPassDesc.depthStencilAttachment = &mRenderPassDepthStencilAttachment;
    mRenderPassDesc.colorAttachments = &mRenderPassColorAttachment;
    mRenderPassDesc.timestampWrites = nullptr;

    // Creating binding group layout
    size_t linkedlist_size = (1920 * 1080 * 4) * (sizeof(uint32_t) + sizeof(float) + sizeof(uint32_t));
    size_t head_size = (1920 * 1080) * sizeof(uint32_t) + sizeof(uint32_t);

    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::STORAGE, head_size);
    mBindingGroup.addBuffer(1, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::STORAGE, linkedlist_size);

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "composition pass pipeline");
    mRenderPipeline = new Pipeline{mApp, {bind_group_layout}, "Composition layout"};

    /*WGPUVertexBufferLayout d = mRenderPipeline->getDefaultVertexBufferLayout();*/
    WGPUVertexBufferLayout d = {};
    WGPUBlendState blend_state = {};
    blend_state.color.srcFactor = WGPUBlendFactor_One;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.color.operation = WGPUBlendOperation_Add;
    blend_state.alpha = {};

    /*WGPUColorTargetState color_state;*/
    mRenderPipeline->setShader(RESOURCE_DIR "/composition.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState(0, "vs_main", {})
        .setPrimitiveState(WGPUFrontFace_CCW, WGPUCullMode_None)
        .setDepthStencilState()
        .setBlendState(blend_state)
        .setColorTargetState()
        .setFragmentState();

    mRenderPipeline->setMultiSampleState().createPipeline(mApp);

    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].size = head_size;
    mBindingData[0].buffer = mHeadsBuffer;

    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].size = linkedlist_size;
    mBindingData[1].buffer = mLinkedlistBuffer;
}

// Getters
WGPURenderPassDescriptor* CompositionPass::getRenderPassDescriptor() { return &mRenderPassDesc; }
Pipeline* CompositionPass::getPipeline() { return mRenderPipeline; }
/*WGPURenderPassDepthStencilAttachment CompositionPass::mRenderPassDepthStencilAttachment{}*/

void CompositionPass::setSSBOBuffers(WGPUBuffer headBuffer, WGPUBuffer linkedlistBuffer) {
    mLinkedlistBuffer = linkedlistBuffer;
    mHeadsBuffer = headBuffer;
}

void CompositionPass::render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder,
                             WGPURenderPassColorAttachment* colorAttachment) {
    (void)models;
    (void)encoder;
    (void)colorAttachment;
    /*mRenderPassDesc.colorAttachments = colorAttachment;*/

    mBindingData[0].buffer = mHeadsBuffer;
    mBindingData[1].buffer = mLinkedlistBuffer;

    auto bindgroup = mBindingGroup.createNew(mApp, mBindingData);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);
    wgpuRenderPassEncoderDraw(encoder, 6, 1, 0, 0);
    wgpuBindGroupRelease(bindgroup);
}
