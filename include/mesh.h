#ifndef WEBGPUTEST_MESH_H
#define WEBGPUTEST_MESH_H

#include <stdalign.h>

#include <cstdint>
#include <format>
#include <iostream>
#include <memory>

#include "../tinyobjloader/tiny_obj_loader.h"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gpu_buffer.h"
#include "imgui.h"
// #include "texture.h"

class Texture;

enum class MaterialProps : uint32_t {
    None = 0,                     // No flags set
    HasDiffuseMap = (1u << 0),    // Bit 0: Does the material have a diffuse texture map?
    HasNormalMap = (1u << 1),     // Bit 1: Does the material have a normal map?
    IsMetallic = (1u << 2),       // Bit 2: Is it a metallic material?
    HasRoughnessMap = (1u << 3),  // Bit 3: Does it have a roughness map?
    HasEmissiveMap = (1u << 4),   // Bit 4: Does it have an emissive map?
    IsDoubleSided = (1u << 5),    // Bit 5: Is it double-sided?
    IsAnimated = (1u << 6),       // Bit 5: Is it animated?

};

inline MaterialProps operator|(MaterialProps a, MaterialProps b) {
    return static_cast<MaterialProps>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline MaterialProps operator&(MaterialProps a, MaterialProps b) {
    return static_cast<MaterialProps>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline MaterialProps operator^(MaterialProps a, MaterialProps b) {
    return static_cast<MaterialProps>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

inline MaterialProps& operator|=(MaterialProps& a, MaterialProps b) {
    a = static_cast<MaterialProps>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    return a;
}

inline MaterialProps& operator&=(MaterialProps& a, MaterialProps b) {
    a = static_cast<MaterialProps>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    return a;
}

inline MaterialProps operator~(MaterialProps a) { return static_cast<MaterialProps>(~static_cast<uint32_t>(a)); }

struct alignas(16) Material {
        // Block 1: 4 x 4-byte fields = 16 bytes
        uint32_t isFlat;
        uint32_t useTexture;
        uint32_t isFoliage;
        uint32_t materialProps;

        float roughness;  // Matches metallicness: f32 (4 bytes)

        uint32_t _padding[3];  // 3 * 4 bytes = 12 bytes

        glm::vec3 uvMultiplier = glm::vec3{1.0};  // Matches uvMultiplier: vec3f (12 bytes)

        inline bool hasFlag(MaterialProps checkFlag) {
            return (static_cast<uint32_t>(materialProps) & static_cast<uint32_t>(checkFlag)) != 0;
        }

        inline void setFlag(MaterialProps flag, bool value) {
            if (value) {
                materialProps |= static_cast<uint32_t>(flag);
            } else {
                materialProps &= ~static_cast<uint32_t>(flag);
            }
        }
};

// Shader-equivalant struct for vertex data
struct alignas(16) VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec3 tangent;
        glm::vec3 biTangent;
        glm::vec2 uv;
        glm::ivec4 boneIds{0, 0, 0, 0};
        glm::vec4 weights{1.0, 0.0, 0.0, 0.0};
};

class Mesh {
    public:
        unsigned int meshId;
        std::vector<VertexAttributes> mVertexData;
        std::vector<uint32_t> mIndexData;
        std::shared_ptr<Texture> mTexture = nullptr;
        std::shared_ptr<Texture> mSpecularTexture = nullptr;
        std::shared_ptr<Texture> mNormalMapTexture = nullptr;
        Buffer mVertexBuffer = {};
        Buffer mIndexBuffer = {};
        Buffer mIndirectDrawArgsBuffer;
        Buffer mMaterialBuffer;
        Buffer mMeshMapIdx;
        bool isTransparent = false;
        WGPUBindGroup mTextureBindGroup = {};
        std::vector<WGPUBindGroupEntry> binding_data{2};
        WGPUBindGroup mMaterialBindGroup = {};
        Material mMaterial;
};

#endif  //! WEBGPUTEST_MESH_H
