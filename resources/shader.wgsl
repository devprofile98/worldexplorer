struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(5) shadowPos: vec4f,
    @location(0) color: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) viewDirection: vec3f,
    @location(4) worldPos: vec3f,
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
    padding1: i32,
    padding2: i32,
    padding3: i32,
}


struct PointLight {
    position: vec4f,
    ambient: vec4f,
    diffuse: vec4f,
    specular: vec4f,

    constant: f32,
    linear: f32,
    quadratic: f32,
}

struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
    view: mat4x4f,
};

struct OffsetData {
    offsets: array<vec4f, 10>, // Array of 10 offset vectors
};


@group(0) @binding(0) var<uniform> uMyUniform: MyUniform;
@group(0) @binding(1) var diffuse_map: texture_2d<f32>;
@group(0) @binding(2) var textureSampler: sampler;
@group(0) @binding(3) var<uniform> lightingInfos: LightingUniforms;
@group(0) @binding(4) var<uniform> pointLight: PointLight;
@group(0) @binding(5) var metalic_roughness_texture: texture_2d<f32>;
@group(0) @binding(6) var grass_ground_texture: texture_2d<f32>;
@group(0) @binding(7) var rock_mountain_texture: texture_2d<f32>;
@group(0) @binding(8) var sand_lake_texture: texture_2d<f32>;
@group(0) @binding(9) var snow_mountain_texture: texture_2d<f32>;
@group(0) @binding(10) var depth_texture: texture_depth_2d;
@group(0) @binding(11) var<uniform> lightSpaceTrans: Scene;
@group(0) @binding(12) var shadowMapSampler: sampler_comparison;
@group(0) @binding(13) var<uniform> offsetData: OffsetData;


@group(1) @binding(0) var<uniform> objectTranformation: ObjectInfo;

const PI: f32 = 3.141592653589793;

fn degreeToRadians(degrees: f32) -> f32 {
    return degrees * (PI / 180.0); // Convert 90 degrees to radians
    // Rest of your shader code
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
    let world_position = objectTranformation.transformations * vec4f(in.position + offsetData.offsets[instance_index].xyz, 1.0);
    let shadow_position = lightSpaceTrans.projection * lightSpaceTrans.view * objectTranformation.transformations * vec4f(in.position, 1.0);
    out.position = uMyUniform.projectionMatrix * uMyUniform.viewMatrix * world_position;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform.cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.normal = (objectTranformation.transformations * vec4(in.normal, 0.0)).xyz;
    out.uv = in.uv;
    out.shadowPos = shadow_position;
    return out;
}


fn calculateShadow(fragPosLightSpace: vec4f) -> f32 {
    var projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // projCoords = projCoords * 0.5 + 0.5;

    projCoords = vec3(
        projCoords.xy * vec2(0.5, -0.5) + vec2(0.5),
        projCoords.z
    );

    var shadow = 0.0;
    for (var i: i32 = -1; i <= 1; i++) {
        for (var j: i32 = -1; j <= 1; j++) {
            let closestDepth = textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy + vec2(f32(i), f32(j)) * vec2(0.00048828125, 0.00048828125), projCoords.z);
            shadow += closestDepth;
        }
    }
    shadow /= 9.0;
    // let closestDepth = textureSampleCompare(depth_texture, shadowMapSampler, projCoords.xy, projCoords.z);
    // return closestDepth;
    return shadow;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {


    let normal = normalize(in.normal);

    let view_direction = normalize(in.viewDirection);
    let metallic_roughness = textureSample(metalic_roughness_texture, textureSampler, in.uv).rgb;

    var shading = vec3f(0.0);
    // for (var i = 0; i < 1; i++) {
    var color = lightingInfos.colors[0].rgb;
    let direction = normalize(lightingInfos.directions[0].xyz);
    let reflection = reflect(-direction, normal);
        // The closer the cosine is to 1.0, the closer V is to R
    let RoV = max(0.0, dot(reflection, view_direction));
    let hardness = 16.0;
    let specular = pow(RoV, hardness) ;

    let intensity = dot(direction, normal);
    let diffuse = max(0.2, intensity) * color;
    // let diffuse = abs(intensity) * color;
    shading += diffuse;
    if objectTranformation.isFlat == 0 {
        shading += specular / 1.0;
    }

    let distance_dir = pointLight.position.xyz - in.worldPos;
    let distance = length(pointLight.position.xyz - in.worldPos);
    let attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
    let light_dir = normalize(pointLight.position.xyz - in.worldPos);
    let diff = max(dot(normal, light_dir), 0.0);

    if objectTranformation.isFlat == 0 {
        let fragment_color = textureSample(diffuse_map, textureSampler, in.uv).rgba;
        var base_diffuse = fragment_color.rgb;
        let color2 = shading * base_diffuse;
        let linear_color = pow(color2, vec3f(2.2));
        let ambient = linear_color;
        let diffuse = pointLight.ambient.xyz * attenuation * color * diff;
        color = vec4f(ambient + diffuse, 1.0).rgb;
    } else {
        if in.color.r == 1 {
            color = textureSample(sand_lake_texture, textureSampler, in.uv).rgb;
        } else if in.color.r > 1 && in.color.r < 2 {
            let distance = in.color.r - 1.0;
            let grass_color = textureSample(grass_ground_texture, textureSampler, in.uv).rgb;
            color = textureSample(sand_lake_texture, textureSampler, in.uv).rgb;
            color = mix(color, grass_color, distance);
        } else if in.color.r == 2 {
            color = textureSample(grass_ground_texture, textureSampler, in.uv).rgb;
        } else if in.color.r > 2 && in.color.r < 3 {
            let distance = in.color.r - 2.0;
            let grass_color = textureSample(grass_ground_texture, textureSampler, in.uv).rgb;
            color = textureSample(rock_mountain_texture, textureSampler, in.uv).rgb;
            color = mix(grass_color, color, distance);
        } else if in.color.r == 3 {
            color = textureSample(rock_mountain_texture, textureSampler, in.uv).rgb;
        } else if in.color.r > 3 && in.color.r < 4 {
            let distance = in.color.r - 3.0;
            let snow_color = textureSample(snow_mountain_texture, textureSampler, in.uv).rgb;
            color = textureSample(rock_mountain_texture, textureSampler, in.uv).rgb;
            color = mix(color, snow_color, distance);
        } else if in.color.r == 4 {
            color = textureSample(snow_mountain_texture, textureSampler, in.uv).rgb;
        }
    }
    let shadow = calculateShadow(in.shadowPos);

    let ambient = color * shading;
    let diffuse_final = pointLight.ambient.xyz * attenuation * diff;

    return vec4f((diffuse_final + ambient) * (1 - shadow / 2), 1.0);
}
