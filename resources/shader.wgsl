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
};

struct OffsetData {
    transformation: mat4x4f, // Array of 10 offset vectors
};


@group(0) @binding(0) var<uniform> uMyUniform: MyUniform;
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
@group(0) @binding(10) var depth_texture: texture_depth_2d;
@group(0) @binding(11) var<uniform> lightSpaceTrans: array<Scene, 2>;
@group(0) @binding(12) var shadowMapSampler: sampler_comparison;
@group(0) @binding(13) var<storage, read> offsetInstance: array<OffsetData>;
@group(0) @binding(14) var<uniform> ElapsedTime: f32;

@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

@group(2) @binding(0) var diffuse_map: texture_2d<f32>;
@group(2) @binding(1) var metalic_roughness_texture: texture_2d<f32>;
@group(2) @binding(2) var normal_map: texture_2d<f32>;

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
    out.normal = (transform * vec4f(in.normal, 0.0)).xyz;

    out.viewSpacePos = uMyUniform.viewMatrix * world_position;
    out.position = uMyUniform.projectionMatrix * out.viewSpacePos;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform.cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.uv = in.uv;

	   let T = normalize(vec3f((transform * vec4(in.tangent,   0.0)).xyz));
	   let B = normalize(vec3f((transform * vec4(in.biTangent, 0.0)).xyz));
	   let N = normalize(vec3f((transform * vec4(in.normal,    0.0)).xyz));



	out.tangent = T;
	out.biTangent = B;
	out.aNormal = out.normal;

    var index = 0;
    if length(out.viewSpacePos) > ElapsedTime { index = 1;}
;
    out.shadowPos = lightSpaceTrans[index].projection * lightSpaceTrans[index].view * world_position;
    //out.shadowPos = shadow_position;
    return out;
}

fn calculateShadow(fragPosLightSpace: vec4f, distance: f32) -> f32 {

    var projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = vec3(
        projCoords.xy * vec2(0.5, -0.5) + vec2(0.5),
        projCoords.z
    );

    var shadow = 0.0;
    for (var i: i32 = -1; i <= 1; i++) {
        for (var j: i32 = -1; j <= 1; j++) {
            var closestDepth = 0.0f;
            if distance < ElapsedTime {
                closestDepth = textureSampleCompare(near_depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), projCoords.z);
            } else {
                closestDepth = textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), projCoords.z);
            }
            shadow += closestDepth;
        }
    }
    shadow /= 9.0;
    return shadow;
}

fn calculateTerrainColor(level: f32, uv: vec2f) -> vec3f {
    var color = vec3f(0.0f);
    if level == 1 {
        color = textureSample(sand_lake_texture, textureSampler, uv).rgb;
    } else if level > 1 && level < 2 {
        let distance = level - 1.0;
        let grass_color = textureSample(grass_ground_texture, textureSampler, uv).rgb;
        color = textureSample(sand_lake_texture, textureSampler, uv).rgb;
        color = mix(color, grass_color, distance);
    } else if level == 2 {
        color = textureSample(grass_ground_texture, textureSampler, uv).rgb;
    } else if level > 2 && level < 3 {
        let distance = level - 2.0;
        let grass_color = textureSample(grass_ground_texture, textureSampler, uv).rgb;
        color = textureSample(rock_mountain_texture, textureSampler, uv * 0.2).rgb;
        color = mix(grass_color, color, distance);
    } else if level == 3 {
        color = textureSample(rock_mountain_texture, textureSampler, uv * 0.2).rgb;
    } else if level > 3 && level < 4 {
        let distance = level - 3.0;
        let snow_color = textureSample(snow_mountain_texture, textureSampler, uv).rgb;
        color = textureSample(rock_mountain_texture, textureSampler, uv * 0.2).rgb;
        color = mix(color, snow_color, distance);
    } else if level == 4 {
        color = textureSample(snow_mountain_texture, textureSampler, uv).rgb;
    }
    return color;
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


@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    //let face_normal = normalize(in.normal);
    var normal = textureSample(normal_map, textureSampler, in.uv).rgb;
    normal = normal * 2.0 - 1.0;
    let TBN = mat3x3f(normalize(in.tangent), normalize(in.biTangent), normalize(in.normal));
    normal = normalize(TBN * normal);
    //normal = normalize(mix(in.normal, normal, 0.1));

    //let normal = normalize(in.normal);

    let view_direction = normalize(in.viewDirection);
    let metallic_roughness = textureSample(metalic_roughness_texture, textureSampler, in.uv).rgb;

    var shading = vec3f(0.0);
    // first, put directional light here
    var color = lightingInfos.colors[0].rgb;
    let direction = normalize(lightingInfos.directions[0].xyz);
    let reflection = reflect(-direction, normal);
        // The closer the cosine is to 1.0, the closer V is to R
    let RoV = max(0.0, dot(reflection, view_direction));
    let hardness = 16.0;
    let specular = pow(RoV, hardness) ;
    let intensity = dot(direction, normal);
    var min_intensity = 0.6;
    if objectTranformation.isFoliage != 1 {
        min_intensity = 0.3;
    }
    let diffuse = max(min_intensity, intensity) * color;
    shading += diffuse + vec3f(0.1, 0.1, 0.15) ;
    if intensity > 0.0 && (objectTranformation.isFlat == 0 || objectTranformation.isFoliage == 1) {
        shading += specular;
    }


    var point_light_color = vec3f(0.0f);
    //for (var i:i32 =0; i < lightCount ; i++) {
    //        let curr_light = pointLight[i];
    //        let dir = curr_light.position.xyz - in.worldPos;

    //        if (curr_light.ftype == 3){
    //    	    point_light_color +=  calculatePointLight(curr_light, normal, dir);
    //        } else if (curr_light.ftype == 2) {
    //        	point_light_color += calculateSpotLight(curr_light, normal, dir);
    //        }
    //}

    if objectTranformation.isFlat == 0 {
        let fragment_color = textureSample(diffuse_map, textureSampler, in.uv).rgba;
        var base_diffuse = fragment_color.rgb;
        let color2 = shading * base_diffuse;
        let linear_color = pow(color2, vec3f(2.2));
        let ambient = linear_color;
        let diffuse = point_light_color * color * point_light_color;

        if objectTranformation.useTexture != 0 {
            color = vec4f(ambient + diffuse, 1.0).rgb;

            if fragment_color.a < 0.1 {
			discard;
            }
        } else {
            color = pow(in.color.rgb , vec3f(2.2));
        }

    	//if objectTranformation.isFoliage == 1 {
	//		color = in.viewDirection;
	//}
    } else {
        color = pow(calculateTerrainColor(in.color.r, in.uv) * color, vec3f(1.9));
    }
    let shadow = calculateShadow(in.shadowPos, length(in.viewSpacePos));

    let ambient = color * shading;
    let diffuse_final = point_light_color;

    var col = (diffuse_final + ambient);

    if objectTranformation.isHovered == 1 {
        let variation = abs(sin(ElapsedTime * 2.0));
        col = col * (vec3f(1.0 + variation * 2.0, 0.0, 0.0));
    }
    //if length(in.viewSpacePos) > ElapsedTime {
    //    return vec4f((col + vec3(1.0, 0.0, 0.5)) * (1 - shadow * (0.75)), 1.0);
    //}else{
    //    return vec4f((col + vec3(0.0, 1.0, 0.5)) * (1 - shadow * (0.75)), 1.0);
    //}
    return vec4f(color * (1 - shadow * (0.75)), 1.0);
}

