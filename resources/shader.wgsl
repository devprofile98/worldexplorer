struct VertexOutput {
    @builtin(position) position: vec4f,
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

// fn getElevationColor(index: i32) -> vec3<f32> {
//     return height_color
// }


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let world_position = objectTranformation.transformations * vec4f(in.position, 1.0);
    out.position = uMyUniform.projectionMatrix * uMyUniform.viewMatrix * world_position;
    out.worldPos = world_position.xyz;
    out.viewDirection = uMyUniform.cameraWorldPosition - world_position.xyz;
    out.color = in.color;
    out.normal = (objectTranformation.transformations * vec4(in.normal, 0.0)).xyz;
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    let normal = normalize(in.normal);

    let view_direction = normalize(in.viewDirection);
    let metallic_roughness = textureSample(metalic_roughness_texture, textureSampler, in.uv).rgb;

    var shading = vec3f(0.0);
    // for (var i = 0; i < 1; i++) {
    let color = lightingInfos.colors[0].rgb;
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
        var color = textureSample(diffuse_map, textureSampler, in.uv).rgb;
        let color2 = shading * color;
        let linear_color = pow(color2, vec3f(2.2));
        let ambient = linear_color;
        let diffuse = pointLight.ambient.xyz * attenuation * color * diff;
        return vec4f(ambient + diffuse, 1.0);
    } else {

        // let height_color: array<vec3<f32>, 5> = array<vec3<f32>, 5>(
        //     vec3<f32>(0.0 / 255.0, 0.0 / 255.0, 139.0 / 255.0), // Deep Water (Dark Blue)
        //     vec3<f32>(0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0), // Shallow Water (Blue)
        //     vec3<f32>(34.0 / 255.0, 139.0 / 255.0, 34.0 / 255.0), // Grassland (Green)
        //     vec3<f32>(139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0), // Mountain (Brown)
        //     vec3<f32>(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0) // Snow (White)
        // );

        var height_color = vec3<f32>(0.0 / 255.0, 139.0 / 255.0, 139.0 / 255.0);
        if in.color.r < 0.5 {
            // height_color = 0;
        } else if in.color.r < 1.5 {
            height_color = vec3<f32>(0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0);
        } else if in.color.r < 2.5 {
            // height_color = vec3<f32>(34.0 / 255.0, 139.0 / 255.0, 34.0 / 255.0);
            height_color = vec3<f32>(255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0);
        } else if in.color.r < 3.5 {
            // height_color = vec3<f32>(139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0);
            height_color = vec3<f32>(255.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0);
        } else if in.color.r < 4.5 {
            height_color = vec3<f32>(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0);
        }
        var color = height_color;
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
        let ambient = color * shading;
        let diffuse = pointLight.ambient.xyz * in.color * attenuation * diff;
        return vec4f(diffuse + ambient, 1.0);
        // return vec4f(color, 1.0);
    }
}