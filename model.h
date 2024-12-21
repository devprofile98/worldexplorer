#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include <array>
#include <filesystem>
#include <numeric>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "texture.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

// forward declaration
class Application;

struct VertexAttributes {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;
};

/*
 * hold the object specific configuration for rendering in shader
 */
struct ObjectInfo {
        glm::mat4 transformation;
        uint32_t isFlat;
        std::array<uint32_t, 3> padding;
};

struct AABB {
        glm::vec3 min = glm::vec3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max()};
        glm::vec3 max = glm::vec3{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                                  std::numeric_limits<float>::lowest()};
};

class Model {
    public:
        Model();

        Model& load(std::string name, WGPUDevice device, WGPUQueue queue, const std::filesystem::path& path,
                    WGPUBindGroupLayout layout);
        Model& moveBy(const glm::vec3& m);
        Model& moveTo(const glm::vec3& m);
        Model& scale(const glm::vec3& s);
        Model& uploadToGPU(WGPUDevice device, WGPUQueue queue);
        size_t getVertexCount() const;
        float calculateVolume();

        WGPUBuffer getVertexBuffer();
        WGPUBuffer getIndexBuffer();

        void draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData);

        void draw(Application* app);

        // Getters
        glm::mat4 getModelMatrix();
        glm::vec3& getPosition();
        glm::vec3& getScale();
        AABB& getAABB();
        glm::vec3 getAABBSize();
        const std::string& getName();

        void createSomeBinding(Application* app);

    private:
        std::string mName;
        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mRotationMatrix;
        ObjectInfo mObjectInfo;
        AABB mBoundingBox;

        Texture* mTexture;
        WGPUTextureView mTextureView = nullptr;
        WGPUBindGroup mBindGroup;
        WGPUBuffer mUniformBuffer;

        WGPUBindGroup ggg = {};
        glm::vec3 mPosition;
        glm::vec3 mScale;

        std::vector<VertexAttributes> mVertexData;
        WGPUBuffer mVertexBuffer = {};
        WGPUBuffer mIndexBuffer = {};

        bool mIsLoaded = false;
};

#endif  //! WEBGPUTEST_MODEL_H