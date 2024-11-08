#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>

#include "binding_group.h"
#include "camera.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

struct MyUniform {
        glm::mat4 projectMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 modelMatrix;
        glm::vec4 color;
        float time;
        float _padding[3];

        void setCamera(const Camera& camera);
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
        Model boat_model{};
        Model tower_model{};

        bool initGui();                                    // called in onInit
        void terminateGui();                               // called in onFinish
        void updateGui(WGPURenderPassEncoder renderPass);  // called in onFrame

        // Getters
        RendererResource& getRendererResource();
        BindingGroup& getBindingGroup();
        WGPUBuffer& getUniformBuffer();
        MyUniform& getUniformData();

    private:
        Camera mCamera;

        WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter) const;
        WGPUTextureView getNextSurfaceTextureView();
        void initializePipeline();
        void initializeBuffers();

        RendererResource mRendererResource;
        WGPUTextureFormat mSurfaceFormat = WGPUTextureFormat_Undefined;

        WGPURenderPipeline mPipeline;
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{3};
        WGPUBindGroupDescriptor mBindGroupDescriptor = {};
        // WGPUBindGroup mBindGroup;
        WGPUBuffer mBuffer1, mBuffer2;
        WGPUBuffer mUniformBuffer;
        MyUniform mUniforms;
        uint32_t mIndexCount;

        WGPUTextureView mDepthTextureView;
        WGPUTexture mDepthTexture;
};

#endif  // TEST_WGPU_APPLICTION_H