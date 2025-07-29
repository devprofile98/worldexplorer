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
};

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) tangent: vec3f,
    @location(4) biTangent: vec3f,
    @location(5) uv: vec2f,
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
@group(0) @binding(5) var near_depth_texture: texture_depth_2d;
@group(0) @binding(6) var grass_ground_texture: texture_2d<f32>;
@group(0) @binding(7) var rock_mountain_texture: texture_2d<f32>;
@group(0) @binding(8) var sand_lake_texture: texture_2d<f32>;
@group(0) @binding(15) var grass_normal_texture: texture_2d<f32>;
@group(0) @binding(9) var snow_mountain_texture: texture_2d<f32>;
@group(0) @binding(10) var depth_texture: texture_depth_2d_array;
@group(0) @binding(11) var<uniform> lightSpaceTrans: array<Scene, 5>;
@group(0) @binding(12) var shadowMapSampler: sampler_comparison;
@group(0) @binding(13) var<storage, read> offsetInstance: array<OffsetData>;
@group(0) @binding(14) var<uniform> numOfCascades: u32;

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var standard_depth: texture_depth_2d;

@group(4) @binding(0) var<uniform> myuniformindex: u32;

@group(5) @binding(0) var<storage, read> visible_instances_indices: array<u32>;

const PI: f32 = 3.141592653589793;

fn degreeToRadians(degrees: f32) -> f32 {
    return degrees * (PI / 180.0); // Convert 90 degrees to radians
}

fn decideColor(default_color: vec3f, is_flat: i32, Y: f32) -> vec3f {

    if is_flat == 1 {
        if Y > 2.0 {
            return vec3f(1.0, 0.0, 0.0);
        }
        return vec3f(0.0, 1.0, 0.0);
    }
    return default_color;
}

@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    //var world_position: vec4f;
    let off_id: u32 = objectTranformation.offsetId * 100000;
    //let is_primary = f32(instance_index == 0);
    var transform: mat4x4f;
    if instance_index != 0 {
	let original_instance_idx = visible_instances_indices[off_id + instance_index];
        transform = offsetInstance[original_instance_idx + off_id].transformation;
    } else {
        transform = objectTranformation.transformations;
    }


    let world_position = transform * vec4f(in.position, 1.0);
    out.viewSpacePos = uMyUniform[0].viewMatrix * world_position;
    out.normal = (transform * vec4f(in.normal, 0.0)).xyz;
    //let color = min(max(abs(out.viewSpacePos.z) / 20.0, 0.02), 0.04);
    let color = max(pow(clamp(abs(out.viewSpacePos.z), 0.0, 10.0), 2.0) / 1250.0, 0.02);
    let extruded_position = transform * vec4f(in.position + normalize(in.normal) * color , 1.0);
    out.viewSpacePos = uMyUniform[0].viewMatrix * extruded_position;

    out.position = uMyUniform[0].projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform[0].cameraWorldPosition - world_position.xyz;
    out.color = vec3f(color * 3,0,0);
    out.uv = in.uv;

    let T = normalize(vec3f((transform * vec4(in.tangent, 0.0)).xyz));
    let B = normalize(vec3f((transform * vec4(in.biTangent, 0.0)).xyz));
    let N = normalize(vec3f((transform * vec4(in.normal, 0.0)).xyz));

    out.tangent = T;
    out.biTangent = B;
    out.aNormal = out.normal;

    var index:u32 = 0;

    for (var i: u32 = 0u; i < numOfCascades; i = i + 1u) {
        if ( length(out.viewSpacePos) < lightSpaceTrans[i].farZ){
        	index= i;
		break;
        }
    }
    // if length(out.viewSpacePos) > ElapsedTime { index = 1;}
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    out.shadowIdx = index;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    //return vec4f(in.color, 1.0);
	let frag_ambient = textureSample(diffuse_map, textureSampler, in.uv).rgba;
	if (frag_ambient.a < 0.001 ) {
		discard;
	}

	let screen_uv = in.position.xy / vec2f(1920.0, 1022.0);
	// let existing_depth_ndc_z = textureSample(near_depth_texture, textureSampler, screen_uv).x;
	let outline_ndc_z = in.position.z / in.position.w;
        let compare_res = textureSampleCompare(standard_depth, shadowMapSampler, screen_uv, outline_ndc_z  - 0.0001 );

        var outline_alpha = 0.9; // Make it more transparent
    if (compare_res == 1) {
        outline_alpha = 0.3; // Make it more transparent
    	return vec4f(vec3f(1.0, 1.0, 0.0), outline_alpha);
    }

    return vec4f(vec3f(1.0, 0.5, 0.0 ), outline_alpha);
}

