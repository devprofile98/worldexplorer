struct Heads {
  numFragments: u32,
  data: array<u32>
};

struct LinkedListElement {
  color: u32,
  depth: f32,
  next: u32
};

struct LinkedList {
  data: array<LinkedListElement>
};

@binding(0) @group(0) var<storage, read_write> heads: Heads;
@binding(1) @group(0) var<storage, read_write> linkedList: LinkedList;

fn u32ToRgba(packedColor: u32) -> vec4f {
    // Extract each 8-bit component using bitwise operations
    let a = f32((packedColor >> 24) & 0xFF) / 255.0; // Alpha
    let b = f32((packedColor >> 16) & 0xFF) / 255.0; // Blue
    let g = f32((packedColor >> 8) & 0xFF) / 255.0; // Green
    let r = f32(packedColor & 0xFF) / 255.0;         // Red

    // Return as a vec4f
    return vec4f(r, g, b, a);
}

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

    var layers: array<LinkedListElement, 4>;

    var head = heads.data[headsIndex];
    var numLayers = 0u;

    while head != 0xFFFFFFFFu && numLayers < 4 {
        layers[numLayers] = linkedList.data[head];
        numLayers++;
        head = linkedList.data[head].next;
    }
    if numLayers == 0u {
    discard;
    }
         // blend the remaining layers
    let tmp_color = u32ToRgba(layers[0].color);
    var color = vec4(tmp_color.a * tmp_color.rgb, tmp_color.a);

     //sort the fragments by depth

    //for (var i = 1u; i < numLayers; i++) {
    //    let toInsert = layers[i];
    //    var j = i;

    //    while j > 0u && toInsert.depth > layers[j - 1u].depth {
    //        layers[j] = layers[j - 1u];
    //        j--;
    //    }

    //    layers[j] = toInsert;
    //}

    for (var i = 1u; i < numLayers; i++) {
        let tmp_color = u32ToRgba(layers[i].color);
        let mixed = mix(color.rgb, tmp_color.rgb, tmp_color.a);

	//var alpha = 1.0;
	//if color.a > 0.9 {
        //   alpha = 1.0;
	//}else{
	//   alpha = 0.0;
	//}
        color = vec4(mixed, color.a);
    }
    return color;
}

