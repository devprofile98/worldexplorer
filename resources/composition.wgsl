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

@binding(0) @group(0) var<storage, read_write> heads: Heads;
@binding(1) @group(0) var<storage, read_write> linkedList: LinkedList;

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
    let headsIndex = u32(fragCoords.y) * 1920 + u32(fragCoords.x);

    var layers: array<LinkedListElement, 2>;

    var head = heads.data[headsIndex];
    var numLayers = 0u;

    while head != 0xFFFFFFFFu && numLayers < 2 {
        layers[numLayers] = linkedList.data[head];
        numLayers++;
        head = linkedList.data[head].next;
    }
    if numLayers == 0u {
    discard;
    }
    //if head != 0xFFFFFFFFu {
    //    var color = linkedList.data[head].color;
    //    color = vec4f(1.0, 0.0, 0.0, 0.5);
    //    return color;
    //}
      // blend the remaining layers
    var color = vec4(layers[0].color.a * layers[0].color.rgb, layers[0].color.a);

    for (var i = 1u; i < numLayers; i++) {
        let mixed = mix(color.rgb, layers[i].color.rgb, layers[i].color.aaa);
        color = vec4(mixed, color.a);
    }
    return color;
}

