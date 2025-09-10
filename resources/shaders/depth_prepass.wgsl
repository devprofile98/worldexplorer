#include "common.wgsl"


@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var<uniform> clipping_plane: vec4f;

@group(5) @binding(0) var<storage, read> visible_instances_indices: array<u32>;


@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    let off_id: u32 = objectTranformation.offsetId * 100000;
    var transform: mat4x4f;

    if instance_index != 0 {
        let original_instance_idx = visible_instances_indices[off_id + instance_index];
        transform = offsetInstance[original_instance_idx + off_id].transformation;
    } else {
        transform = objectTranformation.transformations;
    }


    var skinned_position = vec4f(0.0, 0.0, 0.0, 0.0);
    skinned_position += bonesFinalTransform[in.boneIds.x] * vec4f(in.position, 1.0) * in.boneWeights.x;
    skinned_position += bonesFinalTransform[in.boneIds.y] * vec4f(in.position, 1.0) * in.boneWeights.y;
    skinned_position += bonesFinalTransform[in.boneIds.z] * vec4f(in.position, 1.0) * in.boneWeights.z;
    skinned_position += bonesFinalTransform[in.boneIds.w] * vec4f(in.position, 1.0) * in.boneWeights.w;

    //let world_position = transform * vec4f(in.position, 1.0);
    let world_position = transform * skinned_position;

    var skinned_normal = vec4f(0.0, 0.0, 0.0, 0.0);
    skinned_normal += bonesFinalTransform[in.boneIds.x] * vec4f(in.normal, 0.0) * in.boneWeights.x;
    skinned_normal += bonesFinalTransform[in.boneIds.y] * vec4f(in.normal, 0.0) * in.boneWeights.y;
    skinned_normal += bonesFinalTransform[in.boneIds.z] * vec4f(in.normal, 0.0) * in.boneWeights.z;
    skinned_normal += bonesFinalTransform[in.boneIds.w] * vec4f(in.normal, 0.0) * in.boneWeights.w;

    //out.normal = (transform * vec4f(in.normal, 0.0f)).xyz;
    out.normal = (transform * skinned_normal).xyz;

    out.viewSpacePos = uMyUniform[myuniformindex].viewMatrix * world_position;
    out.position = uMyUniform[myuniformindex].projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform[myuniformindex].cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.uv = in.uv;

    let T = normalize(vec3f((transform * vec4(in.tangent, 0.0)).xyz));
    let B = normalize(vec3f((transform * vec4(in.biTangent, 0.0)).xyz));
    let N = normalize(vec3f((transform * vec4(in.normal, 0.0)).xyz));

    out.tangent = T;
    out.biTangent = B;
    out.aNormal = normalize(out.normal);

    var index: u32 = 0u;
    for (var i: u32 = 0u; i < numOfCascades; i = i + 1u) {
        if abs(out.viewSpacePos.z) < lightSpaceTrans[i].farZ {
            index = i;
        	break;
        }
    }
    // if length(out.viewSpacePos) > ElapsedTime { index = 1;}
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    out.shadowIdx = index;
    out.materialProps = objectTranformation.materialProps;
    out.userSpecular = objectTranformation.metallicness;
    return out;
}



@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    var pos = in.position;
    let transparency = textureSample(diffuse_map, textureSampler, in.uv).a;
    if objectTranformation.useTexture == 1 && transparency < 0.001 {
       discard;
    }
    pos.z += 0.001;
    return pos;
}
