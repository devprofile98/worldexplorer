#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include <limits>

#include "glm/fwd.hpp"
#include "gpu_buffer.h"
#define DEVELOPMENT_BUILD 1

#include <array>
#include <filesystem>
#include <numeric>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "instance.h"
#include "mesh.h"
#include "texture.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

// forward declaration
class Application;

/*
 * hold the object specific configuration for rendering in shader
 */
struct ObjectInfo {
        glm::mat4 transformation;
        uint32_t isFlat;
        uint32_t useTexture;
        uint32_t isFoliage;
        uint32_t instanceOffsetId;
        uint32_t isSelected;
        std::array<uint32_t, 3> offset;
};

// Hold the properties and needed object to represents the object transformation
class Transform {
    public:
        Transform& moveBy(const glm::vec3& m);
        Transform& moveTo(const glm::vec3& m);
        Transform& scale(const glm::vec3& s);
        Transform& rotate(const glm::vec3& around, float degree);

        glm::vec3& getPosition();
        glm::vec3& getScale();
        glm::mat4& getTranformMatrix();

        ObjectInfo mObjectInfo;

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

class DebugUI {
    public:
        virtual void userInterface() {}
};

// Represents an Axis aligned bounding box
struct AABB {
        using FloatMax = std::numeric_limits<float>;
        glm::vec3 min = glm::vec3{FloatMax::max(), FloatMax::max(), FloatMax::max()};
        glm::vec3 max = glm::vec3{FloatMax::lowest(), FloatMax::lowest(), FloatMax::lowest()};

        float calculateVolume();
        glm::vec3 getAABBSize();
};

class BaseModel : public Transform, public Drawable, public AABB, public DebugUI {
    public:
        BaseModel()
            : Transform({}, {glm::vec3{0.0}}, glm::vec3{1.0}, glm::mat4{1.0}, glm::mat4{1.0}, glm::mat4{1.0},
                        glm::mat4{1.0}) {};

        std::string mName;
        const std::string& getName();

        /*Buffer getVertexBuffer();*/
        Buffer getIndexBuffer();
        Buffer mIndexBuffer = {};
        virtual size_t getVertexCount() const;
        void setTransparent(bool value = true);
        bool isTransparent();
        Texture* getDiffuseTexture();
        std::map<int, Mesh> mMeshes;
        size_t instances = 1;
        Instance* instance = nullptr;
        void setInstanced(Instance* instance);
        void selected(bool selected = false);
	std::pair<glm::vec3, glm::vec3> getWorldMin();

    private:
        bool mIsTransparent = false;
};

class Model : public BaseModel {
    public:
        Model();

        Model& load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout);
        Model& uploadToGPU(Application* app);
        void draw(Application* app, WGPURenderPassEncoder encoder,
                  std::vector<WGPUBindGroupEntry>& bindingData) override;

        Model& setFoliage();
        Model& useTexture(bool use = true);

        // Getters
        void createSomeBinding(Application* app);
        size_t getInstaceCount();

#ifdef DEVELOPMENT_BUILD
        // Common User-Interface to interact with Object in the Development state
        // This function only gets called in development of the project
        void userInterface() override;
#endif  // DEVELOPMENT_BUILD

    private:
        Buffer offset_buffer = {};
        Texture* mSpecularTexture = nullptr;
        WGPUBindGroup mBindGroup = nullptr;
        WGPUBindGroup ggg = {};
        bool mIsLoaded = false;
};

#endif  //! WEBGPUTEST_MODEL_H
