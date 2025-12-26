#include "common.wgsl"


@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var<uniform> clipping_plane: vec4f;

@group(5) @binding(0) var<storage, read> visible_instances_indices: array<u32>;

@group(6) @binding(0) var<uniform> meshMaterial: Material;
@group(6) @binding(1) var<uniform> meshIdx: i32;

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
    let off_id: u32 = objectTranformation.offsetId * 100000;
    var transform: mat4x4f;

    if instance_index != 0 {
        let original_instance_idx = visible_instances_indices[off_id + instance_index];
        //transform = offsetInstance[original_instance_idx + off_id].transformation;
        transform = offsetInstance[instance_index + off_id].transformation * meshTransformation.global[meshIdx];
    } else {
        // transform = objectTranformation.transformations;
        transform = objectTranformation.transformations * meshTransformation.global[meshIdx];
    }

    var world_position: vec4f;
    if objectTranformation.isAnimated == 0u {

        world_position = transform * vec4f(in.position, 1.0);
        out.normal = (transform * vec4f(in.normal, 0.0f)).xyz;
    } else {

        var skinned_position = vec4f(0.0, 0.0, 0.0, 0.0);
        skinned_position += bonesFinalTransform[in.boneIds.x] * vec4f(in.position, 1.0) * in.boneWeights.x;
        skinned_position += bonesFinalTransform[in.boneIds.y] * vec4f(in.position, 1.0) * in.boneWeights.y;
        skinned_position += bonesFinalTransform[in.boneIds.z] * vec4f(in.position, 1.0) * in.boneWeights.z;
        skinned_position += bonesFinalTransform[in.boneIds.w] * vec4f(in.position, 1.0) * in.boneWeights.w;

        world_position = transform * skinned_position;

        var skinned_normal = vec4f(0.0, 0.0, 0.0, 0.0);
        skinned_normal += bonesFinalTransform[in.boneIds.x] * vec4f(in.normal, 0.0) * in.boneWeights.x;
        skinned_normal += bonesFinalTransform[in.boneIds.y] * vec4f(in.normal, 0.0) * in.boneWeights.y;
        skinned_normal += bonesFinalTransform[in.boneIds.z] * vec4f(in.normal, 0.0) * in.boneWeights.z;
        skinned_normal += bonesFinalTransform[in.boneIds.w] * vec4f(in.normal, 0.0) * in.boneWeights.w;

        out.normal = (transform * skinned_normal).xyz;
    }
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

    var index: u32 = numOfCascades - 1u;
    for (var i: u32 = 0u; i < numOfCascades; i = i + 1u) {
        if abs(out.viewSpacePos.z) < lightSpaceTrans[i].farZ {
            index = i;
        	break;
        }
    }
    //index = 1u;

    // if length(out.viewSpacePos) > ElapsedTime { index = 1;}
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    out.shadowIdx = index;
    out.materialProps = meshMaterial.materialProps;
    out.userSpecular = meshMaterial.metallicness;
    return out;
}

fn calculateShadow(fragPosLightSpace: vec4f, distance: f32, cascadeIdx: u32) -> f32 {

    var projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = vec3(
        projCoords.xy * vec2(0.5, -0.5) + vec2(0.5),
        projCoords.z
    );

    if projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 {
        return 0.0; // No shadow for out-of-bounds
    }

    var shadow = 0.0;
    for (var i: i32 = -1; i <= 1; i++) {
        for (var j: i32 = -1; j <= 1; j++) {
            shadow += textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), cascadeIdx, projCoords.z - 0.0005);
        }
    }
    shadow /= 9.0;
    return shadow;
}

fn calculateSpotLight(light: PointLight, N: vec3f, V: vec3f, pos: vec3f, albedo: vec3f, roughness: f32, metallic: f32, F0: vec3f) -> vec3f {
    if light.ftype != 2i {
        return vec3f(0.0);
    }
    let diff = light.position.xyz - pos;
    let distance = length(diff);
    let L = normalize(diff);
    let H = normalize(V + L);
    let attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    let theta = dot(L, normalize(-light.direction.xyz));
    let epsilon = light.cutOff - light.outerCutOff;
    let intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    let radiance = light.ambient.rgb * attenuation * intensity;
    let NDF = distributionGGX(N, H, roughness);
    let G = geometrySmith(N, V, L, roughness);
    let F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    let numerator = NDF * G * F;
    let denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    let specular = numerator / denominator;
    let kS = F;
    var kD = vec3f(1.0) - kS;
    kD = kD * (1.0 - metallic);
    let NdotL = max(abs(dot(normalize(N), L)), 0.0);
    return (kD * albedo / 3.1415926535 + specular) * radiance * NdotL;
    //if NdotL > 0.001f {
    //   return vec3f(NdotL, NdotL, NdotL);
    //}
    //return L;
}

