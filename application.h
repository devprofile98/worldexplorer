#ifndef TEST_WGPU_APPLICTION_H
#define TEST_WGPU_APPLICTION_H

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application {
    public:
        bool initialize();
        void terminate();
        void mainLoop();
        bool isRunning();

    private:
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
        WGPUBuffer mBuffer1, mBuffer2;

        WGPUBuffer mVertexBuffer;
        uint32_t mVertexCount;
};

#endif  // TEST_WGPU_APPLICTION_H