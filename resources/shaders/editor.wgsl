#include "common.wgsl"

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
        transform = offsetInstance[instance_index + off_id].transformation;
    } else {
        transform = objectTranformation.transformations;
    }


    let world_position = transform * vec4f(in.position, 1.0);
    out.viewSpacePos = uMyUniform[0].viewMatrix * world_position;
    out.normal = (transform * vec4f(in.normal, 0.0)).xyz;
    //let color = min(max(abs(out.viewSpacePos.z) / 20.0, 0.02), 0.04);
    let color = max(pow(clamp(abs(out.viewSpacePos.z), 0.0, 10.0), 2.0) / 1250.0, 0.02);
    let extruded_position = transform * vec4f(in.position + in.normal * color, 1.0);
    out.viewSpacePos = uMyUniform[0].viewMatrix * extruded_position;

    out.position = uMyUniform[0].projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform[0].cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    mix(out.color, vec3f(1.0, 1.0, 0.0), f32(objectTranformation.isHovered));
    out.uv = in.uv;
    out.shadowIdx = objectTranformation.isHovered;

    let T = normalize(vec3f((transform * vec4(in.tangent, 0.0)).xyz));
    let B = normalize(vec3f((transform * vec4(in.biTangent, 0.0)).xyz));
    let N = normalize(vec3f((transform * vec4(in.normal, 0.0)).xyz));

    out.tangent = T;
    out.biTangent = B;
    out.aNormal = out.normal;

    var index: u32 = 0;

    for (var i: u32 = 0u; i < numOfCascades; i = i + 1u) {
        if length(out.viewSpacePos) < lightSpaceTrans[i].farZ {
            index = i;
		break;
        }
    }
    // if length(out.viewSpacePos) > ElapsedTime { index = 1;}
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    //out.shadowIdx = index;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    if in.shadowIdx > 0 {
        return vec4f(in.color, 0.3);
    }
    return vec4f(in.color, 0.9);
}

