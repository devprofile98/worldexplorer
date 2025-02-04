#include "shapes.h"

#include <array>

#include "application.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "model.h"
#include "webgpu.h"

static float cubeVertexData[] = {
    -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    -1.0f, 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    1.0f,  1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    -1.0f, 1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  //
    1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f,
    0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  -1.0f, 1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  -1.0f, -1.0f, -1.0f, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,
    1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  -1.0f, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,
    1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,
    0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  -1.0f, 1.0f,
    0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,  -1.0f,
    1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  -1.0f, 1.0f,
    1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
    0.0f,  0.0f,  0.0f,  0.0f,
};

/*static float planeVertexData[] = {*/
/*    -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    -1.0f, -1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    -1.0f, 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    1.0f,  1.0f,  -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,*/
/*    -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    -1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    1.0f,  -1.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/**/
/*};*/

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

Cube::Cube(Application* app) : BaseModel() {
    mApp = app;
    mName = "Cube";
    mVertexBuffer.setLabel("Shape vertex buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setSize(sizeof(cubeVertexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mVertexBuffer.getBuffer(), 0, &cubeVertexData,
                         sizeof(cubeVertexData));

    mIndexDataBuffer.setLabel("Shape indices buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index)
        .setSize(sizeof(cubeIndexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mIndexDataBuffer.getBuffer(), 0, &cubeIndexData,
                         sizeof(cubeIndexData));
    Drawable::configure(app);
    for (size_t i = 0; i < getVertexCount(); i++) {
        size_t idx = i * 11;
        min.x = std::min(min.x, cubeVertexData[idx]);
        min.y = std::min(min.y, cubeVertexData[idx+1]);
        min.z = std::min(min.z, cubeVertexData[idx+2]);
        max.x = std::max(max.x, cubeVertexData[idx]);
        max.y = std::max(max.y, cubeVertexData[idx+1]);
        max.z = std::max(max.z, cubeVertexData[idx+2]);
    }
}

Plane::Plane(Application* app) : Cube(app) { mScale = {0.0, 1.0, 1.0}; }

/*Plane::Plane(Application* app) : BaseModel() {*/
/*    mApp = app;*/
/*    mName = "Cube";*/
/*    mVertexBuffer.setLabel("Shape vertex buffer")*/
/*        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)*/
/*        .setSize(sizeof(planeVertexData))*/
/*        .setMappedAtCraetion()*/
/*        .create(mApp);*/
/**/
/*    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mVertexBuffer.getBuffer(), 0, &planeVertexData,*/
/*                         sizeof(planeVertexData));*/
/**/
/*    mIndexDataBuffer.setLabel("Shape indices buffer")*/
/*        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index)*/
/*        .setSize(sizeof(cubeIndexData))*/
/*        .setMappedAtCraetion()*/
/*        .create(mApp);*/
/**/
/*    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mIndexDataBuffer.getBuffer(), 0, &cubeIndexData,*/
/*                         sizeof(cubeIndexData));*/
/*    Drawable::configure(app);*/
/*}*/

size_t Cube::getVertexCount() const { return sizeof(cubeVertexData) / (11 * sizeof(float)); }
/*size_t Plane::getVertexCount() const { return sizeof(planeVertexData) / (11 * sizeof(float)); }*/

#ifdef DEVELOPMENT_BUILD
void Cube::userInterface() {
    ImGui::SliderFloat3("Position", glm::value_ptr(mPosition), 0.0f, 10.0f);
    ImGui::SliderFloat3("Scale", glm::value_ptr(mScale), 0.0f, 10.0f);
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    scale(mScale);
    getTranformMatrix();
}

/*void Plane::userInterface() {*/
/*    ImGui::SliderFloat3("Position", glm::value_ptr(mPosition), 0.0f, 10.0f);*/
/*    ImGui::SliderFloat3("Scale", glm::value_ptr(mScale), 0.0f, 10.0f);*/
/*    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);*/
/*    scale(mScale);*/
/*    getTranformMatrix();*/
/*}*/
#endif  // DEVELOPMENT_BUILD

WGPUBindGroupDescriptor createBindGroup(Application* app, WGPUBuffer buffer) {
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = buffer;
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(ObjectInfo);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    return mTrasBindGroupDesc;
}

void Cube::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;
    auto& render_resource = app->getRendererResource();

    mObjectInfo.transformation = mTransformMatrix;
    mObjectInfo.isFlat = false;
    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,
                         sizeof(ObjectInfo));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mVertexBuffer.getBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mApp->getBindingGroup().getBindGroup(), 0, nullptr);
    auto bindgroup_desc = createBindGroup(app, Drawable::getUniformBuffer().getBuffer());
    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &bindgroup_desc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);

    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
    wgpuBindGroupRelease(bindgroup_object);
}

/*void Plane::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {*/
/*    (void)bindingData;*/
/*    auto& render_resource = app->getRendererResource();*/
/**/
/*    mObjectInfo.transformation = mTransformMatrix;*/
/*    mObjectInfo.isFlat = false;*/
/*    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,*/
/*                         sizeof(ObjectInfo));*/
/**/
/*    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0,*/
/*                                         wgpuBufferGetSize(mVertexBuffer.getBuffer()));*/
/*    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,*/
/*                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));*/
/*    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mApp->getBindingGroup().getBindGroup(), 0, nullptr);*/
/*    auto bindgroup_desc = createBindGroup(app, Drawable::getUniformBuffer().getBuffer());*/
/*    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &bindgroup_desc);*/
/**/
/*    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);*/
/**/
/*    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);*/
/*    wgpuBindGroupRelease(bindgroup_object);*/
/*}*/
