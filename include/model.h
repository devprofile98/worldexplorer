#ifndef WEBGPUTEST_MODEL_H
#define WEBGPUTEST_MODEL_H

#include <assimp/scene.h>

#include <limits>
#include <unordered_map>

// #include "animation.h"
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
struct Animation;
class Behaviour;
struct BoneSocket;
struct PhysicsComponent;

struct alignas(4) DrawIndexedIndirectArgs {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t firstInstance;
};

/*
 * hold the object specific configuration for rendering in shader
 */
struct alignas(16) ObjectInfo {
        glm::mat4 transformation;
        uint32_t instanceOffsetId;
        uint32_t isSelected;
        uint32_t isAnimated;
};

class Node {
    public:
        std::string mName;
        glm::mat4 mLocalTransform;
        Node* mParent;
        std::vector<Node*> mChildrens;
        std::vector<unsigned int> mMeshIndices;

        glm::mat4 getGlobalTransform() const;

        static inline Node* buildNodeTree(aiNode* ainode, Node* parent);
};

// Hold the properties and needed object to represents the object transformation
class Transform {
        friend class BaseModel;

    public:
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
        glm::quat mOrientation{};
        bool mDirty = true;

    private:
        glm::mat4 getGlobalTransform(BaseModel* parent);
};

class Drawable {
    public:
        Drawable();
        void configure(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder);
        virtual void drawHirarchy(Application* app, WGPURenderPassEncoder encoder);

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
        std::pair<glm::vec3, glm::vec3> getPhysicsAABB();

        void addChildren(BaseModel* child);
        glm::mat4 getGlobalTransform();
        void updateHirarchy();

        BaseModel& moveBy(const glm::vec3& m);
        BaseModel& moveTo(const glm::vec3& m);
        BaseModel& scale(const glm::vec3& s);
        BaseModel& rotate(const glm::vec3& around, float degree);
        BaseModel& rotate(const glm::quat& rot);

        /* Scene graph related property */
        BaseModel* mParent = nullptr;
        Transform mTransform;
        Node* mRootNode = nullptr;

        BoneSocket* mSocket = nullptr;

        Buffer mIndexBuffer = {};
        std::map<int, Mesh> mMeshes;
        std::unordered_map<int, Mesh> mFlattenMeshes;
        int mMeshNumber = 0;
        size_t instances = 1;
        Instance* instance = nullptr;
        std::vector<BaseModel*> mChildrens{};
        PhysicsComponent* mPhysicComponent = nullptr;

    private:
        bool mIsTransparent = false;
};

enum CoordinateSystem {
    Y_UP = 0,
    Z_UP = 1,
};

class Model : public BaseModel {
    public:
        Model(CoordinateSystem cs = Y_UP);

        void processMesh(Application* app, aiMesh* mesh, const aiScene* scene, unsigned int meshId,
                         const glm::mat4& globalTransform);
        void processNode(Application* app, aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
        Model& load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout);
        Model& uploadToGPU(Application* app);
        void draw(Application* app, WGPURenderPassEncoder encoder) override;
        void drawHirarchy(Application* app, WGPURenderPassEncoder encoder) override;
        void drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node);
        void internalDraw(Application* app, WGPURenderPassEncoder encoder, Node* node);

        Model& setFoliage();
        Model& useTexture(bool use = true);
        void update(Application* app, float dt, float physicSimulating = true);

        // Getters
        void createSomeBinding(Application* app, std::vector<WGPUBindGroupEntry> bindingData);
        // size_t getInstaceCount();
        WGPUBindGroup getObjectInfoBindGroup();
        void updateAnimation(float dt);

        Buffer mIndirectDrawArgsBuffer;
        Buffer mSkiningTransformationBuffer;
        Buffer mGlobalMeshTransformationBuffer;

        // std::unordered_map<std::string, aiNode*> mNodeCache;
        // void buildNodeCache(aiNode* node);

        Behaviour* mBehaviour = nullptr;
        const aiScene* mScene;
        Assimp::Importer mImport;

        Animation* anim;
        std::vector<glm::vec3> mBonePosition;
        std::vector<glm::mat4> mGlobalMeshTransformationData;
        WGPUBindGroupEntry mSkiningDataEntry;

#ifdef DEVELOPMENT_BUILD
        // Common User-Interface to interact with Object in the Development state
        // This function only gets called in development of the project
        void userInterface() override;
#endif  // DEVELOPMENT_BUILD

    private:
        CoordinateSystem mCoordinateSystem = Y_UP;
        Buffer offset_buffer = {};
        WGPUBindGroup mBindGroup = nullptr;
        WGPUBindGroup ggg = {};
        bool mIsLoaded = false;
};

#endif  //! WEBGPUTEST_MODEL_H
