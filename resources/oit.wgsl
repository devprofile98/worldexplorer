struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
    @location(3) uv: vec2f,
};


struct MyUniform {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
    color: vec4f,    
    cameraWorldPosition: vec3f,
    time: f32,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
};

struct Heads {
  numFragments: atomic<u32>,
  data: array<atomic<u32>>
};

struct LinkedListElement {
  color: vec4f,
  depth: f32,
  next: u32
};

struct LinkedList {
  data: array<LinkedListElement>
};

struct ObjectInfo {
    transformations: mat4x4f,
    isFlat: i32,
    padding1: i32,
    padding2: i32,
    padding3: i32,
}

@binding(0) @group(0) var<uniform> uniforms: MyUniform;
@binding(1) @group(0) var<storage, read_write> heads: Heads;
@binding(2) @group(0) var<storage, read_write> linkedList: LinkedList;
@binding(3) @group(0) var opaqueDepthTexture: texture_depth_2d;
@binding(4) @group(0) var<uniform> objectTranformation: ObjectInfo;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let world_position = objectTranformation.transformations * vec4f(in.position, 1.0);
    //let shadow_position = lightSpaceTrans.projection * lightSpaceTrans.view * objectTranformation.transformations * vec4f(in.position, 1.0);
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * world_position;
    return out;
} 

@fragment
fn fs_main(in: VertexOutput) {
    // return vec4f(1.0, 0.0, 0.0, 0.0);
    let fragCoords = vec2i(in.position.xy);
    let headsIndex = u32(fragCoords.y) * 1920 + u32(fragCoords.x);
    let fragIndex = atomicAdd(&heads.numFragments, 1u);
    if fragIndex < 1920 * 1080 * 2 {
        let lastHead = atomicExchange(&heads.data[headsIndex], fragIndex);
        linkedList.data[fragIndex].depth = in.position.z;
        linkedList.data[fragIndex].next = lastHead;
        linkedList.data[fragIndex].color = vec4(1.0, 1.0, 1.0, 0.3);
    }
}


