#include "utils.h"

#include <webgpu/webgpu.h>

#include <format>
#include <iostream>
#include <numbers>
#include <numeric>
#include <string>
#include <vector>

#include "application.h"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "mesh.h"
#include "model.h"
#include "rendererResource.h"
#include "stb_image.h"
#include "wgpu_utils.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stbi_image_write.h"
#pragma GCC diagnostic pop

#define TINYOBJLOADER_IMPLEMENTATION  // add this to exactly 1 of your C++ files
#include "../tinyobjloader/tiny_obj_loader.h"

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData,
                  size_t dimensions) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open file at " << path << std::endl;
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

TransformProperties decomposeTransformation(const glm::mat4& transformation) {
    // auto new_local_transform = glm::inverse(parent->getGlobalTransform()) * child->mTransform.mTransformMatrix;

    glm::vec3 position = glm::vec3(transformation[3]);

    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(transformation[0]));
    scale.y = glm::length(glm::vec3(transformation[1]));
    scale.z = glm::length(glm::vec3(transformation[2]));
    // child->mTransform.getLocalTransform();
    // auto translate = glm::translate(glm::mat4{1.0}, position);

    // Avoid division by zero for very small scales
    const float epsilon = 1e-6f;
    if (scale.x < epsilon) scale.x = 1.0f;
    if (scale.y < epsilon) scale.y = 1.0f;
    if (scale.z < epsilon) scale.z = 1.0f;

    // Extract rotation by removing scale from the 3x3 part of the matrix
    glm::mat3 rotation_matrix;
    rotation_matrix[0] = glm::vec3(transformation[0]) / scale.x;
    rotation_matrix[1] = glm::vec3(transformation[1]) / scale.y;
    rotation_matrix[2] = glm::vec3(transformation[2]) / scale.z;

    // Convert the rotation matrix to a quaternion
    glm::quat rotation = glm::quat_cast(rotation_matrix);

    return {position, scale, rotation};
}

void setDefault(WGPUStencilFaceState& stencilFaceState) {
    stencilFaceState.compare = WGPUCompareFunction_Always;
    stencilFaceState.failOp = WGPUStencilOperation_Keep;
    stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
    stencilFaceState.passOp = WGPUStencilOperation_Keep;
}

void setDefault(WGPUDepthStencilState& depthStencilState) {
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
    depthStencilState.format = WGPUTextureFormat_Undefined;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_False;
    depthStencilState.depthCompare = WGPUCompareFunction_Always;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0x00;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
}

void setDefaultActiveStencil(WGPUDepthStencilState& depthStencilState) {
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
    depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.depthCompare = WGPUCompareFunction_LessEqual;
    depthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilFront.passOp = WGPUStencilOperation_Replace;
    depthStencilState.stencilFront.depthFailOp = WGPUStencilOperation_Replace;
    depthStencilState.stencilBack.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilBack.passOp = WGPUStencilOperation_Replace;
    depthStencilState.stencilBack.depthFailOp = WGPUStencilOperation_Replace;
    depthStencilState.stencilReadMask = 0xFF;
    depthStencilState.stencilWriteMask = 0xFF;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
}

void setDefaultActiveStencil2(WGPUDepthStencilState& depthStencilState) {
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
    depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilFront.passOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilBack.compare = WGPUCompareFunction_Always;
    depthStencilState.stencilBack.passOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
    depthStencilState.stencilReadMask = 0xFF;
    depthStencilState.stencilWriteMask = 0x00;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
}

