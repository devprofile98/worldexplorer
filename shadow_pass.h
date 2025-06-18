#ifndef WEBGPUTEST_SHADOW_PASS_H
#define WEBGPUTEST_SHADOW_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "renderpass.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

void printMatrix(const glm::mat4& matrix);

class ShadowPass : public RenderPass {
    public:
        explicit ShadowPass(Application* app);
        Pipeline* create(WGPUTextureFormat textureFormat);

        void createRenderPass(WGPUTextureFormat textureFormat) override;

        // Getters
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        WGPURenderPassDescriptor* getRenderPassDescriptor2();

        Pipeline* getPipeline();
        WGPUTextureView getShadowMapView();
        WGPUTextureView getShadowMapView2();
        void render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder, size_t which);
        std::vector<Scene>& getScene();

        glm::vec3 lightPos = glm::vec3{0.0f};
        glm::vec3 center = glm::vec3{0.0f, 0.0f, 2.25f};

        std::vector<glm::vec4> corners;
        std::vector<glm::vec4> mNear;
        std::vector<glm::vec4> mFar;
        std::vector<glm::vec4> mMiddle;

        std::vector<Scene> createFrustumSplits(std::vector<glm::vec4>& corners, float length, float far_length,
                                               float distance, float dd);
        ColorAttachment mRenderPassColorAttachment = {};
        ColorAttachment mRenderPassColorAttachment2;
        DepthStencilAttachment mRenderPassDepthStencil2;
        DepthStencilAttachment mRenderPassDepthStencil;
        void createRenderPassDescriptor();
        void createRenderPassDescriptor2();
        float MinZ = 0.0f;

    private:
        Application* mApp;
        // pipeline
        Pipeline* mRenderPipeline;
        // render pass
        WGPURenderPassDescriptor mRenderPassDesc;
        WGPURenderPassDescriptor mRenderPassDesc2;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData;
        BindingGroup mTextureBindingGroup;
        std::vector<WGPUBindGroupEntry> mTextureBindingData{3};

        std::vector<WGPUBindGroup> mBindgroups = {nullptr, nullptr};

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTexture mDepthTexture;
        Texture* render_target;
        Texture* mShadowDepthTexture;
        Texture* mShadowDepthTexture2;
        // buffers
        Buffer mSceneUniformBuffer;

        // scene
        std::vector<Scene> mScenes;
};

#endif  // WEBGPUTEST_SHADOW_PASS_H
