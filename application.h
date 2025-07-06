#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>

#include "binding_group.h"
#include "camera.h"
#include "composition_pass.h"
#include "gpu_buffer.h"
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
#include "webgpu/webgpu.h"
#include "editor.h"

class ShadowPass;

struct MyUniform {
        glm::mat4 projectMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 modelMatrix;
        glm::vec4 color;
        glm::vec3 cameraWorldPosition;
        float time;
        float _padding[3];

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

        // Mouse events
        void onMouseMove(double xpos, double ypos);
        void onMouseButton(int button, int action, int mods);
        void onScroll(double xoffset, double yoffset);

        bool initSwapChain();
        bool initDepthBuffer();
        void terminateSwapChain();
        void terminateDepthBuffer();
        void updateViewMatrix();
        void updateDragInertia();

        /*Model water{};*/

        // std::vector<BaseModel*> mLoadedModel;
        BaseModel* getModelCounter();

        InstanceManager* mInstanceManager;
        std::vector<glm::vec3> terrainData = {};
        bool initGui();                                    // called in onInit
        void terminateGui();                               // called in onFinish
        void updateGui(WGPURenderPassEncoder renderPass);  // called in onFrame

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
        std::array<WGPUBindGroupLayout, 3> mBindGroupLayouts;
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
        Editor mEditor;

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
        WGPUSampler mDefaultSampler;
        WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter) const;
        WGPUTextureView getNextSurfaceTextureView();

        void initializePipeline();
        void initializeBuffers();

        RendererResource mRendererResource;
        WGPUTextureFormat mSurfaceFormat = WGPUTextureFormat_Undefined;

        // WGPURenderPipeline mPipeline;
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{20};
        std::vector<WGPUBindGroupEntry> mDefaultTextureBindingData{3};
        WGPUBindGroupDescriptor mBindGroupDescriptor = {};
        // WGPUBindGroup mBindGroup;
        WGPUBuffer mUniformBuffer;
        WGPUBuffer mUniformBufferTransform;
        WGPUBuffer mDirectionalLightBuffer;
        Buffer mLightSpaceTransformation;
        Buffer mTimeBuffer;

        // WGPUBuffer mPointlightBuffer = {};
        MyUniform mUniforms;
        uint32_t mIndexCount;

        WGPUTextureView mDepthTextureView;
        WGPUTextureView mDepthTextureViewDepthOnly;
        // WGPUTextureView mShadowDepthTextureView;
        Texture* mDepthTexture;

        // WGPUTexture mShadowDepthTexture;

        WGPURenderPipelineDescriptor mPipelineDescriptor;

        BaseModel* mSelectedModel = nullptr;
        SkyBox* mSkybox;
};

#endif  // TEST_WGPU_APPLICTION_H
