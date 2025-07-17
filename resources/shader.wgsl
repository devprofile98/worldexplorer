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
    materialProps: u32,
    metallicness: f32,
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
@group(0) @binding(5) var near_depth_texture: texture_2d<f32>;
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
@group(0) @binding(15) var<storage, read> visible_instances_indices: array<u32>;

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

@group(3) @binding(0) var<uniform> myuniformindex: u32;

@group(4) @binding(0) var<uniform> clipping_plane: vec4f;

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
	    let original_instance_idx = visible_instances_indices[instance_index];
        transform = offsetInstance[original_instance_idx + off_id].transformation;
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
        N = in.normal;
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

@fragment
fn fs_main2(in: VertexOutput) -> @location(0) vec4f {
    //let face_normal = normalize(in.normal);
    var normal = textureSample(normal_map, textureSampler, in.uv).rgb;
    normal = normal * 2.0 - 1.0;
    let TBN = mat3x3f(normalize(in.tangent), normalize(in.biTangent), normalize(in.normal));
    normal = normalize(TBN * normal);
    //normal = normalize(mix(in.normal, normal, 0.1));

    //let normal = normalize(in.normal);

    let view_direction = normalize(in.viewDirection);
    let metallic_roughness = textureSample(metalic_roughness_texture, textureSampler, in.uv).rgb;
    let metallic_factor = metallic_roughness.g;

    var shading = vec3f(0.0);
    // first, put directional light here
    var color = lightingInfos.colors[0].rgb;
    let direction = normalize(lightingInfos.directions[0].xyz);
    let reflection = reflect(-direction, normal);
        // The closer the cosine is to 1.0, the closer V is to R
    let RoV = max(0.0, dot(reflection, view_direction));
    let hardness = 32.0;
    let intensity = dot(direction, normal);
    var specular = pow(RoV, hardness) * intensity * color * metallic_factor ;
    var min_intensity = 0.6;
    //if objectTranformation.isFoliage == 1 {
    //    min_intensity = 0.3;
    //}
    let diffuse = max(min_intensity, intensity) * color;
    shading += diffuse + vec3f(0.1, 0.1, 0.15) ;
    if intensity > 0.0 && (objectTranformation.isFlat == 0 || objectTranformation.isFoliage == 1) {
        //shading += specular;
    }


    var point_light_color = vec3f(0.0f);
    for (var i: i32 = 0; i < lightCount ; i++) {
        let curr_light = pointLight[i];
        let dir = curr_light.position.xyz - in.worldPos;

        if curr_light.ftype == 3 {
            point_light_color += calculatePointLight(curr_light, normal, dir);
        } else if curr_light.ftype == 2 {
            point_light_color += calculateSpotLight(curr_light, normal, dir);
        }
    }

    let fragment_color = textureSample(diffuse_map, textureSampler, in.uv).rgba;
	// specular = mix(vec3f(0.04), fragment_color.rgb, metallic_factor);	
        {
        var base_diffuse = fragment_color.rgb;
        let color2 = shading * base_diffuse;
        //let ambient = pow(color2, vec3f(2.2));
        let ambient = color2;
        //let ambient = linear_color;
        let diffuse = point_light_color * color * point_light_color;

        if objectTranformation.useTexture != 0 {
            color = vec4f(ambient + diffuse, 1.0).rgb;

            if fragment_color.a < 0.1 {
			discard;
            }
        } else {
            // color = pow(in.color.rgb, vec3f(2.2));
            color = in.color.rgb;
        }
    }

    let shadow = calculateShadow(in.shadowPos, length(in.viewSpacePos), in.shadowIdx);

    let ambient = (color * (1 - shadow * (0.75))) + specular;
    let diffuse_final = point_light_color;

    var col = (diffuse_final + ambient);

    if objectTranformation.isHovered == 1 {
        let variation = abs(sin(1.0 * 2.0));
        col = col * (vec3f(1.0 + variation * 2.0, 0.0, 0.0));
    }
    //if in.shadowIdx == 0 {
    //    // let c = textureSample(depth_texture, textureSampler, in.uv, 0);
    //    return vec4f(vec3f(1.0,0.0, 0.0) * (1 - shadow * (0.75)), 1.0);
    //} else if in.shadowIdx == 1 {
    //    return vec4f(vec3f(0.0,1.0,0.0) * (1 - shadow * (0.75)), 1.0);
    //}
    // else if in.shadowIdx == 2 {
    //    return vec4f(vec3f(0.0,0.0,1.0) * (1 - shadow * (0.75)), 1.0);
    //}

    col = col / (col + vec3f(1.0));
    // col = pow(col, vec3f(1.0/2.2));
    col = pow(col, vec3f(2.2));
    return vec4f(in.normal, 1.0);
}

