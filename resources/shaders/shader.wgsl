#include "common.wgsl"

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var<uniform> clipping_plane: vec4f;

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

    //@location(6) boneIds: vec4i,
    //@location(7) boneWeights: vec4f,
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

    //let skinned_position = bonesFinalTransform[in.boneIds[0]] * vec4f(in.position, 1.0) * 1.0; // + bonesFinalTransform[in.boneIds[1]] * vec4f(in.position, 1.0) * in.boneWeights[1] + bonesFinalTransform[in.boneIds[2]] * vec4f(in.position, 1.0) * in.boneWeights[2] + bonesFinalTransform[in.boneIds[3]] * vec4f(in.position, 1.0) * in.boneWeights[3];
    //let world_position = transform * skinned_position;

    var bone_matrix = mat4x4<f32>(
        vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 0
        vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 1
        vec4<f32>(0.0, 0.0, 0.0, 0.0), // Column 2
        vec4<f32>(0.0, 0.0, 0.0, 0.0)  // Column 3
    );

    bone_matrix += bonesFinalTransform[in.boneIds.x] * normalize(in.boneWeights).x;
    bone_matrix += bonesFinalTransform[in.boneIds.y] * normalize(in.boneWeights).y;
    bone_matrix += bonesFinalTransform[in.boneIds.z] * normalize(in.boneWeights).z;
    bone_matrix += bonesFinalTransform[in.boneIds.w] * normalize(in.boneWeights).w;

    let world_position = transform * bone_matrix * vec4f(in.position, 1.0);

    out.normal = (transform * vec4f(in.normal, 0.0)).xyz;

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

fn calculateShadow(fragPosLightSpace: vec4f, distance: f32, cascadeIdx: u32) -> f32 {

    var projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = vec3(
        projCoords.xy * vec2(0.5, -0.5) + vec2(0.5),
        projCoords.z
    );

    var shadow = 0.0;
    for (var i: i32 = -1; i <= 1; i++) {
        for (var j: i32 = -1; j <= 1; j++) {
            shadow += textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), cascadeIdx, projCoords.z - 0.005);
        }
    }
    shadow /= 9.0;
    return shadow;
}

fn calculatePointLight(curr_light: PointLight, normal: vec3f, dir: vec3f) -> vec3f {
    let distance = length(dir);
    let attenuation = 1.0 / (curr_light.constant + curr_light.linear * distance + curr_light.quadratic * (distance * distance));
    let light_dir = normalize(dir);
    return curr_light.ambient.xyz * attenuation * max(dot(normal, light_dir), 0.0);
}

fn calculateSpotLight(currLight: PointLight, normal: vec3f, dir: vec3f) -> vec3f {
    let lightDir = normalize(dir);
    let diff = max(dot(normal, lightDir), 0.0);
    let diffuse = currLight.diffuse.xyz * diff;
    let theta = dot(lightDir, normalize(-currLight.direction).xyz);
    let epsilon = (currLight.cutOff - currLight.outerCutOff);
    let intensity = clamp((theta - currLight.outerCutOff) / epsilon, 0.0, 1.0);
    let distance = length(dir);
    let attenuation = 1.0 / (1.0 + currLight.linear * distance + currLight.quadratic * distance * distance);

    return diffuse * intensity * attenuation;
}

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

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    //let d = in.worldPos.z + 3.5;
    let d = dot(in.worldPos, clipping_plane.xyz) + clipping_plane.w;
    if d > 0.0 {
	discard;
    }

    var frag_ambient = textureSample(diffuse_map, textureSampler, in.uv).rgba;
    if (in.materialProps & (1u << 0u)) != 1u {
        frag_ambient = vec4f(in.color.rgb, 1.0);
    }
    if frag_ambient.a < 0.001 {
		discard;
    }
    let albedo = pow(frag_ambient.rgb, vec3f(1.5f));
    let material = textureSample(metalic_roughness_texture, textureSampler, in.uv).rgb;
    let ao = material.r;
    let roughness = material.g;
    var metallic = material.b;


    var normal = textureSample(normal_map, textureSampler, in.uv).rgb;
    normal = normal * 2.0 - 1.0;
    let TBN = mat3x3f(normalize(in.tangent), normalize(in.biTangent), normalize(in.normal));
    var N = normalize(TBN * normal);
    let V = normalize(in.viewDirection);

    if (in.materialProps & (1u << 1u)) != 2u {
        N = in.aNormal;
    }

    if (in.materialProps & (1u << 3u)) != 8u {
        metallic = in.userSpecular;
    }

    var F0 = vec3f(0.04f); //base reflectivity: default to 0.04 for even dieelectric material
    F0 = mix(F0, albedo, metallic);

    var lo: vec3f = vec3f(0.0);
    for (var i = 0u; i < 2; i += 1u) {
        let curr_light = pointLight[i];
        let L = normalize(curr_light.position.xyz - in.worldPos);
        let H = normalize(V + L);
        let distance = length(curr_light.position.xyz - in.worldPos);
        let attenuation = 1.0f / (distance * distance);

        let radiance = curr_light.ambient.rgb * attenuation;

        let NDF = distributionGGX(N, H, roughness);
        let G = geometrySmith(N, V, L, roughness);
        let F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        let numerator = NDF * G * F;
        let denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        let specular = numerator / denominator;


        let kS = F;

        var kD = vec3f(1.0) - kS;
        kD = kD * (1.0 - metallic);

        let NdotL = max(dot(N, L), 0.0);

        lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

        {
        let curr_light = lightingInfos.colors[0].rgb;
        let L = normalize(lightingInfos.directions[0].xyz); // normalize(curr_light.position.xyz - in.worldPos);
        let H = normalize(V + L);

        let radiance = lightingInfos.colors[0].rgb;

        let NDF = distributionGGX(N, H, roughness);
        let G = geometrySmith(N, V, L, roughness);
        let F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        let numerator = NDF * G * F;
        let denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        let specular = numerator / denominator;


        let kS = F;

        var kD = vec3f(1.0) - kS;
        kD = kD * (1.0 - metallic);

        let NdotL = max(dot(N, L), 0.0);

        lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    let ambient = vec3(0.03) * albedo * ao;

    var color = ambient + lo;

	    // HDR tonemapping
    color = color / (color + vec3f(1.0));
	    // gamma correct
    color = pow(color, vec3(1.1));
    return vec4f(color, 1.0);
}
