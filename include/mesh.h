#ifndef WEBGPUTEST_MESH_H
#define WEBGPUTEST_MESH_H

#include <stdalign.h>

#include <cstdint>
#include <format>
#include <iostream>

#include "../tinyobjloader/tiny_obj_loader.h"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gpu_buffer.h"
#include "imgui.h"
#include "texture.h"

// Shader-equivalant struct for vertex data
struct alignas(16) VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec3 tangent;
        glm::vec3 biTangent;
        glm::vec2 uv;
        glm::ivec4 boneIds{0};
        glm::vec4 weights{0};
};

class Mesh {
    public:
        std::vector<VertexAttributes> mVertexData;
        std::vector<uint32_t> mIndexData;
        Texture* mTexture = nullptr;
        Texture* mSpecularTexture = nullptr;
        Texture* mNormalMapTexture = nullptr;
        Buffer mVertexBuffer = {};
        Buffer mIndexBuffer = {};
        bool isTransparent = false;
        WGPUBindGroup mTextureBindGroup = {};
        std::vector<WGPUBindGroupEntry> binding_data{2};
        Buffer mIndirectDrawArgsBuffer;  // copy dst, map read
};

#endif  //! WEBGPUTEST_MESH_H
