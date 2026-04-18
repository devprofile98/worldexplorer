
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};


struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
};

struct HDRSettings {
    toneMappingActive: u32,
    exposureValue: f32
};

@group(0) @binding(0) var textureSampler: sampler;
@group(0) @binding(1) var hdrTexture: texture_2d<f32>;

@group(1) @binding(0) var<uniform> hdrSettings: HDRSettings;

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
    let hdrColor = textureSample(hdrTexture, textureSampler, uv).rgb;

    if hdrSettings.toneMappingActive == 1u {
    //Reinhard tone mapping
        var ldrColor = vec3f(1.0) - exp(-hdrColor * hdrSettings.exposureValue);
        return vec4f(ldrColor, 1.0);
    }
    return vec4f(hdrColor, 1.0);
}

