struct Uniforms {
    matrix: mat4x4f,
};

struct Vertex {
    @location(0) position: vec3f,
};

struct VSOutput {
    @builtin(position) position: vec4f,
    @location(0) normal: vec3f,
};

@group(0) @binding(0) var<uniform> uni: Uniforms;
@group(0) @binding(1) var ourSampler: sampler;
// @group(0) @binding(2) var ourTexture: texture_2d<f32>;

@vertex
fn vs_main(vert: Vertex) -> VSOutput {
    var vsOut: VSOutput;
    // uni.matrix * 
    vsOut.position = uni.matrix * vec4f(vert.position, 1.0);
    vsOut.normal = normalize(vsOut.position.xyz);
    return vsOut;
}

@fragment
fn fs_main(vsOut: VSOutput) -> @location(0) vec4f {
    // return textureSample(ourTexture, ourSampler, normalize(vsOut.normal));
    return vec4f(vsOut.normal, 1.0);
}