#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <array>

#include "binding_group.h"
#include "camera.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
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

    private:
        Camera mCamera;

        WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter) const;
        WGPUTextureView getNextSurfaceTextureView();
        void initializePipeline();
        void initializeBuffers();

        WGPUQueue mQueue;
        WGPUDevice mDevice;
        WGPUSurface mSurface;
        GLFWwindow* mWindow;
        WGPURenderPipeline mPipeline;
        WGPUTextureFormat mSurfaceFormat = WGPUTextureFormat_Undefined;

        WGPUBindGroup mBindGroup;
        WGPUBuffer mBuffer1, mBuffer2;
        WGPUBuffer mUniformBuffer;
        WGPUBuffer mVertexBuffer, mIndexBuffer;

        BindingGroup mBindingGroup;

        MyUniform mUniforms;
        uint32_t mIndexCount;

        WGPUTextureView mDepthTextureView;
        WGPUTexture mDepthTexture;
};

#endif  // TEST_WGPU_APPLICTION_H