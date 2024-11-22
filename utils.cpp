#include "utils.h"

#include <format>
#include <iostream>

#include "application.h"

#define TINYOBJLOADER_IMPLEMENTATION  // add this to exactly 1 of your C++ files
#include "tinyobjloader/tiny_obj_loader.h"

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData,
                  size_t dimensions) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open file at " << path.c_str() << std::endl;
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section {
        None,
        Points,
        Indices,
    };

    Section current_section = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof()) {
        getline(file, line);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "[points]") {
            current_section = Section::Points;
        } else if (line == "[indices]") {
            current_section = Section::Indices;
        } else if (line[0] == '#' || line.empty()) {
            // doing nothing
        } else if (current_section == Section::Points) {
            std::istringstream iss{line};
            for (size_t i = 0; i < dimensions + 3; i++) {
                iss >> value;
                pointData.push_back(value);
            }
        } else if (current_section == Section::Indices) {
            std::istringstream iss{line};
            for (int i = 0; i < 3; i++) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open shader at " << path.c_str() << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    std::string shader_code(file_size, ' ');  // what the fuck!
    file.seekg(0);
    file.read(shader_code.data(), file_size);

    WGPUShaderModuleWGSLDescriptor module_descriptor = {};
    module_descriptor.chain.next = nullptr;
    module_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    module_descriptor.code = shader_code.c_str();

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = nullptr;
    shader_descriptor.hintCount = 0;
    shader_descriptor.hints = nullptr;
    shader_descriptor.nextInChain = &module_descriptor.chain;
    return wgpuDeviceCreateShaderModule(device, &shader_descriptor);
}

namespace noise {
// permutation table
uint8_t p[512] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142, 8,
    99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,
    11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139,
    48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,
    244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135,
    130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147,
    118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119,
    248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,  108, 110,
    79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162,
    241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176, 115, 121,
    50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215,
    61,  156, 180, 151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
    69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219,
    203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165,
    71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,
    46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
    200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183,
    170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,
    98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,
    191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204,
    176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195,
    78,  66,  215, 61,  156, 180,
};

// based on the input hash, this function will select
double grad(uint8_t hash, double x, double y, double z) {
    uint8_t h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 14 || h == 12) ? x : z;
    return ((h & 1) == 0 ? u : -u) + (h & 2) == 0 ? v : -v;
}

double lerp(double t, double a, double b) { return a + t * (b - a); }

// cubic fade function
double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

double perlin(double x, double y, double z) {
    uint8_t cx = static_cast<long>(x) & 255;
    uint8_t cy = static_cast<long>(y) & 255;
    uint8_t cz = static_cast<long>(z) & 255;

    x -= static_cast<double>(cx);
    y -= static_cast<double>(cy);
    z -= static_cast<double>(cz);

    double u = fade(x);
    double v = fade(y);
    double w = fade(z);

    uint32_t A = p[cx] + cy;
    uint32_t B = p[cx + 1] + cy;

    uint32_t AA = p[A] + cz;
    uint32_t BA = p[B] + cz;

    uint32_t AB = p[A + 1] + cz;
    uint32_t BB = p[B + 1] + cz;

    return lerp(w,
                lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1.0, y, z)),
                     lerp(u, grad(p[AB], x, y - 1.0, z), grad(p[BB], x - 1.0, y - 1.0, z))),
                lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1.0), grad(p[BA + 1], x - 1.0, y, z - 1.0)),
                     lerp(u, grad(p[AB + 1], x, y - 1.0, z - 1.0), grad(p[BB + 1], x - 1.0, y - 1.0, z - 1.0)))) +
           0.3;
}
}  // namespace noise

Terrain& Terrain::generate(size_t gridSize, uint8_t octaves) {
    // Terrain terrain;
    // terrain.vertices;
    mGridSize = gridSize;
    mPixels.reserve(mGridSize * mGridSize);

    double height_scale = 0.0;
    double frequency = 1.12;
    double amp = 1.0;
    double clamp_scale = 0.0;
    double persistence = 0.5;
    // calculating the clamp scale
    for (size_t o = 0; o < octaves; o++) {
        clamp_scale += amp;
        amp *= persistence;
    }

    std::cout << "The Clamp Scale Is : " << clamp_scale << '\n';

    for (size_t x = 0; x < gridSize; x++) {
        for (size_t z = 0; z < gridSize; z++) {
            double pixel_result = 0.0;
            frequency = 4.0;
            amp = 1.0;
            for (size_t o = 0; o < octaves; o++) {
                double nx = (double)x / (double)gridSize * frequency;
                double nz = (double)z / (double)gridSize * frequency;
                pixel_result += ((noise::perlin(nx, nz, 0) + 1.0) / 2.0 * 255.0) * amp;
                frequency *= 2.0;
                amp *= persistence;
            }

            VertexAttributes attr;
            std::cout << std::format("Generated height is {} {}\n", pixel_result,
                                     pixel_result / clamp_scale * height_scale);
            mPixels.push_back(pixel_result);
            attr.position = {x, z, pixel_result / clamp_scale * height_scale};
            attr.normal = {1.0, 0.0, 0.0};
            attr.color = {1.0, 0.0, 0.0};
            attr.uv = {(double)x / gridSize, (double)z / gridSize};
            this->vertices.push_back(attr);
        }
    }

    // std::vector<size_t> terrain.indices;
    for (size_t x = 0; x < gridSize - 1; x++) {
        for (size_t z = 0; z < gridSize - 1; z++) {
            size_t topLeft = x * gridSize + z;
            size_t topRight = topLeft + 1;
            size_t bottomLeft = topLeft + gridSize;
            size_t bottomRight = bottomLeft + 1;

            this->indices.push_back(topLeft);
            this->indices.push_back(bottomLeft);
            this->indices.push_back(topRight);
            this->indices.push_back(topRight);
            this->indices.push_back(bottomLeft);
            this->indices.push_back(bottomRight);
        }
    }

    // calculate normals
    for (size_t t = 0; t < indices.size(); t += 3) {
        auto a = vertices[indices[t]].position;
        auto b = vertices[indices[t + 1]].position;
        auto c = vertices[indices[t + 2]].position;

        auto ba = glm::vec3{b.x - a.x, b.y - a.y, b.z - a.z};
        auto ca = glm::vec3{c.x - a.x, c.y - a.y, c.z - a.z};
        auto normal = glm::cross(ba, ca);
        vertices[indices[t]].normal = normal;
        vertices[indices[t + 1]].normal = normal;
        vertices[indices[t + 2]].normal = normal;
    }

    return *this;
}

