
#include "common.wgsl"

//@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;


@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var<uniform> clipping_plane: vec4f;

@group(5) @binding(0) var<storage, read> visible_instances_indices: array<u32>;

@group(6) @binding(0) var grass_ground_texture2: texture_2d_array<f32>;
@group(6) @binding(1) var rock_mountain_texture2: texture_2d_array<f32>;
@group(6) @binding(2) var sand_lake_texture2: texture_2d_array<f32>;
@group(6) @binding(3) var snow_mountain_texture2: texture_2d_array<f32>;


const PI: f32 = 3.141592653589793;


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
    out.shadowIdx = index;
    return out;
}

fn calculateShadow(fragPosLightSpace: vec4f, distance: f32, shadowIdx: u32) -> f32 {


    var projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = vec3(
        projCoords.xy * vec2(0.5, -0.5) + vec2(0.5),
        projCoords.z
    );

    var shadow = 0.0;
    for (var i: i32 = -1; i <= 1; i++) {
        for (var j: i32 = -1; j <= 1; j++) {
            shadow += textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), shadowIdx, projCoords.z);
        }
    }
    shadow /= 9.0;
    return shadow;
}


fn calculateTerrainColor(level: f32, uv: vec2f, index: i32) -> vec3f {
    var color = vec3f(0.0f, 1.0f, 0.0f);
    if level <= 1.1 && level >= 0.9 {
        let texture_col = textureSample(sand_lake_texture2, textureSampler, uv, index).rgb;
        color = mix(texture_col, vec3f(1.0, 0.0, 0.0), 0.05);
    } else if level >= 1.1 && level < 1.9 {
        let distance = level - 1.0;
        let grass_color = textureSample(grass_ground_texture2, textureSampler, uv * 0.4, index).rgb;
        color = mix(textureSample(sand_lake_texture2, textureSampler, uv, index).rgb, vec3f(1.0, 0.0, 0.0), 0.05);
        color = mix(color, grass_color, distance);
    } else if level >= 1.9 && level < 2.1 {
        color = textureSample(grass_ground_texture2, textureSampler, uv * 0.4, index).rgb;
    } else if level >= 2.1 && level < 2.9 {
        let distance = level - 2.0;
        let grass_color = textureSample(grass_ground_texture2, textureSampler, uv * 0.4, index).rgb;
        color = textureSample(rock_mountain_texture2, textureSampler, uv * 0.2, index).rgb;
        color = mix(grass_color, color, distance);
    } else if level >= 2.9 && level < 3.1 {
        color = textureSample(rock_mountain_texture2, textureSampler, uv * 0.2, index).rgb;
    } else if level >= 3.1 && level < 3.9 {
        let distance = level - 3.0;
        let snow_color = textureSample(snow_mountain_texture2, textureSampler, uv, index).rgb;
        color = textureSample(rock_mountain_texture2, textureSampler, uv * 0.2, index).rgb;
        color = mix(color, snow_color, distance);
    } else if level >= 3.9 {
        color = textureSample(snow_mountain_texture2, textureSampler, uv, index).rgb;
    }
    return color;
}

fn calculatePointLight(light: PointLight, N: vec3f, V: vec3f, pos: vec3f, albedo: vec3f, metallic_roughness: f32) -> vec3f {
    let L = normalize(light.position.xyz - pos);
    let distance = length(abs(light.position.xyz - pos));
    let attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    let radiance = light.ambient.rgb * attenuation ; // Scale intensity

    // Diffuse term
    let NdotL = max(dot(N, L), 0.0);
    let diffuse = albedo / PI * NdotL * radiance;

    // Specular term (Phong)
    let reflection = reflect(-L, N);
    let shininess = mix(8.0, 128.0, 1.0 - metallic_roughness); // Higher shininess range
    let specular_factor = pow(max(0.0, dot(reflection, V)), shininess);
    let specular_norm = (shininess + 2.0) / (2.0 * PI); // Normalize for energy conservation
    let specular = radiance * specular_factor * specular_norm;

    return diffuse + specular;
}

