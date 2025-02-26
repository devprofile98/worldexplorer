

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
    @builtin(instance_index) instance_index: u32
};

struct VSOutput {
    @builtin(position) position: vec4f,
};

struct OffsetData {
    transformation: mat4x4f, // Array of 10 offset vectors
};


@group(0) @binding(0) var<uniform> scene: Scene;
@group(0) @binding(1) var<storage, read> offsetInstance: array<OffsetData>;
//@group(0) @binding(1) var<uniform> lightSpaceTrans: vec3f;

@vertex
fn vs_main(vertex: Vertex) -> VSOutput {

    var world_position: vec4f;
    //let off_id: u32 = objectTranformation.offsetId * 100000;
    if vertex.instance_index == 0 {
        world_position = scene.projection * scene.view * vec4f(vertex.position, 1.0);
    	//out.normal = (objectTranformation.transformations * vec4(in.normal, 0.0)).xyz;
    }else{
        world_position = offsetInstance[vertex.instance_index].transformation * vec4f(vertex.position, 1.0);
    	//out.normal = (offsetInstance[instance_index+ off_id].transformation * vec4(in.normal, 0.0)).xyz;
    }
    var vsOut: VSOutput;
    vsOut.position = scene.projection * scene.view * world_position;
    return vsOut;
}
