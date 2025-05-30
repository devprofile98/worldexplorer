#ifndef WEBGPUTEST_SHADOW_PASS_H
#define WEBGPUTEST_SHADOW_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

struct Scene {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
};

void printMatrix(const glm::mat4& matrix);

class ShadowPass {
    public:
        ShadowPass(Application* app);
        void createRenderPass();

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
        WGPURenderPassColorAttachment mRenderPassColorAttachment = {};
        WGPURenderPassColorAttachment mRenderPassColorAttachment2 = {};
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
        std::vector<WGPUBindGroupEntry> mTextureBindingData{2};

	std::vector<WGPUBindGroup> mBindgroups = {nullptr, nullptr};

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTextureView mShadowDepthTextureView;
        WGPUTextureView mShadowDepthTextureView2;
        WGPUTexture mDepthTexture;
        WGPUTexture mShadowDepthTexture;
        WGPUTexture mShadowDepthTexture2;
        Texture* render_target;
        // buffers
        Buffer mSceneUniformBuffer;

        // scene
        std::vector<Scene> mScenes;
};

#endif  // WEBGPUTEST_SHADOW_PASS_H
