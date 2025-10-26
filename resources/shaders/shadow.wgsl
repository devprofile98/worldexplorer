struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
    view: mat4x4f,
    padding: vec4f,
};

struct Vertex {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) tangent: vec3f,
    @location(4) biTangent: vec3f,
    @location(5) uv: vec2f,
    @builtin(instance_index) instance_index: u32,
    @location(6) boneIds: vec4i,
    @location(7) boneWeights: vec4f,
};

struct VSOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

struct OffsetData {
    transformation: mat4x4f, // Array of 10 offset vectors
    minAABB: vec4f,
    maxAABB: vec4f
};


struct ObjectInfo {
    transformations: mat4x4f,
    offsetId: u32,
    isHovered: u32,
    isAnimated: u32,
}

struct Material {
    isFlat: i32,
    useTexture: i32,
    isFoliage: i32,
    materialProps: u32,
    metallicness: f32,
    uvMultiplier: vec3f,
}



@group(0) @binding(0) var<uniform> scene: array<Scene, 2>;
@group(0) @binding(1) var<storage, read> offsetInstance: array<OffsetData>;
@group(0) @binding(2) var textureSampler: sampler;

@group(1) @binding(0) var diffuseMap: texture_2d<f32>;
@group(1) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(1) @binding(2) var normal_map: texture_2d<f32>;

@group(2) @binding(0) var<storage, read> visible_instances_indices: array<u32>;


@group(3) @binding(0) var<uniform> objectTranformation: ObjectInfo;
@group(3) @binding(1) var<uniform> bonesFinalTransform: array<mat4x4f, 100>;
@group(3) @binding(2) var<uniform> meshMaterial: Material;


@group(4) @binding(0) var<uniform> sceneIndex: u32;

@vertex
fn vs_main(vertex: Vertex) -> VSOutput {

    let off_id: u32 = objectTranformation.offsetId * 100000;

    var transform: mat4x4f;
    if vertex.instance_index != 0 {
        let original_instance_idx = visible_instances_indices[off_id + vertex.instance_index];
        transform = offsetInstance[original_instance_idx + off_id].transformation;
    } else {
        transform = objectTranformation.transformations;
    }


    var world_position: vec4f;
    //if (objectTranformation.materialProps >> 6) == 0u {
    if objectTranformation.isAnimated == 0u {
        world_position = transform * vec4f(vertex.position, 1.0);
    } else {
        var bone_matrix = mat4x4<f32>(
            vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 0
            vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 1
            vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 2
            vec4<f32>(0.0, 0.0, 0.0, 0.0)  // Column 3
        );

        bone_matrix += bonesFinalTransform[vertex.boneIds.x] * normalize(vertex.boneWeights).x;
        bone_matrix += bonesFinalTransform[vertex.boneIds.y] * normalize(vertex.boneWeights).y;
        bone_matrix += bonesFinalTransform[vertex.boneIds.z] * normalize(vertex.boneWeights).z;
        bone_matrix += bonesFinalTransform[vertex.boneIds.w] * normalize(vertex.boneWeights).w;
        world_position = transform * bone_matrix * vec4f(vertex.position, 1.0);
    }


    var vsOut: VSOutput;
    vsOut.position = scene[sceneIndex].projection * world_position;
    vsOut.uv = vertex.uv;
    return vsOut;
}

@fragment
fn fs_main(in: VSOutput) -> @location(0) vec4f {

    var pos = in.position;
    let transparency = textureSample(diffuseMap, textureSampler, in.uv).a;
    if /*objectTranformation.useTexture == 1  && */ transparency < 0.1 {
       discard;
    }
    return pos;
}
