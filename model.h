#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include <filesystem>
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

class Model {
    public:
        Model();

        Model& load(WGPUDevice device, WGPUQueue queue, const std::filesystem::path& path, WGPUBindGroupLayout layout);
        Model& moveBy(const glm::vec3& m);
        Model& moveTo(const glm::vec3& m);
        Model& scale(const glm::vec3& s);
        Model& uploadToGPU(WGPUDevice device, WGPUQueue queue);
        size_t getVertexCount() const;

        WGPUBuffer getVertexBuffer();
        WGPUBuffer getIndexBuffer();

        void draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData);

        void draw(Application* app);

        // Getters
        glm::mat4 getModelMatrix();
        glm::vec3& getPosition();
        glm::vec3& getScale();

        void createSomeBinding(Application* app);

    private:
        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mRotationMatrix;
        glm::mat4 mModelMatrix;

        Texture* mTexture;
        WGPUTextureView mTextureView;
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