

struct Scene {
    projection: mat4x4f,
    model: mat4x4f,
};

struct Vertex {
    @location(0) position: vec3f,
};

struct VSOutput {
    @builtin(position) position: vec4f,
};


@group(0) @binding(0) var<uniform> scene: Scene;
// @group(0) @binding(1) var<uniform> model: mat4x4f;


@vertex
fn vs_main(vertex: Vertex) -> VSOutput {
    var vsOut: VSOutput;
    vsOut.position = scene.projection * scene.model * vec4f(vertex.position, 1.0);
    return vsOut;
}
