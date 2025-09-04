#include "common.wgsl"

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var water_reflection_texture: texture_2d<f32>;
@group(4) @binding(1) var water_refraction_texture: texture_2d<f32>;

@group(5) @binding(0) var<storage, read> visible_instances_indices: array<u32>;

const PI: f32 = 3.141592653589793;

fn fresnelSchlick(cosTheta: f32, F0: vec3f) -> vec3f {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


fn distributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32 {
    let a = roughness * roughness;
    let a2 = a * a ;
    let NdotH = max(dot(N, H), 0.0);
    let NdotH2 = NdotH * NdotH;

    let nom = a2;
    var denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

fn geometrySchlickGGX(NdotV: f32, roughness: f32) -> f32 {

    let r = (roughness + 1.0);
    let k = (r * r) / 8.0;

    let nom = NdotV;
    let denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

fn geometrySmith(N: vec3f, V: vec3f, L: vec3f, roughness: f32) -> f32 {
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let ggx2 = geometrySchlickGGX(NdotV, roughness);
    let ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx2 * ggx1;
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
    out.normal = -(transform * vec4f(in.normal, 0.0)).xyz;

    out.viewSpacePos = uMyUniform[myuniformindex].viewMatrix * world_position;
    out.position = uMyUniform[myuniformindex].projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform[myuniformindex].cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.uv = in.uv;
    //out.uv = vec2f(in.uv.x, 1 - in.uv.y);

    let T = normalize(vec3f((transform * vec4(in.tangent, 0.0)).xyz));
    let B = normalize(vec3f((transform * vec4(in.biTangent, 0.0)).xyz));
    let N = normalize(vec3f((transform * vec4(in.normal, 0.0)).xyz));

    out.tangent = T;
    out.biTangent = B;
    out.aNormal = out.normal;

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
	// transform world position of the water fragment to reflection camera clip-space
    let reflected_clip_pos = uMyUniform[1].projectionMatrix * uMyUniform[1].viewMatrix * vec4f(in.worldPos, 1.0);
	// perform projection deviding to find ndc
    let ndc_pos = reflected_clip_pos.xyz / reflected_clip_pos.w;
	// ndc to uv 
    let uv_for_reflection = vec2f((ndc_pos.x + 1.0) * 0.5, (ndc_pos.y + 1.0) * 0.5);

    let final_uv = vec2f(uv_for_reflection.x, 1.0 - uv_for_reflection.y);
    let F0 = pow((1.0 - 1.33) / (1.0 + 1.33), 2.0);
    let cos_theta = saturate(dot(normalize(in.viewDirection), normalize(in.normal)));
    let fresnel_co = F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0) ;


    let reflection = textureSample(water_reflection_texture, textureSampler, final_uv).rgba;
    let refraction = textureSample(water_refraction_texture, textureSampler, uv_for_reflection).rgba;
    let greenish_blue = vec4f(0.0, 0.3, 0.2, 1.0);
    return mix(mix(reflection, refraction, 1.0 - fresnel_co), greenish_blue, 0.2);
}
