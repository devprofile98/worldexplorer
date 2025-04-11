#include "shapes.h"

#include <array>

#include "application.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "model.h"
#include "webgpu.h"

/*static float triangleVertexData[] = {*/
/*    1.0, 0.5, 4.0, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    6.0, 3.0, 4.0, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*    6.1, 2.8, 4.0, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //*/
/*};*/

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
    mMeshes[0]
        .mVertexBuffer.setLabel("Shape vertex buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setSize(sizeof(cubeVertexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mMeshes[0].mVertexBuffer.getBuffer(), 0, &cubeVertexData,
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
        min.y = std::min(min.y, cubeVertexData[idx + 1]);
        min.z = std::min(min.z, cubeVertexData[idx + 2]);
        max.x = std::max(max.x, cubeVertexData[idx]);
        max.y = std::max(max.y, cubeVertexData[idx + 1]);
        max.z = std::max(max.z, cubeVertexData[idx + 2]);
    }
    max.z += 0.2;

    offset_buffer.setSize(sizeof(glm::vec4) * 10)
        .setLabel("offset buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(app);

    std::array<glm::vec4, 10> dddata = {};
    for (size_t i = 0; i < 10; i++) {
        dddata[i] = glm::vec4{0.0, i * 2, 0.0, 0.0};
    }
    wgpuQueueWriteBuffer(app->getRendererResource().queue, offset_buffer.getBuffer(), 0, &dddata,
                         sizeof(glm::vec4) * 10);
}

Plane::Plane(Application* app) : Cube(app) { mScale = {0.0, 1.0, 1.0}; }

size_t Cube::getVertexCount() const { return sizeof(cubeVertexData) / (11 * sizeof(float)); }

#ifdef DEVELOPMENT_BUILD
void Cube::userInterface() {
    ImGui::SliderFloat3("Position", glm::value_ptr(mPosition), 0.0f, 10.0f);
    ImGui::SliderFloat3("Scale", glm::value_ptr(mScale), 0.0f, 10.0f);
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    scale(mScale);
    getTranformMatrix();
}

#endif  // DEVELOPMENT_BUILD

WGPUBindGroupDescriptor createBindGroup(Application* app, WGPUBuffer buffer, size_t bufferSize = sizeof(ObjectInfo)) {
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = buffer;
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = bufferSize;

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

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mMeshes[0].mVertexBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mMeshes[0].mVertexBuffer.getBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));

    /*bindingData[13].buffer = offset_buffer.getBuffer();*/
    auto& desc = app->getBindingGroup().getDescriptor();
    desc.entries = bindingData.data();
    auto bindgroup0 = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup0, 0, nullptr);

    auto bindgroup_desc = createBindGroup(app, Drawable::getUniformBuffer().getBuffer());
    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &bindgroup_desc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);

    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
    wgpuBindGroupRelease(bindgroup_object);
    wgpuBindGroupRelease(bindgroup0);
}

glm::vec3 generateLine(glm::vec3 start, glm::vec3 end, float* vertexData) {
    float slope = (end.y - start.y) / (end.x - start.x);
    if (slope == 0) {
        return glm::vec3{0.0};
    }
    float prep_slope = -1.f / (slope);
    /*float b = end.y - (slope)*end.x;*/
    float other_b = end.y - (prep_slope)*end.x;
    float start_b = start.y - (prep_slope)*start.x;
    float y = prep_slope * (end.x + 0.1) + other_b;
    float start_y = prep_slope * (start.x + 0.1) + start_b;
    /*return glm::vec3{end.x + 0.1, y, end.z};*/
    vertexData[0] = start.x;
    vertexData[1] = start.y;
    vertexData[2] = start.z;

    vertexData[0 + 11] = end.x;
    vertexData[1 + 11] = end.y;
    vertexData[2 + 11] = end.z;

    glm::vec3 other_end = glm::vec3{start.x + 0.1, start_y, start.z};
    glm::vec3 other_start = glm::vec3{end.x + 0.1, y, end.z};
    vertexData[0 + 22] = other_end.x;
    vertexData[1 + 22] = other_end.y;
    vertexData[2 + 22] = other_end.z;

    vertexData[0 + 33] = other_start.x;
    vertexData[1 + 33] = other_start.y;
    vertexData[2 + 33] = other_start.z;

    return other_end;
}

Line::Line(Application* app, glm::vec3 start, glm::vec3 end) : BaseModel() {
    (void)start;
    (void)end;
    mApp = app;
    mName = "Line";

    std::cout << "start: " << glm::to_string(start) << "\n";
    std::cout << "end: " << glm::to_string(end) << "\n";
    glm::vec3 middle = generateLine(start, end, triangleVertexData);
    std::cout << "middle: " << glm::to_string(middle) << "\n";

    mMeshes[0]
        .mVertexBuffer.setLabel("Line vertex buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setSize(sizeof(triangleVertexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mMeshes[0].mVertexBuffer.getBuffer(), 0,
                         &triangleVertexData, sizeof(triangleVertexData));

    mIndexDataBuffer.setLabel("Line indices buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index)
        .setSize(sizeof(mIndexData))
        .setMappedAtCraetion()
        .create(mApp);

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mIndexDataBuffer.getBuffer(), 0, &mIndexData,
                         sizeof(mIndexData));
    Drawable::configure(app);

    for (size_t i = 0; i < (sizeof(triangleVertexData) / (11 * sizeof(float))); i++) {
        size_t idx = i * 11;
        min.x = std::min(min.x, triangleVertexData[idx]);
        min.y = std::min(min.y, triangleVertexData[idx + 1]);
        min.z = std::min(min.z, triangleVertexData[idx + 2]);
        max.x = std::max(max.x, triangleVertexData[idx]);
        max.y = std::max(max.y, triangleVertexData[idx + 1]);
        max.z = std::max(max.z, triangleVertexData[idx + 2]);
    }
}

void Line::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;
    auto& render_resource = app->getRendererResource();

    mObjectInfo.transformation = glm::mat4{1.0f};  // mTransformMatrix;
    mObjectInfo.isFlat = false;
    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,
                         sizeof(ObjectInfo));

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mMeshes[0].mVertexBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mMeshes[0].mVertexBuffer.getBuffer()));

    auto& desc = app->getBindingGroup().getDescriptor();
    desc.entries = bindingData.data();
    auto bindgroup0 = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup0, 0, nullptr);

    auto bindgroup_desc = createBindGroup(app, Drawable::getUniformBuffer().getBuffer(), sizeof(mObjectInfo));
    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &bindgroup_desc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);

    /*wgpuRenderPassEncoderDraw(encoder, sizeof(triangleVertexData) / (11 * sizeof(float)), 1, 0, 0);*/
    wgpuRenderPassEncoderDrawIndexed(encoder, sizeof(mIndexData) / sizeof(uint16_t), 1, 0, 0, 0);

    wgpuBindGroupRelease(bindgroup_object);
    wgpuBindGroupRelease(bindgroup0);
}