Terrain& Terrain::uploadToGpu(Application* app) {
    WGPUBufferDescriptor vab_descriptor = {};  // vertex attribte buffer
    vab_descriptor.nextInChain = nullptr;
    vab_descriptor.size = vertices.size() * sizeof(VertexAttributes);
    vab_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    vab_descriptor.mappedAtCreation = false;

    mVertexBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &vab_descriptor);
    // Uploading vertices attribute data to GPU
    wgpuQueueWriteBuffer(app->getRendererResource().queue, mVertexBuffer, 0, vertices.data(), vab_descriptor.size);

    WGPUBufferDescriptor index_buffer_desc = {};  // vertex attribte buffer
    index_buffer_desc.size = indices.size() * sizeof(uint16_t);
    index_buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    index_buffer_desc.size = (index_buffer_desc.size + 3) & ~3;
    mIndexBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &index_buffer_desc);
    // uploading indices data to the gpu
    wgpuQueueWriteBuffer(app->getRendererResource().queue, mIndexBuffer, 0, indices.data(), index_buffer_desc.size);

    // generate the 2D noise texture and upload it too
    // converting data to a 2D texture form in cpu side
    // create texture desc
    WGPUTextureDescriptor texture_desc = {};
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.format = WGPUTextureFormat_R8Unorm;
    texture_desc.label = "Noise Texture";
    texture_desc.mipLevelCount = 1;
    texture_desc.nextInChain = nullptr;
    texture_desc.sampleCount = 1;
    texture_desc.size = {mGridSize, mGridSize, 1};
    texture_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    texture_desc.viewFormatCount = 0;
    texture_desc.viewFormats = 0;

    // create texture
    mTexture = wgpuDeviceCreateTexture(app->getRendererResource().device, &texture_desc);

    // create texture descriptor
    WGPUTextureViewDescriptor view_desc = {};
    view_desc.aspect = WGPUTextureAspect_All;
    view_desc.baseArrayLayer = 0;
    view_desc.arrayLayerCount = 1;
    view_desc.baseMipLevel = 0;
    view_desc.mipLevelCount = 1;
    view_desc.dimension = WGPUTextureViewDimension_2D;
    view_desc.format = WGPUTextureFormat_R8Unorm;

    // create texture view
    mTextureView = wgpuTextureCreateView(mTexture, &view_desc);

    WGPUImageCopyTexture destination;
    destination.texture = mTexture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};              // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All;  // only relevant for depth/Stencil textures

    WGPUTextureDataLayout source;
    source.nextInChain = nullptr;
    source.offset = 0;
    // upload the level zero: aka original texture
    source.bytesPerRow = mGridSize;
    source.rowsPerImage = mGridSize;

    wgpuQueueWriteTexture(app->getRendererResource().queue, &destination, mPixels.data(), mPixels.size(), &source,
                          &texture_desc.size);

    return *this;
}

void Terrain::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;

    // 1 - SET THE TEXTURE
    // 2 - SET VERTEX BUFFER
    // 3 - SET INDEX BUFFER

    // 4 - DRAW INDEXED
    WGPUBindGroup mbidngroup = {};
    bindingData[1].nextInChain = nullptr;
    bindingData[1].binding = 1;
    bindingData[1].textureView = mTextureView;
    auto& desc = app->getBindingGroup().getDescriptor();
    desc.entries = bindingData.data();
    auto& render_resource = app->getRendererResource();
    // auto& uniform_data = app->getUniformData();
    mbidngroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexBuffer));
    // wgpuRenderPassEncoderSetBindGroup(encoder, 0, app->getBindingGroup().getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mbidngroup, 0, nullptr);

    // wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindGroup, 0, nullptr);

    // // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDrawIndexed(encoder, indices.size(), 1, 0, 0, 0);
    // wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
}