fn calculateSpotLight(light: PointLight, N: vec3f, V: vec3f, pos: vec3f, albedo: vec3f, metallic_roughness: f32) -> vec3f {
    let L = normalize(light.position.xyz - pos);
    let distance = length(light.position.xyz - pos);
    let attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Spotlight intensity based on cone
    let theta = dot(L, normalize(-light.direction.xyz));
    let epsilon = light.cutOff - light.outerCutOff;
    let intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    let radiance = light.ambient.rgb * attenuation * intensity;

    // Diffuse term
    let NdotL = max(dot(N, L), 0.0);
    let diffuse = albedo / 3.1415926535 * NdotL * radiance;

    // Specular term (Phong)
    let reflection = reflect(-L, N);
    let shininess = mix(8.0, 128.0, 1.0 - metallic_roughness);
    let specular_factor = pow(max(0.0, dot(reflection, V)), shininess);
    let specular_norm = (shininess + 2.0) / (2.0 * 3.1415926535);
    let specular = radiance * specular_factor * specular_norm;

    return diffuse + specular;
}
//fn calculateSpotLight(currLight: PointLight, normal: vec3f, dir: vec3f) -> vec3f {
//    let lightDir = normalize(dir);
//    let diff = max(dot(normal, lightDir), 0.0);
//    let diffuse = currLight.ambient.xyz * diff;
//    let theta = dot(lightDir, normalize(-currLight.direction).xyz);
//    let epsilon = (currLight.cutOff - currLight.outerCutOff);
//    let intensity = clamp((theta - currLight.outerCutOff) / epsilon, 0.0, 1.0);
//    let distance = length(dir);
//    let attenuation = 1.0 / (1.0 + currLight.linear * distance + currLight.quadratic * distance * distance);
//
//    return diffuse * intensity * attenuation;
//}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let d = dot(in.worldPos, clipping_plane.xyz) + clipping_plane.w;
    if d > 0.0 {
	    discard;
    }

    var normal = normalize(calculateTerrainColor(in.color.r, in.uv, 1));
    normal = normal * 2.0 - 1.0;
    let TBN = mat3x3f(normalize(in.tangent), normalize(in.biTangent), normalize(in.normal));
    normal = normalize(TBN * normal);


    let view_direction = normalize(in.viewDirection);


    var shading = vec3f(0.0);


    ///////////////////////// Ambient ///////////////////////////////
    let terrain_color = calculateTerrainColor(in.color.r, in.uv, 0);
    let ambient = terrain_color * vec3f(0.1, 0.1, 0.2);

    let metallic_roughness = calculateTerrainColor(in.color.r, in.uv, 2).b;


    var sun_color = lightingInfos.colors[0].rgb;
    let sun_direction = normalize(lightingInfos.directions[0].xyz);
    let diffuse_factor = max(0, dot(sun_direction, normal));

    let sun_diffuse = terrain_color * sun_color * diffuse_factor;

    //let reflection = reflect(-sun_direction, normal);
    //let shininess = mix(0.0, 16.0, metallic_roughness);
    //let shininess = mix(8.0, 128.0, 1.0 - metallic_roughness); // Inverse roughness, higher range
    //let specular_factor = pow(max(0, dot(reflection, view_direction)), shininess);
    //let specular_color = sun_color * specular_factor;

    let reflection = reflect(-normalize(sun_direction), normalize(normal));
    let shininess = mix(8.0, 128.0, 1.0 - metallic_roughness); // Inverse roughness, higher range
    let specular_factor = pow(max(0.0, dot(reflection, normalize(view_direction))), shininess);
    // Normalize specular to prevent excessive brightness
    let specular_norm = (shininess + 2.0) / (2.0 * PI); // Normalization factor for Phong
    let specular_color = sun_color * specular_factor * specular_norm * 0.1; // Scale down intensity


    //let RoV = max(0.0, dot(sun_reflection, view_direction));
    //let hardness = metallic_roughness;
    //let specular = pow(RoV, hardness) ;
    //let intensity = dot(direction, normal);
    //var min_intensity = 0.6;

    //let diffuse = color;// max(min_intensity, intensity) * color;
    //shading += diffuse;

    //shading += specular ;


    var point_light_color = vec3f(0.0f);
    for (var i: i32 = 0; i < 5 ; i++) {
        let curr_light = pointLight[i];
        let dir = curr_light.position.xyz - in.worldPos;

        if curr_light.ftype == 3 {
            point_light_color += calculatePointLight(curr_light, normal, view_direction, in.worldPos, terrain_color, metallic_roughness);
        } else if curr_light.ftype == 2 {
            point_light_color += calculateSpotLight(curr_light, normal, view_direction, in.worldPos, terrain_color, metallic_roughness);
        }
    }

    let shadow = calculateShadow(in.shadowPos, length(in.viewSpacePos), in.shadowIdx);

    var color = ambient + sun_diffuse + specular_color + point_light_color;

    color = color / (color + vec3f(1.1)); // HDR tonemapping
    color = pow(color, vec3(1.3)); // gamma correct

    return vec4f(color * (1 - shadow * (0.75)), 1.0);
    //return vec4(calculateSpotLight(pointLight[4], normal, pointLight[4].position.xyz - in.worldPos).xyz, 1.0);
}
