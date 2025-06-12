
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(5) shadowPos: vec4f,
    @location(0) color: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) viewDirection: vec3f,
    @location(4) worldPos: vec3f,
    @location(6) viewSpacePos: vec4f,
};

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) uv: vec2f,
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
    isFlat: i32,
    useTexture: i32,
    isFoliage: i32,
    offsetId: u32,
    isHovered: u32,
    offset1: u32,
    offset2: u32,
    offset3: u32
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
};

struct OffsetData {
    transformation: mat4x4f, // Array of 10 offset vectors
};


@group(0) @binding(0) var<uniform> uMyUniform: MyUniform;
@group(0) @binding(1) var diffuse_map: texture_2d<f32>;
@group(0) @binding(2) var textureSampler: sampler;
@group(0) @binding(3) var<uniform> lightingInfos: LightingUniforms;
@group(0) @binding(4) var<uniform> pointLight: array<PointLight,10>;
@group(0) @binding(5) var metalic_roughness_texture: texture_2d<f32>;
@group(0) @binding(6) var grass_ground_texture: texture_2d<f32>;
@group(0) @binding(7) var rock_mountain_texture: texture_2d<f32>;
@group(0) @binding(8) var sand_lake_texture: texture_2d<f32>;
@group(0) @binding(9) var snow_mountain_texture: texture_2d<f32>;
@group(0) @binding(10) var depth_texture: texture_depth_2d;
@group(0) @binding(11) var<uniform> lightSpaceTrans: array<Scene, 2>;
@group(0) @binding(12) var shadowMapSampler: sampler_comparison;
@group(0) @binding(13) var<storage, read> offsetInstance: array<OffsetData>;
@group(0) @binding(14) var<uniform> ElapsedTime: f32;
@group(0) @binding(15) var<uniform> lightCount: i32;
@group(0) @binding(16) var near_depth_texture: texture_depth_2d;

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    //var world_position: vec4f;
    let off_id: u32 = objectTranformation.offsetId * 100000;
    //let is_primary = f32(instance_index == 0);
    var transform: mat4x4f;
    if instance_index != 0 {
        transform = offsetInstance[instance_index + off_id].transformation;
    } else {
        transform = objectTranformation.transformations;
    }

    let world_position = transform * vec4f(in.position, 1.0);
    out.normal = (transform * vec4f(in.normal, 1.0)).xyz;

    out.viewSpacePos = uMyUniform.viewMatrix * world_position;
    out.position = uMyUniform.projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform.cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.uv = in.uv;
    var index = 0;
    if length(out.viewSpacePos) > ElapsedTime { index = 1;}
;
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    //out.shadowPos = shadow_position;
    return out;
}



@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let normal = normalize(in.normal);

    let view_direction = normalize(in.viewDirection);
}
