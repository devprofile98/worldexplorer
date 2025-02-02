#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include "glm/fwd.hpp"
#include "gpu_buffer.h"
#define DEVELOPMENT_BUILD 1

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

// Hold the properties and needed object to represents the object transformation
class Transform {
    public:
        Transform& moveBy(const glm::vec3& m);
        Transform& moveTo(const glm::vec3& m);
        Transform& scale(const glm::vec3& s);

        glm::vec3& getPosition();
        glm::vec3& getScale();
        glm::mat4& getTranformMatrix();

        glm::vec3 mPosition;
        glm::vec3 mScale;

        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mRotationMatrix;

        glm::mat4 mTransformMatrix;
};

class Drawable {
    public:
        Drawable();
	void configure(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder,
                          std::vector<WGPUBindGroupEntry>& bindingData);

	Buffer& getUniformBuffer();
    private:
        Buffer mUniformBuffer;
};

// Shader-equivalant struct for vertex data
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

// Represents an Axis aligned bounding box
struct AABB {
        glm::vec3 min = glm::vec3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                                  std::numeric_limits<float>::max()};
        glm::vec3 max = glm::vec3{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                                  std::numeric_limits<float>::lowest()};
};

class Model : public Transform, public Drawable {
    public:
        Model();

        Model& load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout);
        Model& uploadToGPU(WGPUDevice device, WGPUQueue queue);
        size_t getVertexCount() const;
        float calculateVolume();

        WGPUBuffer getVertexBuffer();
        WGPUBuffer getIndexBuffer();

        void draw(Application* app, WGPURenderPassEncoder encoder,
                          std::vector<WGPUBindGroupEntry>&  bindingData) override;

        // Getters
        glm::mat4 getModelMatrix();

        AABB& getAABB();
        glm::vec3 getAABBSize();
        const std::string& getName();

        void createSomeBinding(Application* app);
        ObjectInfo mObjectInfo;

#ifdef DEVELOPMENT_BUILD
        // Common User-Interface to interact with Object in the Development state
        // This function only gets called in development of the project
        void userInterface();
#endif  // DEVELOPMENT_BUILD

    private:
        std::string mName;
        AABB mBoundingBox;

        Texture* mTexture = nullptr;
        Texture* mSpecularTexture = nullptr;
        WGPUBindGroup mBindGroup = nullptr;
        /*WGPUBuffer mUniformBuffer;*/
        /*Buffer mUniformBuffer;*/

        WGPUBindGroup ggg = {};
        std::vector<VertexAttributes> mVertexData;
        WGPUBuffer mVertexBuffer = {};
        WGPUBuffer mIndexBuffer = {};

        bool mIsLoaded = false;
};

#endif  //! WEBGPUTEST_MODEL_H
