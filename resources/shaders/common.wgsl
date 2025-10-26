
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(5) shadowPos: vec4f,
    @location(0) color: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) viewDirection: vec3f,
    @location(4) worldPos: vec3f,
    @location(6) viewSpacePos: vec4f,
    @location(7) tangent: vec3f,
    @location(8) biTangent: vec3f,
    @location(9) aNormal: vec3f,
    @location(10) @interpolate(flat) shadowIdx: u32,
    @location(11) @interpolate(flat) materialProps: u32,
    @location(12) @interpolate(flat) userSpecular: f32,
};


struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) tangent: vec3f,
    @location(4) biTangent: vec3f,
    @location(5) uv: vec2f,
    @location(6) boneIds: vec4i,
    @location(7) boneWeights: vec4f,
};


struct MyUniform {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    cameraWorldPosition: vec3f,
    time: f32,
};

struct LightingUniforms {
    directions: array<vec4f, 2>,
    colors: array<vec4f, 2>,
}

struct ObjectInfo {
    transformations: mat4x4f,
    offsetId: u32,
    isHovered: u32,
    isAnimated: u32,
}

struct Material {
    isFlat: u32,
    useTexture: u32,
    isFoliage: u32,
    materialProps: u32, // <-- Offset 12 (16 bytes total)
    metallicness: f32,  // <-- Offset 16 (4 bytes)
    // 12 bytes of padding needed here to make the next offset 32
    uvMultiplier: vec3f, // <-- Offset 32 (12 bytes)
}


struct PointLight {
    position: vec4f,
    direction: vec4f,
    ambient: vec4f,
    diffuse: vec4f,
    specular: vec4f,
    constant: f32,
    linear: f32,
    quadratic: f32,
    ftype: i32,
    cutOff: f32,
    outerCutOff: f32
}

struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
    view: mat4x4f,
    farZ: f32,
    pad1: f32,
    pad2: f32,
    pad3: f32,
};

struct OffsetData {
    transformation: mat4x4f, // Array of 10 offset vectors
    minAABB: vec4f,
    maxAABB: vec4f
};

@group(0) @binding(0) var<uniform> uMyUniform: array<MyUniform, 10>;
@group(0) @binding(1) var<uniform> lightCount: i32;
@group(0) @binding(2) var textureSampler: sampler;
@group(0) @binding(3) var<uniform> lightingInfos: LightingUniforms;
@group(0) @binding(4) var<uniform> pointLight: array<PointLight,10>;
@group(0) @binding(5) var<uniform> numOfCascades: u32;
//@group(0) @binding(6) var grass_ground_texture: texture_2d_array<f32>;
//@group(0) @binding(7) var rock_mountain_texture: texture_2d_array<f32>;
//@group(0) @binding(8) var sand_lake_texture: texture_2d_array<f32>;
//@group(0) @binding(9) var snow_mountain_texture: texture_2d_array<f32>;
@group(0) @binding(6) var depth_texture: texture_depth_2d_array;
@group(0) @binding(7) var<uniform> lightSpaceTrans: array<Scene, 5>;
@group(0) @binding(8) var shadowMapSampler: sampler_comparison;
@group(0) @binding(9) var<storage, read> offsetInstance: array<OffsetData>;

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;
@group(1) @binding(1) var<uniform> bonesFinalTransform: array<mat4x4f, 100>;

