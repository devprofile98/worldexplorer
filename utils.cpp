#include "utils.h"

#include <format>
#include <iostream>
#include <numeric>

#include "application.h"
#include "stb_image.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stbi_image_write.h"
#pragma GCC diagnostic pop

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

void setDefault(WGPUStencilFaceState& stencilFaceState) {
    stencilFaceState.compare = WGPUCompareFunction_Always;
    stencilFaceState.failOp = WGPUStencilOperation_Keep;
    stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
    stencilFaceState.passOp = WGPUStencilOperation_Keep;
}

void setDefault(WGPUDepthStencilState& depthStencilState) {
    depthStencilState.format = WGPUTextureFormat_Undefined;
    depthStencilState.depthWriteEnabled = false;
    depthStencilState.depthCompare = WGPUCompareFunction_Always;
    depthStencilState.stencilReadMask = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask = 0xFFFFFFFF;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
}

void setDefault(WGPULimits& limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

void setDefault(WGPUBindGroupLayoutEntry& bindingLayout) {
    bindingLayout = {};
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
    bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
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
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double lerp(double t, double a, double b) { return a + t * (b - a); }

// cubic fade function
double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

double perlin(double x, double y, double z) {
    uint32_t cx = static_cast<uint32_t>(std::floor(x)) & 255;
    uint32_t cy = static_cast<uint32_t>(std::floor(y)) & 255;
    uint32_t cz = static_cast<uint32_t>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

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
                     lerp(u, grad(p[AB + 1], x, y - 1.0, z - 1.0), grad(p[BB + 1], x - 1.0, y - 1.0, z - 1.0))));
}
}  // namespace noise

double test_perlin(double x, double z, double persistence, size_t octaves, size_t gridSize) {
    double pixel_result = 0.0;
    double frequency = 2.0;
    double amp = 1.0;
    for (size_t o = 0; o < octaves; o++) {
        double nx = (double)x / gridSize * frequency;
        double nz = (double)z / gridSize * frequency;
        double pixel = noise::perlin(nz, nx, 0);
        pixel_result += ((pixel + 1.0) * 0.5 * 255.0) * amp;
        frequency *= 2.0;
        amp *= persistence;
    }
    return pixel_result;
}

float Terrain::perlin(float x, float y) { return test_perlin(x, y, 0.5, 8, 200) * 0.1 - 25.0f; }

