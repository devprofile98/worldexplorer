#include "shapes.h"

#include <array>
#include <cstdint>
#include <glm/fwd.hpp>
#include <limits>
#include <vector>

#include "application.h"
#include "binding_group.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "mesh.h"
#include "pipeline.h"
#include "utils.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "model.h"
#include "rendererResource.h"
#include "renderpass.h"
#include "wgpu_utils.h"

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

Plane::Plane(Application* app) : Cube(app) { mTransform.mScale = {0.0, 1.0, 1.0}; }

size_t Cube::getVertexCount() const { return sizeof(cubeVertexData) / (11 * sizeof(float)); }

#ifdef DEVELOPMENT_BUILD
void Cube::userInterface() {
    ImGui::SliderFloat3("Position", glm::value_ptr(mTransform.mPosition), 0.0f, 10.0f);
    ImGui::SliderFloat3("Scale", glm::value_ptr(mTransform.mScale), 0.0f, 10.0f);
    mTransform.mTranslationMatrix = glm::translate(glm::mat4{1.0}, mTransform.mPosition);
    scale(mTransform.mScale);
    mTransform.getLocalTransform();
}

#endif  // DEVELOPMENT_BUILD

WGPUBindGroupDescriptor createBindGroup(Application* app, WGPUBuffer buffer, size_t bufferSize = sizeof(ObjectInfo)) {
    // static WGPUBindGroupEntry mBindGroupEntry = {};
    // mBindGroupEntry.nextInChain = nullptr;
    // mBindGroupEntry.binding = 0;
    // mBindGroupEntry.buffer = buffer;
    // mBindGroupEntry.offset = 0;
    // mBindGroupEntry.size = bufferSize;
    //
    // WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    // mTrasBindGroupDesc.nextInChain = nullptr;
    // mTrasBindGroupDesc.entries = &mBindGroupEntry;
    // mTrasBindGroupDesc.entryCount = 1;
    // mTrasBindGroupDesc.label = {"translation bind group", WGPU_STRLEN};
    // mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];
    //
    // return mTrasBindGroupDesc;

    static std::array<WGPUBindGroupEntry, 2> mBindGroupEntry = {};
    mBindGroupEntry[0].nextInChain = nullptr;
    mBindGroupEntry[0].binding = 0;
    mBindGroupEntry[0].buffer = buffer;
    mBindGroupEntry[0].offset = 0;
    mBindGroupEntry[0].size = bufferSize;

    mBindGroupEntry[1].nextInChain = nullptr;
    mBindGroupEntry[1].buffer = app->mDefaultBoneFinalTransformData.getBuffer();
    mBindGroupEntry[1].binding = 1;
    mBindGroupEntry[1].offset = 0;
    mBindGroupEntry[1].size = 100 * sizeof(glm::mat4);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = mBindGroupEntry.data();
    mTrasBindGroupDesc.entryCount = 2;
    mTrasBindGroupDesc.label = createStringView("translation bind group");
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    // ggg = ;
    return mTrasBindGroupDesc;
    // return wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);
}

void Cube::draw(Application* app, WGPURenderPassEncoder encoder) {
    auto& render_resource = app->getRendererResource();

    mTransform.mObjectInfo.transformation = mTransform.mTransformMatrix;
    // mTransform.mObjectInfo.isFlat = false;
    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mTransform.mObjectInfo,
                         sizeof(ObjectInfo));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mMeshes[0].mVertexBuffer.getBuffer(), 0,
                                         wgpuBufferGetSize(mMeshes[0].mVertexBuffer.getBuffer()));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexDataBuffer.getBuffer(), WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexDataBuffer.getBuffer()));

    /*bindingData[13].buffer = offset_buffer.getBuffer();*/
    auto& desc = app->getBindingGroup().getDescriptor();
    desc.entries = app->mBindingData.data();
    auto bindgroup0 = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup0, 0, nullptr);

    auto bindgroup_desc = createBindGroup(app, Drawable::getUniformBuffer().getBuffer());
    WGPUBindGroup bindgroup_object = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &bindgroup_desc);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, bindgroup_object, 0, nullptr);

    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
    wgpuBindGroupRelease(bindgroup_object);
    wgpuBindGroupRelease(bindgroup0);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
///
///
std::vector<uint16_t> generateCircleIndexBuffer() {
    std::vector<uint16_t> indices{};

    const uint8_t num_vertices = 64;
    for (uint8_t i = 0; i < num_vertices; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }

    return indices;
}

std::vector<glm::vec3> generateCircleVertices() {
    std::vector<glm::vec3> points{};
    points.push_back(glm::vec3{0.0f});
    const uint8_t num_vertices = 64;
    const float radius = 0.25f;
    float steps = 4 * glm::pi<float>() / num_vertices;
    for (uint8_t i = 0; i < num_vertices; ++i) {
        points.push_back({radius * glm::cos(i * steps), radius * glm::sin(i * steps), 0.0f});
    }
    return points;
}

