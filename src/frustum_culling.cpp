
#include "frustum_culling.h"

#include <cstdint>
#include <format>

#include "application.h"
#include "binding_group.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "rendererResource.h"
#include "shapes.h"
#include "webgpu/webgpu.h"

std::ostream& operator<<(std::ostream& os, const frustum::Plane& plane) {
    os << "Plane(normal: [" << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z
       << "], d: " << plane.distance << ")";
    return os;
}

frustum::Plane makePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    glm::vec3 normal = normalize(cross(b - a, c - a));
    float d = -dot(normal, a);
    return frustum::Plane{d, normal};  // plane: normal.x * x + normal.y * y + normal.z * z + d = 0
}

bool isBoxOutsidePlane(const frustum::Plane& plane, const glm::vec3& min, const glm::vec3& max) {
    (void)max;
    if (dot(plane.normal, min) + plane.distance > 0) {
        return false;  // at least one corner is inside this plane
    }
    return true;  // all corners are outside this plane
}

bool Frustum::AABBTest(const glm::vec3& min, const glm::vec3& max) {
    for (size_t i = 0; i < 6; i++) {
        if (isBoxOutsidePlane(faces[i], min, max)) {
            return false;  // Culled
        }
    }
    return true;  // Visible or intersecting
}

namespace frustum {
Plane::Plane(const glm::vec3& p1, const glm::vec3& norm) : normal(glm::normalize(norm)), distance(glm::dot(norm, p1)) {}
Plane::Plane(float d, const glm::vec3& norm) : normal(norm), distance(d) {}
}  // namespace frustum

Buffer inputBuffer;          // copy dst, storage
Buffer outputBuffer;         // copy dst, map read
Buffer frustumPlanesBuffer;  // copy dst, map read
size_t data_size_bytes = 0;  //
WGPUBindGroup computeBindGroup;
// WGPUBindGroup computeBindGroup2;
WGPUComputePipeline computePipeline;
WGPUBindGroupLayout objectinfo_bg_layout;

std::vector<uint32_t> input_values = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

Buffer& getFrustumPlaneBuffer() { return frustumPlanesBuffer; }

