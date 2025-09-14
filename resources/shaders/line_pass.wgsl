struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct Line {
    p1: vec4<f32>,
    p2: vec4<f32>
};


struct VertexInput {
    @location(0) position: vec4f,
};

@group(0) @binding(0) var<storage, read> lines : array<Line>;


@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    out.position = in.position;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(1.0, 0.0, 0.0, 1.0);
}
