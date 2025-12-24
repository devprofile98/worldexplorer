#ifndef TEST_WGPU_UTILS_H
#define TEST_WGPU_UTILS_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "camera.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "model_registery.h"

namespace fs = std::filesystem;

class Appliaction;  // forward declaration of the app class

void setDefault(WGPUDepthStencilState& depthStencilState);
void setDefault(WGPUStencilFaceState& stencilFaceState);

void setDefaultUseStencil(WGPUDepthStencilState& depthStencilState);
void setDefaultActiveStencil(WGPUDepthStencilState& depthStencilState);
void setDefaultActiveStencil2(WGPUDepthStencilState& depthStencilState);
void setDefault(WGPUBindGroupLayoutEntry& bindingLayout);
void setDefault(WGPULimits& limits);

struct TransformProperties {
        glm::vec3 translation;
        glm::vec3 scale;
        glm::quat rotation;
};
// TransformProperties calculateChildTransform(Model* parent, Model* child);
TransformProperties decomposeTransformation(const glm::mat4& transformation);

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
        void update(Application* app, float dt);
        bool mDirty = true;

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

bool intersection(const glm::vec3& ray_origin, const glm::vec3& ray_dir, const glm::vec3& box_min,
                  const glm::vec3& box_max, glm::vec3* intersection_point = nullptr);

BaseModel* testIntersection(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                            const ModelRegistry::ModelContainer& models);
BaseModel* testIntersection2(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                             const std::vector<BaseModel*>& models);
float rayDotVector(Camera& camera, size_t width, size_t height, std::pair<size_t, size_t> mouseCoord,
                   const glm::vec3& vec);

std::pair<bool, glm::vec3> testIntersectionWithBox(Camera& camera, size_t width, size_t height,
                                                   std::pair<size_t, size_t> mouseCoord, const glm::vec3& min,
                                                   const glm::vec3& max);
// Terrain generateTerrainVertices(size_t gridSize);
std::vector<glm::vec4> generateAABBLines(const glm::vec3& min, const glm::vec3& max);
std::vector<glm::vec4> generateCone();
std::vector<glm::vec4> generateBox(const glm::vec3& center = {0, 0, 0}, const glm::vec3& halfExtents = {0.5, 0.5, 0.5});
std::vector<glm::vec4> generateSphere(uint8_t numLong = 16, uint8_t numLat = 12, uint8_t numLongSegments = 8);

glm::quat rotationBetweenVectors(glm::vec3 start, glm::vec3 dest);

class PerfTimer {
    public:
        explicit PerfTimer(std::string_view label);
        ~PerfTimer();

    private:
        std::string_view mLabel;
        std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
};

#endif  //  TEST_WGPU_UTILS_H
