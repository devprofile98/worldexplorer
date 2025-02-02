#include "shapes.h"

#include <array>

#include "application.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "model.h"
#include "webgpu.h"

static float cubeVertexData[] = {
    -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,
    0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  -1.0f,
    -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f,
    1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,
    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  -1.0f, 1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,  -1.0f, 1.0f,  1.0f,
    0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  -1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  -1.0f,
    1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f,
    1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  -1.0f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -1.0f, 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  0.0f,  0.0f,
};

static uint16_t cubeIndexData[] = {
    // Back face
    0, 1, 2, 2, 3, 0,  //
    // Front face
    4, 5, 6, 6, 7, 4,  //
    // Left face
    0, 3, 7, 7, 4, 0,  //
    // Right face
    1, 2, 6, 6, 5, 1,  //
    // Bottom face
    0, 1, 5, 5, 4, 0,  //
    // Top face
    3, 2, 6, 6, 7, 3,  //

};

Cube::Cube(Application* app)
    : Transform({glm::vec3{0.0}}, glm::vec3{1.0}, glm::mat4{1.0}, glm::mat4{1.0}, glm::mat4{1.0}, glm::mat4{1.0}) {
    mApp = app;

    mVertexDataBuffer.setLabel("Shape vertex buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setSize(sizeof(cubeVertexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mVertexDataBuffer.getBuffer(), 0, &cubeVertexData,
                         sizeof(cubeVertexData));

    mIndexDataBuffer.setLabel("Shape indices buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index)
        .setSize(sizeof(cubeIndexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mIndexDataBuffer.getBuffer(), 0, &cubeIndexData,
                         sizeof(cubeIndexData));
    Drawable::configure(app);
    /**/
    /*mUniformBuffer.setLabel("Uniform buffer for object for shape")*/
    /*    .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)*/
    /*    .setSize(sizeof(mObjectInfo))*/
    /*    .setMappedAtCraetion()*/
    /*    .create(app);*/
}

/*void createBindGroup() {*/
/*    WGPUBindGroupEntry mBindGroupEntry = {};*/
/*    mBindGroupEntry.nextInChain = nullptr;*/
/*    mBindGroupEntry.binding = 0;*/
/*    mBindGroupEntry.buffer = mUniformBuffer;*/
/*    mBindGroupEntry.offset = 0;*/
/*    mBindGroupEntry.size = sizeof(ObjectInfo);*/
/**/
/*    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};*/
/*    mTrasBindGroupDesc.nextInChain = nullptr;*/
/*    mTrasBindGroupDesc.entries = &mBindGroupEntry;*/
/*    mTrasBindGroupDesc.entryCount = 1;*/
/*    mTrasBindGroupDesc.label = "translation bind group";*/
/*    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];*/
/**/
/*    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);*/
/*}*/

void Cube::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    // draw indexed
    (void)app;
    (void)bindingData;
    auto& render_resource = app->getRendererResource();

    mTransformMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    mObjectInfo.transformation = mTransformMatrix;
    mObjectInfo.isFlat = false;
    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo, sizeof(ObjectInfo));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexDataBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mVertexDataBuffer.getBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mApp->getBindingGroup().getBindGroup(), 0, nullptr);
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = Drawable::getUniformBuffer().getBuffer();
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(ObjectInfo);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);

    wgpuRenderPassEncoderDraw(encoder, sizeof(cubeVertexData) / (11 * sizeof(float)), 1, 0, 0);
    /*wgpurelease*/
}
