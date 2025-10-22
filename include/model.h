#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include <assimp/scene.h>

#include <limits>
#include <unordered_map>

#include "glm/fwd.hpp"
#include "gpu_buffer.h"
#define DEVELOPMENT_BUILD 1

#include <array>
#include <assimp/Importer.hpp>
#include <filesystem>
#include <format>
#include <iostream>
#include <numeric>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "instance.h"
#include "mesh.h"
// #include "texture.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

// forward declaration
class Application;
class Texture;
class BaseModel;

struct alignas(4) DrawIndexedIndirectArgs {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t firstInstance;
};

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

/*
 * hold the object specific configuration for rendering in shader
 */
struct alignas(16) ObjectInfo {
        glm::mat4 transformation;
        uint32_t isFlat;
        uint32_t useTexture;
        uint32_t isFoliage;
        uint32_t instanceOffsetId;
        uint32_t isSelected;
        uint32_t materialProps;
        float roughness;
        uint32_t isAnimated;
        glm::vec3 uvMultiplier;

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

// Hold the properties and needed object to represents the object transformation
class Transform {
        friend class BaseModel;

    public:
        Transform& moveBy(const glm::vec3& m);
        Transform& moveTo(const glm::vec3& m);
        Transform& scale(const glm::vec3& s);
        Transform& rotate(const glm::vec3& around, float degree);

        glm::vec3& getPosition();
        glm::vec3& getScale();
        glm::mat4& getLocalTransform();

        ObjectInfo mObjectInfo;

        glm::vec3 mPosition;
        glm::vec3 mEulerRotation;
        glm::vec3 mScale;

        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mRotationMatrix;

        glm::mat4 mTransformMatrix;
        glm::quat mOrientation;
        bool mDirty = true;

    private:
        glm::mat4 getGlobalTransform(BaseModel* parent);
};

class Drawable {
    public:
        Drawable();
        void configure(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder);

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

class BaseModel : public Drawable, public AABB, public DebugUI {
    public:
        BaseModel()
            : mTransform({}, {glm::vec3{0.0}}, glm::vec3{0.0}, glm::vec3{1.0}, glm::mat4{1.0}, glm::mat4{1.0},
                         glm::mat4{1.0}, glm::mat4{1.0}, glm::vec3{0.0, 0.0, 1.0}) {};

        std::string mName;
        const std::string& getName();

        /*Buffer getVertexBuffer();*/
        Buffer getIndexBuffer();
        virtual size_t getVertexCount() const;
        void setTransparent(bool value = true);
        bool isTransparent();
        Texture* getDiffuseTexture();
        void setInstanced(Instance* instance);
        void selected(bool selected = false);
        bool isSelected() const;
        std::pair<glm::vec3, glm::vec3> getWorldMin();
        std::pair<glm::vec3, glm::vec3> getWorldSpaceAABB();
        Buffer mIndexBuffer = {};
        std::map<int, Mesh> mMeshes;
        size_t instances = 1;
        Instance* instance = nullptr;
        std::vector<BaseModel*> mChildrens{};

        /* Scene graph related property */
        void addChildren(BaseModel* child);
        glm::mat4 getGlobalTransform();
        void update();
        BaseModel* mParent = nullptr;
        Transform mTransform;

    private:
        bool mIsTransparent = false;
};

class Model : public BaseModel {
    public:
        Model();

        // void processMesh(Application* app, aiMesh* mesh, const aiScene* scene, unsigned int meshId);
        void processMesh(Application* app, aiMesh* mesh, const aiScene* scene, unsigned int meshId,
                         const glm::mat4& globalTransform);
        // void processNode(Application* app, aiNode* node, const aiScene* scene);
        void processNode(Application* app, aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
        Model& load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout);
        Model& uploadToGPU(Application* app);
        void draw(Application* app, WGPURenderPassEncoder encoder) override;

        Model& setFoliage();
        Model& useTexture(bool use = true);
        void update(Application* app, float dt);

        // Getters
        void createSomeBinding(Application* app, std::vector<WGPUBindGroupEntry> bindingData);
        // size_t getInstaceCount();
        WGPUBindGroup getObjectInfoBindGroup();

        Buffer mIndirectDrawArgsBuffer;
        Buffer mSkiningTransformationBuffer;

        std::unordered_map<std::string, aiNode*> mNodeCache;
        std::unordered_map<std::string, glm::mat4> mOffsetMatrixCache;
        std::vector<glm::mat4> mFinalTransformations;
        std::map<std::string, size_t> boneToIdx;
        void updateAnimation();
        void buildNodeCache(aiNode* node);
        std::vector<glm::vec3> mBonePosition;
        const aiScene* mScene;
        Assimp::Importer mImport;
        std::map<std::string, glm::mat4> globalMap;
        std::unordered_set<std::string> uniqueBones;  // Avoid duplicate spheres if bones are shared
        std::map<std::string, aiNodeAnim*> channelMap;
        // size_t mAnimationPoseCounter = 0;
        double mAnimationSecond = 0.0;
        double mAnimationDuration = 0.0;
        // WGPUBindGroup mSkiningBindGroup = {};
        WGPUBindGroupEntry mSkiningDataEntry;

#ifdef DEVELOPMENT_BUILD
        // Common User-Interface to interact with Object in the Development state
        // This function only gets called in development of the project
        void userInterface() override;
#endif  // DEVELOPMENT_BUILD

    private:
        Buffer offset_buffer = {};
        WGPUBindGroup mBindGroup = nullptr;
        WGPUBindGroup ggg = {};
        bool mIsLoaded = false;
};

#endif  //! WEBGPUTEST_MODEL_H
