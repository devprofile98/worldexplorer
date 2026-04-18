struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
};

@group(0) @binding(0) var textureSampler: sampler;
@group(0) @binding(1) var texture: texture_depth_2d;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4f(in.position, 1.0);
    out.uv = in.uv;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let uv = vec2f(in.uv.x, 1.0 - in.uv.y);
    let texel = textureSample(texture, textureSampler, uv);

    return vec4f(vec3f(texel), 1.0);
}
