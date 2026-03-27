#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>
#include <filesystem>
#include <vector>

#include "binding_group.h"
#include "camera.h"
#include "gpu_buffer.h"
#include "material.h"
#include "mesh.h"
#include "terrain_pass.h"
#include "texture.h"
#include "utils.h"
#include "webgpu/webgpu.h"

// Forward Declarations
class ShadowPass;
class InstanceManager;
class LightManager;
class DepthPrePass;
class TransparencyPass;
class Texture;
struct LineEngine;
class SkyBox;
class Pipeline;
class CompositionPass;
class WaterPass;
class ParticleSystemsManager;
struct World;
template <typename K, typename V>
class Registry;  // Forward declaration
template <typename W>
class Window;
struct RendererResource;
struct Editor;

/*
 * Corresponding data for wgsl struct representing Directional light in scene
 */
struct LightingUniforms {
        std::array<glm::vec4, 2> directions;
        std::array<glm::vec4, 2> colors;
};

class Application {
    public:
        Application(const char* runningBinaryPath, const std::string& sceneFile = "world.json");
        bool initialize(const char* windowName, uint16_t width, uint16_t height);
        void terminate();
        void mainLoop();
        bool isRunning();
        void onResize();

        void terminateSwapChain();
        void terminateDepthBuffer();

        bool initGui();
        void terminateGui();

        void updateGui(WGPURenderPassEncoder renderPass, double time);

        // Getters
        RendererResource& getRendererResource();
        BindingGroup& getBindingGroup();
        Buffer& getUniformBuffer();
        CameraInfo& getUniformData();
        const WGPUBindGroupLayout& getObjectBindGroupLayout() const;
        const WGPUBindGroupLayout* getBindGroupLayouts() const;
        const std::vector<WGPUBindGroupEntry> getDefaultTextureBindingData() const;
        WGPUTextureView getDepthStencilTarget();
        WGPUTextureView getColorTarget();
        Pipeline* getPipeline();

        std::filesystem::path getWorkingDirectoryPath() const;
        std::filesystem::path getBinaryPathAbsolute() const;
        std::filesystem::path getBinaryPathRelative() const;

        Camera& getCamera();
        WGPUTextureFormat getTextureFormat();
        WGPUSampler getDefaultSampler();

        std::pair<size_t, size_t> getWindowSize();
        void setWindowSize(size_t width, size_t height);

        void initializePipeline();
        void initializeBuffers();

        InstanceManager* mInstanceManager;

        WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
        std::array<WGPUBindGroupLayout, 7> mBindGroupLayouts;

        // textures
        Texture* mDefaultDiffuse = nullptr;
        Texture* mDefaultMetallicRoughness = nullptr;
        Texture* mDefaultNormalMap = nullptr;

        Registery<std::string, Texture>* mTextureRegistery;
        // Registery<std::string, Material>* mMaterialRegistery;
        MaterialRegistery* mMaterialRegistery;

        BindingGroup mDefaultSkiningData = {};
        BindingGroup mDefaultTextureBindingGroup = {};
        BindingGroup mDefaultCameraIndexBindgroup = {};
        BindingGroup mDefaultClipPlaneBG = {};
        BindingGroup mDefaultVisibleBuffer = {};

        Editor* mEditor;
        BaseModel* mSelectedModel = nullptr;
        Buffer mLightBuffer;
        Buffer mVisibleIndexBuffer;
        Buffer mDefaultBoneFinalTransformData;
        Buffer mDefaultMeshGlobalTransformData;
        std::vector<WGPUBindGroupEntry> mBindingData{20};
        WGPUTextureView mCurrentTargetView;
        LineEngine* mLineEngine;
        SkyBox* mSkybox;
        Window<GLFWwindow>* mWindow;
        DebugBox debugbox;

        // private:
        Camera mCamera;
        LightingUniforms mLightingUniforms;
        LightManager* mLightManager;
        Pipeline* mPipeline;
        Pipeline* mStenctilEnabledPipeline;

        ShadowPass* mShadowPass;
        DepthPrePass* mDepthPrePass;
        TransparencyPass* mTransparencyPass;
        CompositionPass* mCompositionPass;
        TerrainPass* mTerrainPass;
        OutlinePass* mOutlinePass;
        ViewPort3DPass* m3DviewportPass;

        WaterPass* mWaterRenderPass;
        WGPUSampler mDefaultSampler;

        RendererResource* mRendererResource;
        WGPUTextureFormat mSurfaceFormat = WGPUTextureFormat_Undefined;

        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mDefaultTextureBindingData{3};
        std::vector<WGPUBindGroupEntry> mDefaultCameraIndexBindingData{1};
        std::vector<WGPUBindGroupEntry> mDefaultClipPlaneBGData{1};
        std::vector<WGPUBindGroupEntry> mDefaultVisibleBGData{1};
        std::vector<WGPUBindGroupEntry> mDefaultBoneTransformations{1};
        WGPUBindGroupDescriptor mBindGroupDescriptor = {};
        Buffer mUniformBuffer;
        Buffer mDirectionalLightBuffer;
        Buffer mLightSpaceTransformation;
        Buffer mTimeBuffer;
        Buffer mDefaultCameraIndex;
        Buffer mDefaultClipPlaneBuf;
        glm::vec4 mDefaultPlane{0.0, 0.0, 1.0, -100};

        CameraInfo mUniforms;

        WGPUTextureView mDepthTextureView;
        WGPUTextureView mDepthTextureViewDepthOnly;
        Texture* mDepthTexture;

        WGPURenderPipelineDescriptor mPipelineDescriptor;
        World* mWorld;
        ParticleSystemsManager* mParticleSystemsManager;

    private:
        bool initDepthBuffer();
        std::filesystem::path mCWDPath;
        std::filesystem::path mBinaryPath;
        std::string mSceneFilePath;
};

#endif  // TEST_WGPU_APPLICTION_H
