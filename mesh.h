#ifndef WEBGPUTEST_MESH_H
#define WEBGPUTEST_MESH_H

#include <format>
#include <iostream>

#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gpu_buffer.h"
#include "imgui.h"
#include "texture.h"
#include "tinyobjloader/tiny_obj_loader.h"

// Shader-equivalant struct for vertex data
struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec3 tangent;
        glm::vec3 biTangent;
        glm::vec2 uv;
};

class Mesh {
    public:
        std::vector<VertexAttributes> mVertexData;
        Texture* mTexture = nullptr;
        Buffer mVertexBuffer = {};
        bool isTransparent = false;
        WGPUBindGroup mTextureBindGroup = {};
        std::vector<WGPUBindGroupEntry> binding_data{2};
};

#endif  //! WEBGPUTEST_MESH_H