Terrain& Terrain::generate(size_t gridSize, uint8_t octaves, std::vector<glm::vec3>& output) {
    mRotationMatrix = glm::mat4{1.0};
    mTranslationMatrix = glm::mat4{1.0};
    mScaleMatrix = glm::mat4{1.0};
    mScaleMatrix = glm::scale(mScaleMatrix, glm::vec3{1.0});
    mObjectInfo.transformation = mScaleMatrix * mRotationMatrix * mTranslationMatrix;
    mObjectInfo.isFlat = 1;
    std::vector<VertexAttributes> foliage_positions;

    std::array<glm::vec3, 5> height_color = {
        glm::vec3{0.0 / 255.0, 0.0 / 255.0, 139.0 / 255.0},      // Deep Water (Dark Blue)
        glm::vec3{0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0},      // Shallow Water (Blue)
        glm::vec3{34.0 / 255.0, 139.0 / 255.0, 34.0 / 255.0},    // Grassland (Green)
        glm::vec3{139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0},    // Mountain (Brown)
        glm::vec3{255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},  // Snow (White)
    };

    std::array<uint8_t, 5> height_color_index = {
        0,  // glm::vec3{0.0 / 255.0, 0.0 / 255.0, 139.0 / 255.0},      // Deep Water (Dark Blue)
        1,  // glm::vec3{0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0},      // Shallow Water (Blue)
        2,  // glm::vec3{34.0 / 255.0, 139.0 / 255.0, 34.0 / 255.0},    // Grassland (Green)
        3,  // glm::vec3{139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0},    // Mountain (Brown)
        4,  // glm::vec3{255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},  // Snow (White)
    };

    mGridSize = gridSize;
    mPixels.reserve(mGridSize * mGridSize);

    double height_scale = 0.1;
    /*double frequency = 1.023;*/
    double start_amp = 1.0;
    double clamp_scale = 0.0;
    double persistence = 0.5;
    // calculating the clamp scale
    for (size_t o = 0; o < octaves; o++) {
        clamp_scale += start_amp;
        start_amp *= persistence;
    }

    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();

    for (size_t x = 0; x < gridSize; x++) {
        for (size_t z = 0; z < gridSize; z++) {
            double pixel_result = test_perlin(x, z, persistence, octaves, gridSize);

            VertexAttributes attr = {};
            double vertex_height = pixel_result / clamp_scale;

            min = vertex_height < min ? vertex_height : min;
            max = vertex_height > max ? vertex_height : max;

            mPixels.push_back(vertex_height);
            attr.position = {x, z, pixel_result * height_scale};
            attr.normal = {1.0, 0.0, 0.0};

            if (pixel_result > 300) {
                attr.color = height_color[4];
                attr.color.r = height_color_index[4];
            } else if (pixel_result > 260) {
                attr.color = height_color[3];
                attr.color.r = height_color_index[3];

            } else if (pixel_result > 210) {
                attr.color = height_color[2];
                attr.color.r = height_color_index[2];

            } else {
                attr.color = height_color[0];
                attr.color.r = height_color_index[1];
            }
            attr.uv = {x, z};
            this->vertices.push_back(attr);
            // Generate foliage positions
            if (pixel_result > 220 && pixel_result < 260) {  // Place foliage only on higher terrain
                for (int i = 0; i < 4; i++) {                // Add multiple foliage per grid cell
                    double offsetX = (rand() % 100) / 100.0;
                    double offsetZ = (rand() % 100) / 100.0;
                    double foliage_x = x + offsetX;
                    double foliage_z = z + offsetZ;

                    foliage_positions.push_back(
                        {{foliage_x, foliage_z, Terrain::perlin(foliage_x, foliage_z )}, {0.0, 1.0, 0.0}, {0, 255, 0}, {foliage_x, foliage_z}});
                }
            }
        }
    }

    for (auto& e : vertices) {
        e.position.z -= 25;
        e.position.x -= 50;
        e.position.y -= 50;
    }

    for (auto& f : foliage_positions) {
        /*f.position.z -= 25;*/
        f.position.x -= 50;
        f.position.y -= 50;
        output.push_back(f.position);
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
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    buffer_descriptor.size = sizeof(ObjectInfo);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &buffer_descriptor);

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

    stbi_write_png("Noise.png", mGridSize, mGridSize, 1, mPixels.data(), mGridSize);

    return *this;
}

void Terrain::createSomeBinding(Application* app) {
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = mUniformBuffer;
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(ObjectInfo);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);
}

void Terrain::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;

    auto& render_resource = app->getRendererResource();
    // mbidngroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexBuffer));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, app->getBindingGroup().getBindGroup(), 0, nullptr);

    wgpuQueueWriteBuffer(render_resource.queue, mUniformBuffer, 0, &mObjectInfo, sizeof(ObjectInfo));

    createSomeBinding(app);
    wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);

    // // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDrawIndexed(encoder, indices.size(), 1, 0, 0, 0);
    // wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
}

/// vertex buffer layout

WGPUVertexStepMode stepModeFrom(VertexStepMode mode) {
    WGPUVertexStepMode ret = WGPUVertexStepMode_Vertex;
    if (mode == VertexStepMode::VERTEX) {
        ret = WGPUVertexStepMode_Vertex;
    } else if (mode == VertexStepMode::INSTANCE) {
        ret = WGPUVertexStepMode_Instance;
    } else if (mode == VertexStepMode::VERTEX_BUFFER_NOT_USED) {
        ret = WGPUVertexStepMode_VertexBufferNotUsed;
    } else {
        ret = WGPUVertexStepMode_Force32;
    }
    return ret;
}

VertexBufferLayout& VertexBufferLayout::addAttribute(uint64_t offset, uint32_t location, WGPUVertexFormat format) {
    mAttribs.push_back(WGPUVertexAttribute{.format = format, .offset = offset, .shaderLocation = location});
    return *this;
}

WGPUVertexBufferLayout VertexBufferLayout::configure(uint64_t arrayStride, VertexStepMode stepMode) {
    mLayout.attributeCount = mAttribs.size();
    mLayout.attributes = mAttribs.data();
    mLayout.stepMode = stepModeFrom(stepMode);
    mLayout.arrayStride = arrayStride;

    return mLayout;
}

WGPUVertexBufferLayout VertexBufferLayout::getLayout() { return mLayout; }