void setDefaultUseStencil(WGPUDepthStencilState& depthStencilState) {
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
    depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_False;
    depthStencilState.depthCompare = WGPUCompareFunction_Always;
    depthStencilState.stencilFront.compare = WGPUCompareFunction_NotEqual;
    depthStencilState.stencilBack.compare = WGPUCompareFunction_NotEqual;
    depthStencilState.stencilReadMask = 0xFF;
    depthStencilState.stencilWriteMask = 0x00;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
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
    // limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
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
    bindingLayout.buffer.type = WGPUBufferBindingType_BindingNotUsed;
    bindingLayout.buffer.hasDynamicOffset = false;
    bindingLayout.buffer.minBindingSize = 100;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_BindingNotUsed;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_BindingNotUsed;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_BindingNotUsed;
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
    // mObjectInfo.isFlat = 1;
    std::vector<VertexAttributes> foliage_positions;

    std::array<glm::vec3, 5> height_color = {
        glm::vec3{0.0 / 255.0, 0.0 / 255.0, 139.0 / 255.0},      // Deep Water (Dark Blue)
        glm::vec3{0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0},      // Shallow Water (Blue)
        glm::vec3{34.0 / 255.0, 139.0 / 255.0, 34.0 / 255.0},    // Grassland (Green)
        glm::vec3{139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0},    // Mountain (Brown)
        glm::vec3{255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0},  // Snow (White)
    };

    std::array<uint8_t, 5> height_color_index = {
        0,  // Deep Water (Dark Blue)
        1,  // Shallow Water (Blue)
        2,  // Grassland (Green)
        3,  // Mountain (Brown)
        4,  // Snow (White)
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

    for (size_t x = 0; x < gridSize; ++x) {
        for (size_t z = 0; z < gridSize; ++z) {
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

            } else if (pixel_result > 217) {
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

                    foliage_positions.push_back({{foliage_x, foliage_z, Terrain::perlin(foliage_x, foliage_z)},
                                                 {0.0, 1.0, 0.0},
                                                 {0, 255, 0},
                                                 {1, 0, 0},
                                                 {0, 1, 0},
                                                 {foliage_x, foliage_z}});
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
    for (size_t x = 0; x < gridSize - 1; ++x) {
        for (size_t z = 0; z < gridSize - 1; ++z) {
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

    // calculate normals Using Cross product of 2 side of the triangle

    // for (size_t t = 0; t < indices.size(); t += 3) {
    //     auto a = vertices[indices[t]].position;
    //     auto b = vertices[indices[t + 1]].position;
    //     auto c = vertices[indices[t + 2]].position;
    //
    //     auto ba = glm::vec3{b.x - a.x, b.y - a.y, b.z - a.z};
    //     auto ca = glm::vec3{c.x - a.x, c.y - a.y, c.z - a.z};
    //     auto normal = glm::cross(ba, ca);
    //     vertices[indices[t]].normal = normal;
    //     vertices[indices[t + 1]].normal = normal;
    //     vertices[indices[t + 2]].normal = normal;
    // }

    // calculate normal using centeral derivative
    auto gridSpacing_x = 1;
    auto gridSpacing_z = 1;
    for (size_t x = 1; x < gridSize - 1; ++x) {
        for (size_t y = 1; y < gridSize - 1; ++y) {
            float dfdx = (vertices[(x + 1) * gridSize + y].position.z - vertices[(x - 1) * gridSize + y].position.z) /
                         (2.0f * gridSpacing_x);

            float dfdz = (vertices[x * gridSize + (y + 1)].position.z - vertices[x * gridSize + (y - 1)].position.z) /
                         (2.0f * gridSpacing_z);

            glm::vec3 T = glm::normalize(glm::vec3(1.0f, 0.0, dfdx));
            vertices[x * gridSize + y].normal = glm::normalize(glm::vec3(-dfdx, -dfdz, 1.0f));
            vertices[x * gridSize + y].tangent = glm::normalize(T);
            vertices[x * gridSize + y].biTangent = glm::normalize(glm::cross(vertices[x * gridSize + y].normal, T));
        }
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
    texture_desc.label = {"Noise Texture", 14};
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

    WGPUTexelCopyTextureInfo destination;
    destination.texture = mTexture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};              // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All;  // only relevant for depth/Stencil textures

    WGPUTexelCopyBufferLayout source;
    // source.nextInChain = nullptr;
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
    std::array<WGPUBindGroupEntry, 3> mBindGroupEntry = {};
    mBindGroupEntry[0].nextInChain = nullptr;
    mBindGroupEntry[0].binding = 0;
    mBindGroupEntry[0].buffer = mUniformBuffer;
    mBindGroupEntry[0].offset = 0;
    mBindGroupEntry[0].size = sizeof(ObjectInfo);

    mBindGroupEntry[1].nextInChain = nullptr;
    mBindGroupEntry[1].buffer = app->mDefaultBoneFinalTransformData.getBuffer();
    mBindGroupEntry[1].binding = 1;
    mBindGroupEntry[1].offset = 0;
    mBindGroupEntry[1].size = 100 * sizeof(glm::mat4);

    mBindGroupEntry[2].nextInChain = nullptr;
    mBindGroupEntry[2].buffer = app->mDefaultMeshGlobalTransformData.getBuffer();
    mBindGroupEntry[2].binding = 2;
    mBindGroupEntry[2].offset = 0;
    mBindGroupEntry[2].size = 20 * sizeof(glm::mat4);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = mBindGroupEntry.data();
    mTrasBindGroupDesc.entryCount = 3;
    mTrasBindGroupDesc.label = createStringView("translation bind group");
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);
}

void Terrain::update(Application* app, float dt) {
    if (mDirty) {
        wgpuQueueWriteBuffer(app->getRendererResource().queue, mUniformBuffer, 0, &mObjectInfo, sizeof(ObjectInfo));
        mDirty = false;
    }
}

void Terrain::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;

    // auto& render_resource = app->getRendererResource();
    // mbidngroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mIndexBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexBuffer));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, app->getBindingGroup().getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(encoder, 2, app->mDefaultTextureBindingGroup.getBindGroup(), 0, nullptr);

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

bool intersection(const glm::vec3& ray_origin, const glm::vec3& ray_dir, const glm::vec3& box_min,
                  const glm::vec3& box_max, glm::vec3* intersection_point) {
    float tmin = 0.0, tmax = INFINITY;

    for (int d = 0; d < 3; ++d) {
        float t1 = (box_min[d] - ray_origin[d]) / ray_dir[d];
        float t2 = (box_max[d] - ray_origin[d]) / ray_dir[d];

        tmin = std::max(tmin, std::min(std::min(t1, t2), tmax));
        tmax = std::min(tmax, std::max(std::max(t1, t2), tmin));
    }

    if (tmin < tmax && tmax >= 0.0f) {
        if (intersection_point != nullptr) {
            // If tmin < 0, the ray starts inside the box; use tmax (exit point)
            float t = tmin >= 0.0f ? tmin : tmax;
            *intersection_point = ray_origin + t * ray_dir;
        }
        return true;
    }
    return false;
}

float rayDotVector(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                   const glm::vec3& vec) {
    auto [xpos, ypos] = mouseCoord;
    double xndc = 2.0 * xpos / (float)width - 1.0;
    double yndc = 1.0 - (2.0 * ypos) / (float)height;
    glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
    glm::vec4 eyecoord = glm::inverse(camera.getProjection()) * clip_coords;
    eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

    // auto ray_origin = camera.getPos();

    glm::vec4 worldray = glm::normalize(glm::inverse(camera.getView()) * eyecoord);
    return glm::dot(glm::vec3{worldray}, vec);
}

BaseModel* testIntersection(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                            const ModelRegistry::ModelContainer& models) {
    auto [xpos, ypos] = mouseCoord;
    double xndc = 2.0 * xpos / (float)width - 1.0;
    double yndc = 1.0 - (2.0 * ypos) / (float)height;
    glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
    glm::vec4 eyecoord = glm::inverse(camera.getProjection()) * clip_coords;
    eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

    auto ray_origin = camera.getPos();

    glm::vec4 worldray = glm::inverse(camera.getView()) * eyecoord;
    auto normalized = glm::normalize(worldray);
    for (auto& obj : models) {
        auto [obj_in_world_min, obj_in_world_max] = obj->getWorldMin();
        bool does_intersect = intersection(ray_origin, normalized, obj_in_world_min, obj_in_world_max);
        if (does_intersect) {
            return obj;
        }
    }
    return nullptr;
}

std::pair<bool, glm::vec3> testIntersectionWithBox(Camera& camera, size_t width, size_t height,
                                                   std::pair<size_t, size_t> mouseCoord, const glm::vec3& min,
                                                   const glm::vec3& max) {
    auto [xpos, ypos] = mouseCoord;
    double xndc = 2.0 * xpos / (float)width - 1.0;
    double yndc = 1.0 - (2.0 * ypos) / (float)height;
    glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
    glm::vec4 eyecoord = glm::inverse(camera.getProjection()) * clip_coords;
    eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

    auto ray_origin = camera.getPos();

    glm::vec4 worldray = glm::inverse(camera.getView()) * eyecoord;
    auto normalized = glm::normalize(worldray);
    // auto [obj_in_world_min, obj_in_world_max] = obj->getWorldMin();
    glm::vec3 data{};
    auto res = intersection(ray_origin, normalized, min, max, &data);
    return {res, data};
}

BaseModel* testIntersection2(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                             const std::vector<BaseModel*>& models) {
    auto [xpos, ypos] = mouseCoord;
    double xndc = 2.0 * xpos / (float)width - 1.0;
    double yndc = 1.0 - (2.0 * ypos) / (float)height;
    glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
    glm::vec4 eyecoord = glm::inverse(camera.getProjection()) * clip_coords;
    eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

    auto ray_origin = camera.getPos();

    glm::vec4 worldray = glm::inverse(camera.getView()) * eyecoord;
    auto normalized = glm::normalize(worldray);
    for (auto& obj : models) {
        if (obj == nullptr) {
            continue;
        }
        auto [obj_in_world_min, obj_in_world_max] = obj->getWorldMin();
        bool does_intersect = intersection(ray_origin, normalized, obj_in_world_min, obj_in_world_max);
        if (does_intersect) {
            return obj;
        }
    }
    return nullptr;
}

std::vector<glm::vec4> generateAABBLines(const glm::vec3& min, const glm::vec3& max) {
    std::vector<glm::vec4> result;

    result.emplace_back(glm::vec4{min, 0.0});
    result.emplace_back(min.x, min.y, max.z, 0.0);
    result.emplace_back(max.x, min.y, max.z, 0.0);
    result.emplace_back(max.x, min.y, min.z, 0.0);
    result.emplace_back(glm::vec4{min, 0.0});
    result.emplace_back(min.x, max.y, min.z, 0.0);
    result.emplace_back(min.x, max.y, max.z, 0.0);
    result.emplace_back(min.x, min.y, max.z, 0.0);
    result.emplace_back(glm::vec4{max, 0.0});
    result.emplace_back(max.x, max.y, min.z, 0.0);
    result.emplace_back(min.x, max.y, min.z, 0.0);
    result.emplace_back(min.x, max.y, max.z, 0.0);
    result.emplace_back(glm::vec4{max, 0.0});
    result.emplace_back(max.x, min.y, max.z, 0.0);
    result.emplace_back(max.x, min.y, min.z, 0.0);
    result.emplace_back(max.x, max.y, min.z, 0.0);
    result.emplace_back(glm::vec4{max, 1.0});

    return result;
}

std::vector<glm::vec4> generateBox(const glm::vec3& center, const glm::vec3& halfExtents) {
    std::vector<glm::vec4> result;

    const float cx = center.x;
    const float cy = center.y;
    const float cz = center.z;

    const float hx = halfExtents.x;
    const float hy = halfExtents.y;
    const float hz = halfExtents.z;

    // Bottom face (Y = center.y - half.y)
    auto bottomFrontLeft = glm::vec3(cx - hx, cy - hy, cz - hz);
    auto bottomFrontRight = glm::vec3(cx + hx, cy - hy, cz - hz);
    auto bottomBackLeft = glm::vec3(cx - hx, cy - hy, cz + hz);
    auto bottomBackRight = glm::vec3(cx + hx, cy - hy, cz + hz);

    // Top face (Y = center.y + half.y)
    glm::vec3 topFrontLeft = glm::vec3(cx - hx, cy + hy, cz - hz);
    auto topFrontRight = glm::vec3(cx + hx, cy + hy, cz - hz);
    auto topBackLeft = glm::vec3(cx - hx, cy + hy, cz + hz);
    auto topBackRight = glm::vec3(cx + hx, cy + hy, cz + hz);

    // front
    result.emplace_back(glm::vec4{topFrontLeft, 0.0});
    result.emplace_back(glm::vec4{topFrontRight, 0.0});
    result.emplace_back(glm::vec4{topBackRight, 0.0});
    result.emplace_back(glm::vec4{topBackLeft, 0.0});
    result.emplace_back(glm::vec4{topFrontLeft, 0.0});
    // back
    result.emplace_back(glm::vec4{bottomFrontLeft, 0.0});
    result.emplace_back(glm::vec4{bottomFrontRight, 0.0});
    result.emplace_back(glm::vec4{bottomBackRight, 0.0});
    result.emplace_back(glm::vec4{bottomBackLeft, 0.0});
    result.emplace_back(glm::vec4{bottomFrontLeft, 1.0});

    // connecting lines
    result.emplace_back(glm::vec4{topBackLeft, 0.0});
    result.emplace_back(glm::vec4{bottomBackLeft, 1.0});

    result.emplace_back(glm::vec4{topBackRight, 0.0});
    result.emplace_back(glm::vec4{bottomBackRight, 1.0});

    result.emplace_back(glm::vec4{topFrontRight, 0.0});
    result.emplace_back(glm::vec4{bottomFrontRight, 1.0});

    return result;
}

std::vector<glm::vec4> generateSphere(uint8_t numLong, uint8_t numLat, uint8_t numLongSegments) {
    std::vector<glm::vec4> results;

    std::vector<glm::vec3> meridians;
    // longitudes
    for (size_t lon = 0; lon < numLong; ++lon) {
        auto theta = lon * (2 * std::numbers::pi) / numLong;
        for (size_t seg = 0; seg < numLongSegments + 1; ++seg) {
            auto phi = seg * (std::numbers::pi) / numLongSegments;
            float x = glm::sin(phi) * glm::cos(theta);
            float y = glm::sin(phi) * glm::sin(theta);
            float z = glm::cos(phi);
            meridians.emplace_back(x, y, z);
        }
    }

    for (size_t i = 0; i < meridians.size() - 1; ++i) {
        auto line_ended = (i % (numLongSegments - 1)) == 0;
        results.emplace_back(glm::vec4{meridians[i], 0});
        results.emplace_back(glm::vec4{meridians[i + 1], line_ended ? 1 : 0});
    }
    results[results.size() - 1].w = 1;

    for (size_t lat = 1; lat < numLat; ++lat) {
        auto phi = lat * (std::numbers::pi / numLat);
        std::vector<glm::vec3> rings;
        // for (size_t seg = 0; seg < num_segments + 1; ++seg) {
        for (size_t lon = 0; lon < numLong; ++lon) {
            auto theta = lon * (2 * std::numbers::pi / numLong);
            // auto phi = seg * (std::numbers::pi) / num_segments;
            float x = glm::sin(phi) * glm::cos(theta);
            float y = glm::sin(phi) * glm::sin(theta);
            float z = glm::cos(phi);
            rings.emplace_back(x, y, z);
        }
        for (size_t i = 0; i < rings.size(); ++i) {
            auto line_ended = i + 1 == rings.size();
            auto next_i = (i + 1) % numLong;
            results.emplace_back(glm::vec4{rings[i], 0});
            results.emplace_back(glm::vec4{rings[next_i], line_ended ? 1 : 0});
        }
    }

    return results;
}

std::vector<glm::vec4> generateCone() {
    std::vector<glm::vec4> result;

    glm::vec3 head{0.0};
    glm::vec3 base_center{head};
    // base_center.z += 0.5;
    glm::vec3 first = base_center + glm::vec3{0.5, 0.5, 0.5};
    glm::vec3 second = base_center + glm::vec3{0.5, -0.5, 0.5};
    glm::vec3 third = base_center + glm::vec3{-0.5, 0.5, 0.5};
    glm::vec3 fourth = base_center + glm::vec3{-0.5, -0.5, 0.5};
    result.emplace_back(head, 0.0);
    result.emplace_back(first, 0.0);
    result.emplace_back(second, 0.0);
    result.emplace_back(fourth, 0.0);
    result.emplace_back(third, 0.0);
    result.emplace_back(first, 0.0);
    result.emplace_back(head, 0.0);
    result.emplace_back(second, 1.0);
    result.emplace_back(head, 0.0);
    result.emplace_back(third, 1.0);
    result.emplace_back(head, 0.0);
    result.emplace_back(fourth, 1.0);

    return result;
}

glm::quat rotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
    start = glm::normalize(start);
    dest = glm::normalize(dest);

    float cos_theta = glm::dot(start, dest);
    glm::vec3 rotation_axis;

    if (cos_theta < -0.9999f) {  // Nearly opposite directions → 180° rotation
        // Find a perpendicular axis (try cross with X, fallback to Y)
        rotation_axis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
        if (glm::length(rotation_axis) < 0.01f) {
            rotation_axis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
        }
        rotation_axis = glm::normalize(rotation_axis);
        return glm::angleAxis(glm::radians(180.0f), rotation_axis);
    }

    // Standard case: cross product gives axis, half-angle formula for stability
    rotation_axis = glm::cross(start, dest);
    float s = std::sqrt((1.0f + cos_theta) * 2.0f);
    float invs = 1.0f / s;

    return glm::quat(s * 0.5f, rotation_axis.x * invs, rotation_axis.y * invs, rotation_axis.z * invs);
}

PerfTimer::PerfTimer(std::string_view label) : mLabel(label), mStart(std::chrono::high_resolution_clock::now()) {}

PerfTimer::~PerfTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - mStart;
    std::cout << "Completes in " << duration.count() << std::endl;
}
