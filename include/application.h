#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>
#include <vector>

#include "binding_group.h"
#include "camera.h"
#include "editor.h"
#include "gpu_buffer.h"
#include "terrain_pass.h"
#include "utils.h"
#include "water_pass.h"
#include "webgpu/webgpu.h"
// #include "window.h"

// Forward Declarations
class ShadowPass;
class InstanceManager;
class LightManager;
class DepthPrePass;
class TransparencyPass;
class Texture;
class LineEngine;
class SkyBox;
class Pipeline;
class CompositionPass;
//
template <typename W>
class Window;
struct RendererResource;

/*
 * Corresponding data for wgsl struct representing Directional light in scene
 */
struct LightingUniforms {
        std::array<glm::vec4, 2> directions;
        std::array<glm::vec4, 2> colors;
};

class Application {
    public:
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

        Camera& getCamera();
        WGPUTextureFormat getTextureFormat();
        WGPUSampler getDefaultSampler();

        std::pair<size_t, size_t> getWindowSize();
        void setWindowSize(size_t width, size_t height);

        void initializePipeline();
        void initializeBuffers();

        InstanceManager* mInstanceManager;

        WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
        std::array<WGPUBindGroupLayout, 6> mBindGroupLayouts;

        // textures
        Texture* mDefaultDiffuse = nullptr;
        Texture* mDefaultMetallicRoughness = nullptr;
        Texture* mDefaultNormalMap = nullptr;

        BindingGroup mDefaultSkiningData = {};
        BindingGroup mDefaultTextureBindingGroup = {};
        BindingGroup mDefaultCameraIndexBindgroup = {};
        BindingGroup mDefaultClipPlaneBG = {};
        BindingGroup mDefaultVisibleBuffer = {};

        Editor mEditor;
        BaseModel* mSelectedModel = nullptr;
        Buffer mLightBuffer;
        Buffer mVisibleIndexBuffer;
        Buffer mDefaultBoneFinalTransformData;
        std::vector<WGPUBindGroupEntry> mBindingData{20};
        WGPUTextureView mCurrentTargetView;
        LineEngine* mLineEngine;
        SkyBox* mSkybox;
        Window<GLFWwindow>* mWindow;

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

    private:
        bool initDepthBuffer();
};

#endif  // TEST_WGPU_APPLICTION_H
