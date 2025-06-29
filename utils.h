#ifndef TEST_WGPU_UTILS_H
#define TEST_WGPU_UTILS_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

namespace fs = std::filesystem;

class Appliaction;  // forward declaration of the app class

void setDefault(WGPUDepthStencilState& depthStencilState);
void setDefault(WGPUStencilFaceState& stencilFaceState);

void setDefaultUseStencil(WGPUDepthStencilState& depthStencilState);
void setDefaultActiveStencil(WGPUDepthStencilState& depthStencilState);
void setDefault(WGPUBindGroupLayoutEntry& bindingLayout);
void setDefault(WGPULimits& limits);

struct Terrain {
        std::vector<VertexAttributes> vertices;
        std::vector<uint16_t> indices;

        Terrain& uploadToGpu(Application* app);
        /*
         * @brief Generate Terrain vertices and indices for the gridsize
         */
        Terrain& generate(size_t gridSize, uint8_t octaves, std::vector<glm::vec3>& output);
        void draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData);
        void createSomeBinding(Application* app);
	static float perlin(float x, float y);

    private:
        WGPUBuffer mVertexBuffer;
        WGPUBuffer mIndexBuffer;
        std::vector<uint8_t> mPixels;
        WGPUTexture mTexture;
        WGPUTextureView mTextureView;

        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mRotationMatrix;

        WGPUBindGroup mBindGroup;
        WGPUBuffer mUniformBuffer;

        WGPUBindGroup ggg = {};

        ObjectInfo mObjectInfo;

        uint32_t mGridSize = 0;
};

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData,
                  size_t dimensions);

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device);

enum class VertexStepMode {
    VERTEX = 0,
    INSTANCE,
    VERTEX_BUFFER_NOT_USED,
};

struct VertexBufferLayout {
        VertexBufferLayout& addAttribute(uint64_t offset, uint32_t location, WGPUVertexFormat format);
        WGPUVertexBufferLayout configure(uint64_t arrayStride, VertexStepMode stepMode);

        // Getters
        WGPUVertexBufferLayout getLayout();

    private:
        std::vector<WGPUVertexAttribute> mAttribs;
        WGPUVertexBufferLayout mLayout;
};

// Terrain generateTerrainVertices(size_t gridSize);

#endif  //  TEST_WGPU_UTILS_H
