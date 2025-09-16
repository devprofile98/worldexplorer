struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct Line {
    p1: vec4<f32>,
    p2: vec4<f32>
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
@group(1) @binding(0) var<uniform> viewProjection : MyUniform;


@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;

    //let pointA = vec3f(0.0, 0.0, 0.0);
    //let pointB = vec3f(10.0, 10.0, 0.0);


    let pointA = lines[instance_index].p1.xyz;
    let pointB = lines[instance_index].p2.xyz;

    let xBasis = pointB - pointA;
    let xBasis_normalized = normalize(xBasis.xy);
    let yBasis = vec3f(-xBasis_normalized.y, xBasis_normalized.x, 0.0);
    let point = pointA + xBasis * in.position.x + yBasis * 0.5f * in.position.y;

    out.position = viewProjection.projectionMatrix * viewProjection.viewMatrix * vec4f(point.x, point.y, point.z, 1.0f);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(1.0, 0.0, 0.0, 1.0);
}
