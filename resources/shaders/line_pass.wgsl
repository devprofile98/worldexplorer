struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct Line {
    p: vec4<f32>,
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

    let pointA = lines[instance_index].p.xyz;
    var pointB = lines[instance_index + 1u].p.xyz;
    let flag = lines[instance_index].p.w;
    pointB = mix(pointB, pointA, flag);

    let xBasis = pointB - pointA;
    let xBasis_normalized = normalize(xBasis.xy);
    //let yBasis = vec3f(-xBasis_normalized.y, xBasis_normalized.x, 0.0);
    let yBasis = normalize(cross((viewProjection.cameraWorldPosition - pointA), xBasis));
    let point = pointA + xBasis * in.position.x + yBasis * 0.1f * in.position.y;

    var d = vec4f(point.x, point.y, point.z, 1.0f);


    out.position = viewProjection.projectionMatrix * viewProjection.viewMatrix * d;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(0.5, 0.0, 1.0, 1.0);
}
