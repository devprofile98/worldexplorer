@group(0) @binding(0) var srcTexture: texture_2d<f32>;           // read-only, sampled from previous mip
@group(0) @binding(1) var dstTexture: texture_storage_2d<rgba8unorm, write>;  // write-only

@compute @workgroup_size(8, 8)
fn computeMip(@builtin(global_invocation_id) id: vec3<u32>) {
    let dstSize = textureDimensions(dstTexture);
    if (id.x >= dstSize.x || id.y >= dstSize.y) {
        return;
    }

    // Sample 2x2 block from source mip (mip level 0 relative to the *view* we bound)
    let c0 = textureLoad(srcTexture, vec2u(2 * id.x,     2 * id.y),     0);
    let c1 = textureLoad(srcTexture, vec2u(2 * id.x + 1, 2 * id.y),     0);
    let c2 = textureLoad(srcTexture, vec2u(2 * id.x,     2 * id.y + 1), 0);
    let c3 = textureLoad(srcTexture, vec2u(2 * id.x + 1, 2 * id.y + 1), 0);

    let avg = (c0 + c1 + c2 + c3) * 0.25;

    textureStore(dstTexture, id.xy, avg);
}
