#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>

#include "binding_group.h"
#include "camera.h"
#include "composition_pass.h"
#include "editor.h"
#include "gpu_buffer.h"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "pipeline.h"
#include "point_light.h"
#include "shapes.h"
#include "skybox.h"
#include "terrain_pass.h"
#include "texture.h"
#include "transparency_pass.h"
#include "utils.h"
#include "water_pass.h"
#include "webgpu/webgpu.h"

class ShadowPass;

struct MyUniform {
        glm::mat4 projectMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 modelMatrix;
        glm::vec4 color;
        glm::vec3 cameraWorldPosition;
        float time;
        // float _padding[3];

        void setCamera(Camera& camera);
};

/*
 * A struct to gather primitives for our renderer
 * Note: Pipelines and bindgroups are not part of this struct
 *       they could be included in some pipeline struct for each
 *       models that defines its own pipeline and render pass
 */
struct RendererResource {
        WGPUQueue queue;
        WGPUDevice device;
        WGPUSurface surface;
        GLFWwindow* window;
        WGPUCommandEncoder commandEncoder;
};

/*
 * Corresponding data for wgsl struct representing Directional light in scene
 */
struct LightingUniforms {
        std::array<glm::vec4, 2> directions;
        std::array<glm::vec4, 2> colors;
};

/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */

class Application {
    public:
        bool initialize();
        void terminate();
        void mainLoop();
        bool isRunning();
        void onResize();
        void updateProjectionMatrix();

        bool initSwapChain();
        bool initDepthBuffer();
        void terminateSwapChain();
        void terminateDepthBuffer();

        InputManager* mInputManager;
        InstanceManager* mInstanceManager;

        std::vector<glm::vec3> terrainData = {};

        bool initGui();
        void terminateGui();

        void updateGui(WGPURenderPassEncoder renderPass);

        // Getters
        RendererResource& getRendererResource();
        BindingGroup& getBindingGroup();
        WGPUBuffer& getUniformBuffer();
        MyUniform& getUniformData();
        const WGPUBindGroupLayout& getObjectBindGroupLayout() const;
        const WGPUBindGroupLayout* getBindGroupLayouts() const;
        const std::vector<WGPUBindGroupEntry> getDefaultTextureBindingData() const;
        WGPUBindGroup bindGrouptrans = {};
        glm::mat4 mtransmodel{1.0};
        WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
        std::array<WGPUBindGroupLayout, 6> mBindGroupLayouts;
        Camera& getCamera();
        WGPUTextureFormat getTextureFormat();
        WGPUSampler getDefaultSampler();
        // textures
        Texture* mLightViewSceneTexture = nullptr;
        Texture* mDefaultDiffuse = nullptr;
        Texture* mDefaultMetallicRoughness = nullptr;
        Texture* mDefaultNormalMap = nullptr;
        std::pair<size_t, size_t> getWindowSize();
        void setWindowSize(size_t width, size_t height);

        WGPUBuffer mBuffer1;
        BindingGroup mDefaultTextureBindingGroup = {};
        BindingGroup mDefaultCameraIndexBindgroup = {};
        BindingGroup mDefaultClipPlaneBG = {};
        BindingGroup mDefaultVisibleBuffer = {};
        // BindingGroup mDefaultVisibleBuffer2 = {};

        Editor mEditor;
        BaseModel* mSelectedModel = nullptr;
        // Buffer indirectDrawArgsBuffer;  // copy dst, map read
        Buffer mVisibleIndexBuffer;   // copy src, storage
        Buffer mVisibleIndexBuffer2;  // copy src, storage
        int ccounter = 0;

    private:
        size_t mWindowWidth = 1920;
        size_t mWindowHeight = 1080;
        Camera mCamera;
        Terrain terrain;
        LightingUniforms mLightingUniforms;
        LightManager* mLightManager;
        Pipeline* mPipeline;
        Pipeline* mStenctilEnabledPipeline;
        Cube* shapes;
        Plane* plane;

        ModelRegistry mViewportModelRegistery;

        /*Line* line;*/
        ShadowPass* mShadowPass;
        TransparencyPass* mTransparencyPass;
        CompositionPass* mCompositionPass;
        TerrainPass* mTerrainPass;
        OutlinePass* mOutlinePass;
        ViewPort3DPass* m3DviewportPass;
        WaterReflectionPass* mWaterPass;
        WaterRefractionPass* mWaterRefractionPass;
        WaterPass* mWaterRenderPass;
        WGPUSampler mDefaultSampler;
        WGPULimits GetRequiredLimits(WGPUAdapter adapter) const;
        WGPUTextureView getNextSurfaceTextureView();

        void initializePipeline();
        void initializeBuffers();

        RendererResource mRendererResource;
        WGPUTextureFormat mSurfaceFormat = WGPUTextureFormat_Undefined;

        // WGPURenderPipeline mPipeline;
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{20};
        std::vector<WGPUBindGroupEntry> mDefaultTextureBindingData{3};
        std::vector<WGPUBindGroupEntry> mDefaultCameraIndexBindingData{1};
        std::vector<WGPUBindGroupEntry> mDefaultClipPlaneBGData{1};
        std::vector<WGPUBindGroupEntry> mDefaultVisibleBGData{1};
        std::vector<WGPUBindGroupEntry> mDefaultVisibleBGData2{1};
        WGPUBindGroupDescriptor mBindGroupDescriptor = {};
        // WGPUBindGroup mBindGroup;
        WGPUBuffer mUniformBuffer;
        WGPUBuffer mUniformBufferTransform;
        WGPUBuffer mDirectionalLightBuffer;
        Buffer mLightSpaceTransformation;
        Buffer mTimeBuffer;
        Buffer mDefaultCameraIndex;
        Buffer mDefaultClipPlaneBuf;
        glm::vec4 mDefaultPlane{0.0, 0.0, 1.0, -100};

        // WGPUBuffer mPointlightBuffer = {};
        MyUniform mUniforms;
        uint32_t mIndexCount;

        WGPUTextureView mDepthTextureView;
        WGPUTextureView mDepthTextureViewDepthOnly;
        // WGPUTextureView mShadowDepthTextureView;
        Texture* mDepthTexture;

        // WGPUTexture mShadowDepthTexture;

        WGPURenderPipelineDescriptor mPipelineDescriptor;

        SkyBox* mSkybox;
};

#endif  // TEST_WGPU_APPLICTION_H