BindingGroup mBindingGroup;
BindingGroup mObjectInfoBindgroup;
void setupComputePass(Application* app, WGPUBuffer instanceDataBuffer) {
    data_size_bytes = input_values.size() * sizeof(uint32_t);
    auto& resources = app->getRendererResource();

    const char* shader_code = R"(

	struct FrustumPlane {
	    N_D: vec4f, // (Normal.xyz, D.w)
	};
	struct FrustumPlanesUniform {
	    planes: array<FrustumPlane, 2>,
	};

	struct OffsetData {
	    transformation: mat4x4f, // Array of 10 offset vectors
	    minAABB: vec4f,
	    maxAABB: vec4f
	};

	struct DrawIndexedIndirectArgs {
	    indexCount: u32,
	    instanceCount: atomic<u32>, // This is what we modify atomically
	    firstIndex: u32,
	    baseVertex: u32,
	    firstInstance: u32,
	};

    struct ObjectInfo {
        transformations: mat4x4f,
        offsetId: u32,
        isHovered: u32,
        isAnimated: u32,
    }

    struct Camera {
        projectionMatrix: mat4x4f,
        viewMatrix: mat4x4f,
        modelMatrix: mat4x4f,
        color: vec4f,
        cameraWorldPosition: vec3f,
        time: f32,
    };

    @group(0) @binding(0) var<storage, read> input_data: array<u32>;
    @group(0) @binding(1) var<storage, read_write> visible_instances_indices: array<u32>;
	@group(0) @binding(2) var<storage, read> instanceData: array<OffsetData>;
	@group(0) @binding(3) var<uniform> uFrustumPlanes: FrustumPlanesUniform;
    @group(0) @binding(4) var<uniform> uCamera: array<Camera, 10>;
	
	@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;
	@group(1) @binding(1) var<storage, read_write> indirect_draw_args: DrawIndexedIndirectArgs;


        @compute @workgroup_size(32)
        fn main(@builtin(global_invocation_id) global_id: vec3u) {
            let index = global_id.x;
            // if (indirect_draw_args.instanceCount < 2u){
            //if (index < 31u){
	            let off_id: u32 = objectTranformation.offsetId * 100000u;
		        let transform = instanceData[index + off_id].transformation;
		        let minAABB = instanceData[index + off_id].minAABB;
		        let maxAABB = instanceData[index + off_id].maxAABB;

		        let left = dot(normalize(uFrustumPlanes.planes[0].N_D.xyz), minAABB.xyz) + uFrustumPlanes.planes[0].N_D.w;
		        let right = dot(normalize(uFrustumPlanes.planes[1].N_D.xyz), minAABB.xyz) + uFrustumPlanes.planes[1].N_D.w;

		        let max_left = dot(normalize(uFrustumPlanes.planes[0].N_D.xyz),  maxAABB.xyz) + uFrustumPlanes.planes[0].N_D.w;
		        let max_right = dot(normalize(uFrustumPlanes.planes[1].N_D.xyz), maxAABB.xyz) + uFrustumPlanes.planes[1].N_D.w;

		        let is_lod0 = uCamera[0].cameraWorldPosition - maxAABB.xyz;
                // if (length(is_lod0) < 23.0){
	            if (left >= -1.0 && max_left > -1.0 && right >= -1.0 && max_right >= -1.0) {
		            let write_idx = atomicAdd(&indirect_draw_args.instanceCount, 1u);
		            // if (write_idx < arrayLength(&visible_instances_indices)) {
		                visible_instances_indices[off_id + write_idx] = index; // Store the original global_id.x as the visible index
	                // }
		        }
            // }
        }
    )";

    // create neccesarry buffers
    inputBuffer.setLabel("input buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setSize(data_size_bytes)
        .setMappedAtCraetion()
        .create(app);

    wgpuQueueWriteBuffer(resources.queue, inputBuffer.getBuffer(), 0, input_values.data(), data_size_bytes);
    // one buffer for input, one for output
    app->mVisibleIndexBuffer.setLabel("visible index buffer")
        .setUsage(WGPUBufferUsage_CopySrc | WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect)
        .setSize(sizeof(uint32_t) * 100000 * 5)
        .setMappedAtCraetion()
        .create(app);

    outputBuffer.setLabel("output result buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead)
        .setSize(data_size_bytes)
        .setMappedAtCraetion()
        .create(app);

    frustumPlanesBuffer.setLabel("frustum planes buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(FrustumPlanesUniform))
        .setMappedAtCraetion()
        .create(app);

    // create the compute pass, bind group and pipeline
    WGPUShaderSourceWGSL shader_wgsl_desc = {};
    shader_wgsl_desc.chain.next = nullptr;
    shader_wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shader_wgsl_desc.code = {shader_code, strlen(shader_code)};
    WGPUShaderModuleDescriptor shader_module_desc = {};
    shader_module_desc.nextInChain = &shader_wgsl_desc.chain;
    shader_module_desc.label = {"Simple Compute Shader Module", WGPU_STRLEN};
    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(resources.device, &shader_module_desc);

    WGPUBindGroupLayout bind_group_layout =
        mBindingGroup.addBuffer(0, BindGroupEntryVisibility::COMPUTE, BufferBindingType::STORAGE_READONLY, 0)
            .addBuffer(1, BindGroupEntryVisibility::COMPUTE, BufferBindingType::STORAGE, 0)
            .addBuffer(2, BindGroupEntryVisibility::COMPUTE, BufferBindingType::STORAGE_READONLY, 0)
            .addBuffer(3, BindGroupEntryVisibility::COMPUTE, BufferBindingType::UNIFORM, 0)
            .addBuffer(4, BindGroupEntryVisibility::COMPUTE, BufferBindingType::UNIFORM, sizeof(CameraInfo) * 10)
            .createLayout(app, "Compute bind group layout");

    objectinfo_bg_layout =
        mObjectInfoBindgroup.addBuffer(0, BindGroupEntryVisibility::COMPUTE, BufferBindingType::UNIFORM, 0)
            .addBuffer(1, BindGroupEntryVisibility::COMPUTE, BufferBindingType::STORAGE, 0)
            .createLayout(app, "Compute Bind Group For Object info");

    // 5. Create Pipeline Layout
    WGPUBindGroupLayout bind_group_layouts[] = {bind_group_layout, objectinfo_bg_layout};
    WGPUPipelineLayoutDescriptor pipeline_layout_desc = {};
    pipeline_layout_desc.label = {"Compute Pipeline Layout", WGPU_STRLEN};
    pipeline_layout_desc.bindGroupLayoutCount = 2;
    pipeline_layout_desc.bindGroupLayouts = bind_group_layouts;
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(resources.device, &pipeline_layout_desc);

    // 6. Create Compute Pipeline
    WGPUComputePipelineDescriptor compute_pipeline_desc = {};
    compute_pipeline_desc.label = {"Simple Compute Pipeline", WGPU_STRLEN};
    compute_pipeline_desc.layout = pipeline_layout;
    compute_pipeline_desc.compute.module = shader_module;
    compute_pipeline_desc.compute.entryPoint = {"main", WGPU_STRLEN};  // Matches `fn main` in WGSL
    computePipeline = wgpuDeviceCreateComputePipeline(resources.device, &compute_pipeline_desc);

    // 7. Create Bind Group (linking actual buffers to shader bindings)
    WGPUBindGroupEntry bind_group_entries[5] = {};
    bind_group_entries[0].binding = 0;
    bind_group_entries[0].buffer = inputBuffer.getBuffer();
    bind_group_entries[0].offset = 0;
    bind_group_entries[0].size = data_size_bytes;

    bind_group_entries[1].binding = 1;
    bind_group_entries[1].buffer = app->mVisibleIndexBuffer.getBuffer();
    bind_group_entries[1].offset = 0;
    bind_group_entries[1].size = sizeof(uint32_t) * 100000 * 5;

    bind_group_entries[2].binding = 2;
    bind_group_entries[2].buffer = instanceDataBuffer;
    bind_group_entries[2].offset = 0;
    bind_group_entries[2].size = sizeof(InstanceData) * 100000 * 10;

    bind_group_entries[3].binding = 3;
    bind_group_entries[3].buffer = frustumPlanesBuffer.getBuffer();
    bind_group_entries[3].offset = 0;
    bind_group_entries[3].size = sizeof(FrustumPlanesUniform);

    bind_group_entries[4].nextInChain = nullptr;
    bind_group_entries[4].binding = 4;
    bind_group_entries[4].buffer = app->mUniformBuffer.getBuffer();
    bind_group_entries[4].offset = 0;
    bind_group_entries[4].size = sizeof(CameraInfo) * 10;

    WGPUBindGroupDescriptor bind_group_desc = {};
    bind_group_desc.label = {"Compute Bind Group", WGPU_STRLEN};
    bind_group_desc.layout = bind_group_layout;
    bind_group_desc.entryCount = 5;
    bind_group_desc.entries = bind_group_entries;
    computeBindGroup = wgpuDeviceCreateBindGroup(resources.device, &bind_group_desc);
};

WGPUBindGroup createObjectInfoBindGroupForComputePass(Application* app, WGPUBuffer objetcInfoBuffer,
                                                      WGPUBuffer indirectDrawArgsBuffer) {
    WGPUBindGroupEntry objectinfo_bg_entries[2] = {};
    objectinfo_bg_entries[0].binding = 0;
    objectinfo_bg_entries[0].buffer = objetcInfoBuffer;
    objectinfo_bg_entries[0].offset = 0;
    objectinfo_bg_entries[0].size = sizeof(ObjectInfo);

    objectinfo_bg_entries[1].binding = 1;
    objectinfo_bg_entries[1].buffer = indirectDrawArgsBuffer;
    objectinfo_bg_entries[1].offset = 0;
    objectinfo_bg_entries[1].size = sizeof(DrawIndexedIndirectArgs);

    WGPUBindGroupDescriptor objectinfo_bg_desc = {};
    objectinfo_bg_desc.label = {"Object info Compute Bind Group", WGPU_STRLEN};
    objectinfo_bg_desc.layout = objectinfo_bg_layout;
    objectinfo_bg_desc.entryCount = 2;
    objectinfo_bg_desc.entries = objectinfo_bg_entries;

    return wgpuDeviceCreateBindGroup(app->getRendererResource().device, &objectinfo_bg_desc);
}

void runFrustumCullingTask(Application* app, WGPUCommandEncoder encoder) {
    auto& objs = ModelRegistry::instance().getLoadedModel(Visibility_User);

    for (auto& model : objs) {
        if (model->instance != nullptr) {
            auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            wgpuQueueWriteBuffer(app->getRendererResource().queue, model->mIndirectDrawArgsBuffer.getBuffer(),
                                 sizeof(uint32_t), &indirect, sizeof(uint32_t));

            WGPUComputePassDescriptor compute_pass_desc = {};
            std::string compute_name = "Simple Compute Pass for " + model->getName();
            compute_pass_desc.label = {compute_name.c_str(), WGPU_STRLEN};
            compute_pass_desc.nextInChain = nullptr;
            WGPUComputePassEncoder compute_pass_encoder =
                wgpuCommandEncoderBeginComputePass(encoder, &compute_pass_desc);

            // Set the pipeline and bind group for the compute pass
            wgpuComputePassEncoderSetPipeline(compute_pass_encoder, computePipeline);
            wgpuComputePassEncoderSetBindGroup(compute_pass_encoder, 0, computeBindGroup, 0,
                                               nullptr);  // Group 0, no dynamic offsets
            auto objectinfo_bg = createObjectInfoBindGroupForComputePass(app, model->getUniformBuffer().getBuffer(),
                                                                         model->mIndirectDrawArgsBuffer.getBuffer());
            wgpuComputePassEncoderSetBindGroup(compute_pass_encoder, 1, objectinfo_bg, 0, nullptr);

            // uint32_t workgroup_size_x = 32;  // Must match shader's @workgroup_size(32)
            // uint32_t num_workgroups_x =
            //     (model->instance->getInstanceCount() + workgroup_size_x - 1) / workgroup_size_x;  // Ceil division
            // std::cout << num_workgroups_x << '\n';

            // wgpuComputePassEncoderDispatchWorkgroups(compute_pass_encoder, num_workgroups_x, 1, 1);
            //
            uint32_t workgroup_size_x = 32;  // Must match shader's @workgroup_size(32)
            uint32_t num_workgroups_x = ((31 * 3) + workgroup_size_x - 1) / workgroup_size_x;  // Ceil division
            wgpuComputePassEncoderDispatchWorkgroups(compute_pass_encoder, num_workgroups_x, 1, 1);

            // End the compute pass
            wgpuComputePassEncoderEnd(compute_pass_encoder);
            wgpuBindGroupRelease(objectinfo_bg);
            wgpuComputePassEncoderRelease(compute_pass_encoder);
        }

        // perform a buffer copy for indirect draw args for mesh parts
        if (model->instance != nullptr) {
            for (auto& [mat_id, mesh] : model->mMeshes) {
                wgpuCommandEncoderCopyBufferToBuffer(
                    encoder, model->mIndirectDrawArgsBuffer.getBuffer(),
                    offsetof(DrawIndexedIndirectArgs, instanceCount), mesh.mIndirectDrawArgsBuffer.getBuffer(),
                    offsetof(DrawIndexedIndirectArgs, instanceCount), sizeof(uint32_t));
            }
        }
    }
}

FrustumCorners getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
    const auto inv = glm::inverse(proj * view);
    std::vector<glm::vec4> frustumCorners;
    frustumCorners.reserve(8);

    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    FrustumCorners corners;

    corners.nearBottomLeft = frustumCorners[0];
    corners.farBottomLeft = frustumCorners[1];
    corners.nearTopLeft = frustumCorners[2];
    corners.farTopLeft = frustumCorners[3];
    corners.nearBottomRight = frustumCorners[4];
    corners.farBottomRight = frustumCorners[5];
    corners.nearTopRight = frustumCorners[6];
    corners.farTopRight = frustumCorners[7];
    return corners;
}

std::vector<FrustumPlane> create2FrustumPlanes(const FrustumCorners& corners) {
    auto ab = glm::vec3(corners.nearBottomLeft - corners.farBottomLeft);
    auto ac = glm::vec3(corners.nearBottomLeft - corners.nearTopLeft);
    auto left_plane_normal = glm::normalize(glm::cross(ab, ac));
    auto left_constant_sigend_distanc = -glm::dot(left_plane_normal, glm::vec3(corners.farTopLeft));

    ab = glm::vec3(corners.nearBottomRight - corners.farBottomRight);
    ac = glm::vec3(corners.nearBottomRight - corners.nearTopRight);
    auto right_plane_normal = glm::normalize(glm::cross(ac, ab));
    auto right_constant_sigend_distanc = -glm::dot(right_plane_normal, glm::vec3(corners.farTopRight));

    FrustumPlane fr{};
    fr.N_D = {right_plane_normal.x, right_plane_normal.y, right_plane_normal.z, right_constant_sigend_distanc};

    FrustumPlane fl{};
    fl.N_D = {left_plane_normal.x, left_plane_normal.y, left_plane_normal.z, left_constant_sigend_distanc};

    return {fl, fr};
}

bool isInFrustum(const FrustumCorners& corners, BaseModel* model) {
    if (model == nullptr) {
        return false;
    }

    // for left plane
    auto ab = glm::vec3(corners.nearBottomLeft - corners.farBottomLeft);
    auto ac = glm::vec3(corners.nearBottomLeft - corners.nearTopLeft);
    auto left_plane_normal = glm::normalize(glm::cross(ab, ac));
    auto left_constant_sigend_distanc = -glm::dot(left_plane_normal, glm::vec3(corners.farTopLeft));

    // for right plane
    ab = glm::vec3(corners.nearBottomRight - corners.farBottomRight);
    ac = glm::vec3(corners.nearBottomRight - corners.nearTopRight);
    auto right_plane_normal = glm::normalize(glm::cross(ab, ac));
    auto right_constant_sigend_distanc = -glm::dot(right_plane_normal, glm::vec3(corners.farTopRight));

    auto [min, max] = model->getWorldSpaceAABB();
    auto dmin = glm::dot(left_plane_normal, min) + left_constant_sigend_distanc;
    auto dmax = glm::dot(left_plane_normal, max) + left_constant_sigend_distanc;

    auto drmin = glm::dot(right_plane_normal, min) + right_constant_sigend_distanc;
    auto drmax = glm::dot(right_plane_normal, max) + right_constant_sigend_distanc;

    return (drmax < 0.0 || drmin < 0.0) && (dmin > 0.0 || dmax > 0.0);
}

//
