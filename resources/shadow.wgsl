struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
    view: mat4x4f,
};

struct Vertex {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) tangent: vec3f,
    @location(4) biTangent: vec3f,
    @location(5) uv: vec2f,
    @builtin(instance_index) instance_index: u32
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
    isFlat: i32,
    useTexture: i32,
    isFoliage: i32,
    offsetId: u32,
    isHovered: u32,
    offset1: u32,
    offset2: u32,
    offset3: u32
}

@group(0) @binding(0) var<uniform> scene: Scene;
@group(0) @binding(1) var<storage, read> offsetInstance: array<OffsetData>;
@group(0) @binding(2) var<uniform> objectTranformation: ObjectInfo;
@group(0) @binding(3) var textureSampler: sampler;

@group(1) @binding(0) var diffuseMap: texture_2d<f32>;
@group(1) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(1) @binding(2) var normal_map: texture_2d<f32>;

@vertex
fn vs_main(vertex: Vertex) -> VSOutput {

    var world_position: vec4f;
    let off_id: u32 = objectTranformation.offsetId * 100000;
    if vertex.instance_index == 0 {
        world_position = objectTranformation.transformations * vec4f(vertex.position, 1.0);
    } else {
        world_position = offsetInstance[vertex.instance_index + off_id].transformation * vec4f(vertex.position, 1.0);
    }
    var vsOut: VSOutput;
    vsOut.position = scene.projection * scene.view * world_position;
    vsOut.uv = vertex.uv;
    return vsOut;
}

@fragment
fn fs_main(in: VSOutput) -> @location(0) vec4f {

    var pos = in.position;
    let transparency = textureSample(diffuseMap, textureSampler, in.uv).a;
    if objectTranformation.useTexture == 1 && transparency == 0.0 {
       discard; 
    }
    return pos;
}
