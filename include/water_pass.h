

#ifndef WORLD_EXPLORER_WATER_PASS_H
#define WORLD_EXPLORER_WATER_PASS_H

#include <webgpu/webgpu.h>

#include "gpu_buffer.h"
#include "renderpass.h"

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
        BindingGroup mDefaultClipPlaneBG = {};

        std::vector<WGPUBindGroupEntry> mDefaultCameraIndexBindingData{1};
        std::vector<WGPUBindGroupEntry> mDefaultClipPlaneBGData{1};
        Buffer mDefaultCameraIndex;
        Buffer mDefaultClipPlaneBuf;
        glm::vec4 mDefaultPlane{0.0, 0.0, -1.0, -3.5};
        WGPUBindGroupLayout layout;

        void createRenderPass(WGPUTextureFormat textureFormat) override;
};

class WaterRefractionPass : public RenderPass {
    public:
        WaterRefractionPass(Application* app);

        Texture* mRenderTarget;
        Texture* mDepthTexture;
        WGPUTextureView mRenderTargetView;
        WGPUTextureView mDepthTextureView;

        Application* mApp;

        BindingGroup mDefaultClipPlaneBG = {};
        Buffer mDefaultClipPlaneBuf;
        std::vector<WGPUBindGroupEntry> mDefaultClipPlaneBGData{1};
        glm::vec4 mDefaultPlane{0.0, 0.0, 1.0, 3.5};

        // BindingGroup mDefaultCameraIndexBindgroup = {};
        // std::vector<WGPUBindGroupEntry> mDefaultCameraIndexBindingData{1};
        // Buffer mDefaultCameraIndex;
        // WGPUBindGroupLayout layout;

        void createRenderPass(WGPUTextureFormat textureFormat) override;
};

class WaterPass : public RenderPass {
    public:
        WaterPass(Application* app, Texture* renderTarget, Texture* refractionTarget);

        // Texture* mRenderTarget;
        // Texture* mDepthTexture;
        // WGPUTextureView mRenderTargetView;
        // WGPUTextureView mDepthTextureView;

        Application* mApp;

        BindingGroup mWaterTextureBindGroup = {};
        std::vector<WGPUBindGroupEntry> mWaterTextureBindngData{2};
        WGPUBindGroupLayout mBindGroupLayout;

        void createRenderPass(WGPUTextureFormat textureFormat) override;
};

#endif  //! WORLD_EXPLORER_WATER_PASS_H
