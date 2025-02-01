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
}

void Cube::draw(Application* app, WGPURenderPassEncoder encoder) {
    // draw indexed
    (void)app;
    auto& render_resource = app->getRendererResource();

    mTransformMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    wgpuQueueWriteBuffer(render_resource.queue, app->getUniformBuffer(), offsetof(MyUniform, modelMatrix),
                         glm::value_ptr(mTransformMatrix), sizeof(glm::mat4));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexDataBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mVertexDataBuffer.getBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mApp->getBindingGroup().getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderDraw(encoder, sizeof(cubeVertexData) / (11 * sizeof(float)), 1, 0, 0);
}
