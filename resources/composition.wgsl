struct Heads {
  numFragments: u32,
  data: array<u32>
};

struct LinkedListElement {
  color: vec4f,
  depth: f32,
  next: u32
};

struct LinkedList {
  data: array<LinkedListElement>
};

// @binding(0) @group(0) var<storage, read_write> heads: Heads;
// @binding(1) @group(0) var<storage, read_write> linkedList: LinkedList;

// Output a full screen quad
@vertex
fn vs_main(@builtin(vertex_index) vertIndex: u32) -> @builtin(position) vec4f {
    var position = array<vec2f, 6>(
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0),
    );

    return vec4(position[vertIndex], 0.0, 1.0);
}


@fragment
fn fs_main(@builtin(position) position: vec4f) -> @location(0) vec4f {
    let fragCoords = vec2i(position.xy);
    if fragCoords.x > 500 && fragCoords.x < 1000 && fragCoords.y > 500 && fragCoords.y < 1000 {
        return vec4f(0.5, 0.0, 0.3, 0.3);
    }
	discard;
}

