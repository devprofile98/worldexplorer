

#ifndef WORLD_EXPLORER_WATER_PASS_H
#define WORLD_EXPLORER_WATER_PASS_H

#include "gpu_buffer.h"
#include "renderpass.h"
#include "webgpu.h"

class Application;

class WaterReflectionPass : public RenderPass {
    public:
        WaterReflectionPass(Application* app);

        Texture* mRenderTarget;
        Texture* mDepthTexture;
        WGPUTextureView mRenderTargetView;
        WGPUTextureView mDepthTextureView;

        Application* mApp;

        BindingGroup mDefaultCameraIndexBindgroup = {};
        std::vector<WGPUBindGroupEntry> mDefaultCameraIndexBindingData{1};
        Buffer mDefaultCameraIndex;
        WGPUBindGroupLayout layout;

        void createRenderPass(WGPUTextureFormat textureFormat) override;
};

class WaterPass : public RenderPass {
    public:
        WaterPass(Application* app);

        // Texture* mRenderTarget;
        // Texture* mDepthTexture;
        // WGPUTextureView mRenderTargetView;
        // WGPUTextureView mDepthTextureView;

        Application* mApp;

        void createRenderPass(WGPUTextureFormat textureFormat) override;
};

#endif  //! WORLD_EXPLORER_WATER_PASS_H
