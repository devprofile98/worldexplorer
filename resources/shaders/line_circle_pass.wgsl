struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: f32,
};

struct VertexInput {
    @location(0) position: vec3<f32>,
};

struct Line {
    p: vec4<f32>,
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
@group(1) @binding(0) var<uniform> viewProjection : MyUniform;


@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;

    let model_position: vec4<f32> = vec4<f32>(in.position.xyz + lines[instance_index].p.xyz, 1.0);

    // Apply the view-projection matrix to get the final clip-space position
    out.position = viewProjection.projectionMatrix * viewProjection.viewMatrix * model_position;
    out.color = in.position.x + in.position.y;

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(0.5, 0.0, 1.0, 1.0);
}