void LineEngine::initCirclePipeline() {
    mCircleBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
        .configure(sizeof(glm::vec3), VertexStepMode::VERTEX);
}

void LineEngine::initialize(Application* app) {
    // create render pass
    // create bindgroups
    // create pipeline

    mApp = app;

    mLineRenderingPass = new NewRenderPass{"Line Rendering render pass"};
    mLineRenderingPass->setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mLineRenderingPass->setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    mLineRenderingPass->init();

    //// create attribute vector
    mBindGroup
        .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::STORAGE_READONLY,
                   mMaxPoints * sizeof(LineSegment))
        .addBuffer(1, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::STORAGE_READONLY,
                   mMaxPoints * sizeof(glm::mat4));
    mCameraBindGroup.addBuffer(0,  //
                               BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                               sizeof(CameraInfo));
    initCirclePipeline();

    auto layout = mBindGroup.createLayout(app, "line rendering bindgroup");
    auto camera_layout = mCameraBindGroup.createLayout(app, "camera for line rendering bindgroup");

    mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x2)
        .configure(sizeof(glm::vec2), VertexStepMode::VERTEX);

    mPipeline = new Pipeline{app, {layout, camera_layout}, "Line Drawing pipeline"};

    mCirclePipeline = new Pipeline{app, {layout, camera_layout}, "Line joint Circle Drawing pipeline"};

    mPipeline->setVertexBufferLayout(mVertexBufferLayout.getLayout())
        .setShader(RESOURCE_DIR "/shaders/line_pass.wgsl")
        .setVertexState()
        .setBlendState()
        .setPrimitiveState()
        .setColorTargetState(WGPUTextureFormat_BGRA8UnormSrgb)
        .setDepthStencilState(true, 0xFF, 0xFF, WGPUTextureFormat_Depth24PlusStencil8)
        .setFragmentState()
        .createPipeline(app);

    mCirclePipeline->setVertexBufferLayout(mCircleBufferLayout.getLayout())
        .setShader(RESOURCE_DIR "/shaders/line_circle_pass.wgsl")
        .setVertexState()
        .setBlendState()
        .setPrimitiveState()
        .setColorTargetState(WGPUTextureFormat_BGRA8UnormSrgb)
        .setDepthStencilState(true, 0xFF, 0xFF, WGPUTextureFormat_Depth24PlusStencil8)
        .setFragmentState()
        .createPipeline(app);

    mOffsetBuffer.setSize(mMaxPoints * sizeof(LineSegment))
        .setLabel("Instancing Shader Storage Buffer for lines")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setMappedAtCraetion(false)  // No need to map initially
        .create(app);

    mTransformationBuffer.setSize(mMaxPoints * sizeof(glm::mat4))
        .setLabel("Transformation storage for lines")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setMappedAtCraetion(false)  // No need to map initially
        .create(app);

    mVertexBuffer.setSize(sizeof(mLineInstance))
        .setLabel("Single Line Instance Vertex Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(app);

    wgpuQueueWriteBuffer(app->getRendererResource().queue, mVertexBuffer.getBuffer(), 0, mLineInstance.data(),
                         sizeof(mLineInstance));

    mCircleIndexData = generateCircleIndexBuffer();
    mCircleVertexData = generateCircleVertices();

    mCircleVertexBuffer.setSize(mCircleVertexData.size() * sizeof(glm::vec3))
        .setLabel("Single Circle Instance Vertex Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(app);

    wgpuQueueWriteBuffer(app->getRendererResource().queue, mCircleVertexBuffer.getBuffer(), 0, mCircleVertexData.data(),
                         mCircleVertexData.size() * sizeof(glm::vec3));

    mCircleIndexBuffer.setSize(mCircleIndexData.size() * sizeof(uint16_t))
        .setLabel("Single Circle Index Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index)
        .setMappedAtCraetion()
        .create(app);

    wgpuQueueWriteBuffer(app->getRendererResource().queue, mCircleIndexBuffer.getBuffer(), 0, mCircleIndexData.data(),
                         mCircleIndexData.size() * sizeof(uint16_t));

    mBindingData.push_back({});
    mBindingData.push_back({});
    mBindingData[0] = {};
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].buffer = mOffsetBuffer.getBuffer();
    mBindingData[0].binding = 0;
    mBindingData[0].offset = 0;
    mBindingData[0].size = mMaxPoints * sizeof(LineSegment);

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = mTransformationBuffer.getBuffer();
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = mMaxPoints * sizeof(glm::mat4);

    mCameraBindingData.push_back({});
    mCameraBindingData[0].nextInChain = nullptr;
    mCameraBindingData[0].binding = 0;
    mCameraBindingData[0].buffer = app->getUniformBuffer().getBuffer();
    mCameraBindingData[0].offset = 0;
    mCameraBindingData[0].size = sizeof(CameraInfo);

    mBindGroup.create(app, mBindingData);
    mCameraBindGroup.create(app, mCameraBindingData);
}

void LineEngine::draw(Application* app, WGPURenderPassEncoder encoder) {
    // Calculate total points needed
    uint32_t totalPoints = 0;
    for (const auto& [id, group] : mLineGroups) {
        totalPoints += static_cast<uint32_t>(group.segment.size());
    }

    // Resize if needed
    // resizeBufferIfNeeded(app, totalPoints);

    // Recalculate offsets if structure changed
    if (mGlobalDirty) {
        uint32_t currentOffset = 0;
        for (auto& [id, group] : mLineGroups) {
            group.buffer_offset = currentOffset;
            currentOffset += static_cast<uint32_t>(group.segment.size());
            group.dirty = true;  // Force write since offset may have changed
        }
        mGlobalDirty = false;
    }

    // Write dirty groups to buffer
    WGPUQueue queue = app->getRendererResource().queue;
    for (auto& [id, group] : mLineGroups) {
        if (group.dirty && !group.segment.empty()) {
            uint64_t byteOffset = static_cast<uint64_t>(group.buffer_offset) * sizeof(LineSegment);
            uint64_t byteSize = group.segment.size() * sizeof(LineSegment);
            wgpuQueueWriteBuffer(queue, mOffsetBuffer.getBuffer(), byteOffset, group.segment.data(), byteSize);
            wgpuQueueWriteBuffer(queue, mTransformationBuffer.getBuffer(), id * sizeof(glm::mat4),
                                 glm::value_ptr(group.transformation), sizeof(glm::mat4));
            group.dirty = false;
            // No need to set dirty=false here; do it after successful write if you add error handling
        }
    }

    // Bind groups (as before)
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindGroup.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(encoder, 1, mCameraBindGroup.getBindGroup(), 0, nullptr);

    // Set pipeline and vertex buffer (as before)
    wgpuRenderPassEncoderSetPipeline(encoder, mPipeline->getPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0, sizeof(mLineInstance));

    // Draw each group separately
    for (const auto& [id, group] : mLineGroups) {
        uint32_t instanceCount = static_cast<uint32_t>(group.segment.size()) - 1;
        if (instanceCount > 0) {
            wgpuRenderPassEncoderDraw(encoder, 6, instanceCount, 0, group.buffer_offset);
        }
    }
}

void LineEngine::executePass() {
    {
        mLineRenderingPass->setColorAttachment(
            {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
        mLineRenderingPass->setDepthStencilAttachment({mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false,
                                                       StoreOp::Undefined, LoadOp::Undefined, false});
        mLineRenderingPass->init();

        WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(
            mApp->getRendererResource().commandEncoder, &mLineRenderingPass->mRenderPassDesc);
        draw(mApp, render_pass_encoder);
        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }
}

// Returns a handle for the new group
uint32_t LineEngine::addLines(const std::vector<glm::vec4>& points, const glm::vec3& color) {
    // Invalid cases
    if (points.size() < 2 || mNextGroupId >= 1024) {
        return std::numeric_limits<uint32_t>::max();
    }

    uint32_t id = mNextGroupId++;
    // std::vector<LineSegment> segments;
    mLineGroups[id] = {glm::mat4{1.0}, color, {}, 0, true};
    auto& segments = mLineGroups[id].segment;
    for (const auto& p : points) {
        segments.emplace_back(LineSegment{p, color, id});
        std::cout << segments.size() << " " << id << std::endl;
    }
    // std::cout << "Line is " << mNextGroupId << " " << mLineGroups[id].segment.transformationId << std::endl;
    mGlobalDirty = true;
    return id;
}

// Remove a group by handle
void LineEngine::removeLines(uint32_t id) {
    if (mLineGroups.erase(id)) {
        mGlobalDirty = true;
    }
}

// Update a group's points (e.g., when object moves)
void LineEngine::updateLines(uint32_t id, const std::vector<glm::vec4>& newPoints) {
    auto it = mLineGroups.find(id);
    if (it == mLineGroups.end()) return;

    LineGroup& group = it->second;
    bool sizeChanged = (newPoints.size() != group.segment.size());
    std::vector<LineSegment> segments;
    for (const auto& p : newPoints) {
        segments.emplace_back(LineSegment{p, group.groupColor, id});
    }
    group.segment = segments;
    group.dirty = true;
    if (sizeChanged) {
        mGlobalDirty = true;
    }
}

void LineEngine::updateLineTransformation(uint32_t id, const glm::mat4& trans) {
    auto it = mLineGroups.find(id);
    if (it == mLineGroups.end()) return;
    LineGroup& group = it->second;
    group.transformation = trans;
    group.dirty = true;
}
