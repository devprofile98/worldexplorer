

struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
    view: mat4x4f,
};

struct Vertex {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) uv: vec2f,
};

struct VSOutput {
    @builtin(position) position: vec4f,
};


@group(0) @binding(0) var<uniform> scene: Scene;


@vertex
fn vs_main(vertex: Vertex) -> VSOutput {
    var vsOut: VSOutput;
    vsOut.position = scene.projection * scene.view * scene.model * vec4f(vertex.position, 1.0);
    return vsOut;
}
