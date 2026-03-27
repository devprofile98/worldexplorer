
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) texCoord: vec2f,
    @location(1) particleColor: vec4f,
    @location(2) uv: vec2f,
};


struct VertexInput {
    @location(0) position: vec3f,
    @location(1) uv: vec2f,
};


struct ViewUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,
    cameraWorldPosition: vec3f,
    time: f32,
};


struct InstanceData {
    transformation: mat4x4f,
    props: vec4f,
    uvoffset: vec2f,
};

@group(0) @binding(0) var<uniform> uViewUniforms: array<ViewUniforms, 10>;
@group(0) @binding(1) var<storage, read> uInstancingInfo: array<InstanceData>;
@group(0) @binding(2) var textureSampler: sampler;
@group(0) @binding(3) var particleTexture: texture_2d<f32>;

@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    let scale: f32 = 10.0;

    var out: VertexOutput;

    let instance = uInstancingInfo[instance_index];
    out.position = uViewUniforms[0].modelMatrix * instance.transformation * vec4((in.position.xyz), 1.0f);
    out.texCoord = vec2f(0.0, 0.0);
    out.particleColor = instance.props.rgba;
    out.uv = in.uv + instance.uvoffset;
    return out;
}


@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    //var frag_ambient = textureSample(diffuse_map, textureSampler, uv).rgba;
    //color = (texture(sprite, TexCoords) * ParticleColor);
    let color = textureSample(particleTexture, textureSampler, in.uv) * in.particleColor;
    return color;
    //return in.particleColor;
}

