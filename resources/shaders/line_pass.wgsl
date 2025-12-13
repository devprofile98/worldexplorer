struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
};

struct Line {
    p: vec4<f32>,
    color: vec3<f32>,
    tid: u32,
};

struct Transformation {
    trans: mat4x4f,
};

struct VertexInput {
    @location(0) position: vec2f,
};

struct MyUniform {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    cameraWorldPosition: vec3f,
    time: f32,
};

@group(0) @binding(0) var<storage, read> lines : array<Line>;
@group(0) @binding(1) var<storage, read> transformations : array<Transformation>;

@group(1) @binding(0) var<uniform> viewProjection : MyUniform;


@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    out.color = vec4(lines[instance_index].color, 1.0);

    let pointA = lines[instance_index].p.xyz;
    let pointB = mix(lines[instance_index + 1u].p.xyz, pointA, lines[instance_index].p.w);

    // Interpolate in 3D space first (correct for perspective)
    let point = mix(pointA, pointB, in.position.x);
    let line_transform = transformations[lines[instance_index].tid].trans;
    var center_clip = viewProjection.projectionMatrix * viewProjection.viewMatrix * line_transform * vec4f(point, 1.0);

    // Project endpoints to clip space (for direction calculation)
    let clipPosA = viewProjection.projectionMatrix * viewProjection.viewMatrix * line_transform * vec4f(pointA, 1.0);
    let clipPosB = viewProjection.projectionMatrix * viewProjection.viewMatrix * line_transform * vec4f(pointB, 1.0);

    // NDC positions (post-perspective divide)
    let ndcA = clipPosA.xy / clipPosA.w;
    let ndcB = clipPosB.xy / clipPosB.w;

    // Convert direction to pixel space for aspect-correct normalization
    // Viewport uniform: e.g., vec2f(1920.0, 1080.0)
    let viewport_size = vec2f(1920.0, 1080.0);  // Assume this uniform exists; add if needed
    let pixel_dir = (ndcB - ndcA) * (viewport_size / 2.0);
    let norm_pixel_dir = normalize(pixel_dir);
    let perp_pixel = vec2f(-norm_pixel_dir.y, norm_pixel_dir.x);

    // Desired full line width in pixels (e.g., 2.0)
    let line_width_pixels = 5.0;
    let half_width_pixels = line_width_pixels / 2.0;

    // Offset in pixel space, then convert to NDC
    let offset_pixels = perp_pixel * half_width_pixels * in.position.y;
    let offset_ndc = offset_pixels * (2.0 / viewport_size);  // Pixel to NDC scale

    // Apply offset in clip space: scale by w for constant NDC delta
    center_clip.x += offset_ndc.x * center_clip.w;
    center_clip.y += offset_ndc.y * center_clip.w;

    out.position = center_clip;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    //return vec4f(0.8, 0.4, 0.0, 1.0);
    return in.color;
}