fn calculatePointLight(light: PointLight, N: vec3f, V: vec3f, pos: vec3f, albedo: vec3f, roughness: f32, metallic: f32, F0: vec3f) -> vec3f {
    // Only process point lights (type == 3)
    if light.ftype != 3i {
        return vec3f(0.0);
    }

    let diff = light.position.xyz - pos;
    let distance = length(diff);
    //if distance > 5.0 {
     //   return vec3f(0.0);
    //}
    let L = normalize(diff);
    let H = normalize(V + L);
    let attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    let radiance = light.ambient.rgb * attenuation;

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

    return (kD * albedo / 3.1415926535 + specular) * radiance * NdotL;
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

    let uv = in.uv * meshMaterial.uvMultiplier.xy;
    let d = dot(in.worldPos, clipping_plane.xyz) + clipping_plane.w;
    if d > 0.0 {
	    discard;
    }

    var frag_ambient = textureSample(diffuse_map, textureSampler, uv).rgba;
    if (in.materialProps & (1u << 0u)) != 1u {
        frag_ambient = vec4f(in.color.rgb, 1.0);
    }
    if frag_ambient.a < 0.001 {
		discard;
    }
    let albedo = pow(frag_ambient.rgb, vec3f(1.5f));
    let material = textureSample(metalic_roughness_texture, textureSampler, uv).rgb;
    let ao = material.r;
    let roughness = material.g;
    var metallic = material.b;


    var normal = textureSample(normal_map, textureSampler, uv).rgb;
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


    ////////////// Calculations for point lights

    for (var i = 0u; i < 5; i += 1u) {
        let light = pointLight[i];
        if light.ftype == 3i {
            //lo += calculatePointLight(light, in.normal, V, in.worldPos, in.albedo, in.roughness, in.metallic, F0);
            lo += calculatePointLight(light, N, V, in.worldPos, albedo, roughness, metallic, F0);
        } else if light.ftype == 2i {
            lo += calculateSpotLight(light, N, V, in.worldPos, albedo, roughness, metallic, F0);
        }
    }

    let shadow = calculateShadow(in.shadowPos, length(in.viewSpacePos), in.shadowIdx);

    ////////////// Calculations for sun light 
        {
        let L = normalize(lightingInfos.directions[0].xyz); // normalize(curr_light.position.xyz - in.worldPos);
        let H = normalize(V + L);

        let radiance = lightingInfos.colors[0].rgb;

        let NDF = distributionGGX(N, H, roughness);
        let G = geometrySmith(N, V, L, roughness);
        let F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        let numerator = NDF * G * F;
        let denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        let specular = numerator / denominator;

        //return vec4f(numerator, 1.0);

        let kS = F;

        var kD = vec3f(1.0) - kS;
        kD = kD * (1.0 - metallic);

        let NdotL = dot(N, L);

        //lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow) ;
        //lo += F * radiance * NdotL * (1.0 - shadow);
        lo += L;
    }

    let ambient = vec3(0.03) * albedo * ao;

    var color = ambient + lo;

	// HDR tonemapping
    color = color / (color + vec3f(1.0));
	// gamma correct
    color = pow(color, vec3(1.1 / 1.0));


    //return vec4f(color, 1.0);
    //let animated = f32(in.materialProps >> 6);
    //if in.shadowIdx == 0u {

    //    return vec4f(1.0, 0.0, 0.0, 1.0);
    //} else if in.shadowIdx == 1u {

    //    return vec4f(0.0, 1.0, 0.0, 1.0);
    //} else {

    //    return vec4f(0.0, 0.0, 1.0, 1.0);
    //}

    //if lo.x < 0.0 || lo.y < 0.0 || lo.z < 0.0 {
    //    return vec4f(0.0, 1.0, 0.0, 1.0);
    //} else {
    //    return vec4f(0.0, 0.0, 1.0, 1.0);
    //}

    return vec4f((ambient + lo), 1.0);
    //return vec4f(in.shadowIdx * 50.0, in.shadowIdx * 50.0, f32(in.shadowIdx) * 50.0, 1.0);
}
