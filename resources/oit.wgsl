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
    @location(2) uv: vec2f,
};

struct Heads {
  numFragments: atomic<u32>,
  data: array<atomic<u32>>
};

struct LinkedListElement {
  color: u32,
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

struct OffsetData {
    offsets: vec4f, // Array of 10 offset vectors
};


@binding(0) @group(0) var<uniform> uniforms: MyUniform;
@binding(1) @group(0) var<storage, read_write> heads: Heads;
@binding(2) @group(0) var<storage, read_write> linkedList: LinkedList;
@binding(3) @group(0) var opaqueDepthTexture: texture_depth_2d;
@binding(4) @group(0) var<uniform> objectTranformation: ObjectInfo;
@binding(5) @group(0) var diffuseTexture: texture_2d<f32>;
@binding(6) @group(0) var textureSampler: sampler;
//@binding(7) @group(0) var<uniform> offsetData: OffsetData;
@group(0) @binding(7) var<storage, read> offsetInstance: array<OffsetData>;

@vertex
fn vs_main(in: VertexInput, @builtin(instance_index) instance_index: u32) -> VertexOutput {
    var out: VertexOutput;
    let world_position = objectTranformation.transformations * vec4f(in.position + offsetInstance[instance_index].offsets.xyz, 1.0);
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * world_position;
    out.uv = in.uv;
    return out;
} 

fn rgbaToU32(color: vec4f) -> u32 {
    // Scale each component from [0.0, 1.0] to [0, 255] and cast to u32
    let r = u32(color.r * 255.0);
    let g = u32(color.g * 255.0);
    let b = u32(color.b * 255.0);
    let a = u32(color.a * 255.0);

    // Pack the 8-bit values into a single u32
    return (a << 24) | (b << 16) | (g << 8) | r;
}

fn isEven(index: u32) -> vec4f {
    if index % 2 == 0 {
        return vec4f(0.0, 0.0, 1.0, 0.3);
    } else {return vec4f(0.0, 0.0, 1.0, 0.3);}
}

@fragment
fn fs_main(in: VertexOutput) {
    let fragCoords = vec2i(in.position.xy);
    let opaqueDepth = textureLoad(opaqueDepthTexture, fragCoords, 0);
    let fragmentColor = textureSample(diffuseTexture, textureSampler, in.uv);
    var color = vec4f(fragmentColor.rgba);
    
   //var color = vec4f(fragmentColor.aaa / 255.0, 1.0);

  // reject fragments behind opaque objects
    if in.position.z >= opaqueDepth {
    discard;
    }
    let headsIndex = u32(fragCoords.y) * 1920 + u32(fragCoords.x);
    let fragIndex = atomicAdd(&heads.numFragments, 1u);

    if fragIndex < 1920 * 1080 * 4 {
        let lastHead = atomicExchange(&heads.data[headsIndex], fragIndex);
        linkedList.data[fragIndex].depth = in.position.z;
        linkedList.data[fragIndex].next = lastHead;
        //linkedList.data[fragIndex].color = rgbaToU32(isEven(lastHead));
        linkedList.data[fragIndex].color = rgbaToU32(vec4f(color.rgb, color.a));
    }
}


