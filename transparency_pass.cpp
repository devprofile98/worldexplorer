#include "transparency_pass.h"

#include <cstdint>

#include "application.h"
#include "binding_group.h"
#include "glm/fwd.hpp"
#include "model.h"
#include "webgpu.h"

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
    size_t linkedlist_size = (1920 * 1080 * 2) * sizeof(LinkedListElement);
    mLinkedlistBuffer.setLabel("Linkedlist buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setSize(linkedlist_size)
        .setMappedAtCraetion()
        .create(app);
}

void TransparencyPass::initializePass() {
    // create the render pass for transparency
    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.label = "transparency render pass descriptor";
    mRenderPassDesc.colorAttachmentCount = 0;
    mRenderPassDesc.colorAttachments = nullptr;

    // transparency pass depth stencil attachment
    /*static WGPURenderPassDepthStencilAttachment transparency_pass_dsa;*/
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
    mRenderPassDesc.timestampWrites = nullptr;

    // Creating binding group layout
    //

    constexpr size_t head_size = (1920 * 1080) * sizeof(uint32_t) + sizeof(uint32_t);
    size_t linkedlist_size =
        (1920 * 1080 * 2) * sizeof(LinkedListElement);  // (sizeof(glm::vec4) + sizeof(float) + sizeof(uint32_t));
    /*sizeof(LinkedListElement);*/
    std::cout << "the linkedlist size is" << linkedlist_size << '\n';
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(MyUniform));
    mBindingGroup.addBuffer(1, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::STORAGE, head_size);
    mBindingGroup.addBuffer(2, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::STORAGE, linkedlist_size);
    mBindingGroup.addTexture(3, BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::DEPTH,
                             TextureViewDimension::VIEW_2D);

    mBindingGroup.addBuffer(4, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(ObjectInfo));

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");

    mRenderPipeline = new Pipeline{mApp, {bind_group_layout}};

    WGPUVertexBufferLayout d = mRenderPipeline->getDefaultVertexBufferLayout();

    WGPUColorTargetState color_state;
    mRenderPipeline->setShader(RESOURCE_DIR "/oit.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState()
        .setPrimitiveState(WGPUFrontFace_CCW, WGPUCullMode_None)
        .setDepthStencilState()
        .setBlendState()
        .setFragmentState()
        .setColorTargetState(color_state);

    mRenderPipeline->setMultiSampleState().createPipeline(mApp);
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].size = sizeof(MyUniform);
    mBindingData[0].buffer = mUniformBuffer.getBuffer();

    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].size = head_size;
    mBindingData[1].buffer = mHeadsBuffer.getBuffer();

    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].size = linkedlist_size;
    mBindingData[2].buffer = mLinkedlistBuffer.getBuffer();

    mBindingData[3] = {};
    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].textureView = nullptr;  // we should set it in draw function

    mBindingData[4].nextInChain = nullptr;
    mBindingData[4].binding = 4;
    mBindingData[4].size = sizeof(ObjectInfo);
    mBindingData[4].buffer = nullptr;

    // Prepare CPU-side data to reset the buffers
    headsData = std::vector<uint32_t>(1920 * 1080, 0xFFFFFFFF);  // Set all to 0xFFFFFFFF
	std::cout <<"TTTTTTTTTTTTTTTTTTTTTTTTTT the value is " << headsData.size() << '\n';

    linkedListData = std::vector<LinkedListElement>(1920 * 1080 * 2);  // Default-initialize
}

std::pair<WGPUBuffer, WGPUBuffer> TransparencyPass::getSSBOBuffers() {
    return {mHeadsBuffer.getBuffer(), mLinkedlistBuffer.getBuffer()};
}

void TransparencyPass::render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder,
                              WGPUTextureView opaqueDepthTextureView) {
    auto& render_resource = mApp->getRendererResource();

    // Write reset data to heads buffer
    uint32_t num = 0;
    wgpuQueueWriteBuffer(render_resource.queue, mHeadsBuffer.getBuffer(), 0, &num, sizeof(uint32_t));  // Reset numFragments
    wgpuQueueWriteBuffer(render_resource.queue, mHeadsBuffer.getBuffer(), sizeof(uint32_t), headsData.data(),
                         headsData.size() * sizeof(uint32_t));  // Reset heads.data

    // Write reset data to linked list buffer
    wgpuQueueWriteBuffer(render_resource.queue, mLinkedlistBuffer.getBuffer(), 0, linkedListData.data(),
                         linkedListData.size() * sizeof(LinkedListElement));

    for (auto* model : models) {
        if (!model->isTransparent()) {
            continue;
        }
        Buffer object_info_buffer;
        object_info_buffer.setSize(sizeof(ObjectInfo))
            .setLabel("object info for oit")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
            .setMappedAtCraetion()
            .create(mApp);

        mBindingData[3].textureView = opaqueDepthTextureView;
        mBindingData[4].buffer = object_info_buffer.getBuffer();

        wgpuQueueWriteBuffer(mApp->getRendererResource().queue, object_info_buffer.getBuffer(), 0,
                             &model->getTranformMatrix(), sizeof(glm::mat4));

        wgpuQueueWriteBuffer(render_resource.queue, mUniformBuffer.getBuffer(), 0, &mApp->getUniformData(),
                             sizeof(MyUniform));

        auto bindgroup = mBindingGroup.createNew(mApp, mBindingData);

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, model->getVertexBuffer().getBuffer(), 0,
                                             wgpuBufferGetSize(model->getVertexBuffer().getBuffer()));

        wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);

        wgpuRenderPassEncoderDraw(encoder, model->getVertexCount(), 1, 0, 0);

        wgpuBufferRelease(object_info_buffer.getBuffer());
        wgpuBindGroupRelease(bindgroup);
    }
}

WGPURenderPassDescriptor* TransparencyPass::getRenderPassDescriptor() { return &mRenderPassDesc; }

Pipeline* TransparencyPass::getPipeline() { return mRenderPipeline; }

// implement a draw function that calculate the A-Buffer for the scene
