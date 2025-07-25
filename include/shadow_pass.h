#ifndef WEBGPUTEST_SHADOW_PASS_H
#define WEBGPUTEST_SHADOW_PASS_H

#include <vector>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "model_registery.h"
#include "pipeline.h"
#include "renderpass.h"
#include "texture.h"

class Application;
class ShadowFrustum;

void printMatrix(const glm::mat4& matrix);

struct FrustumParams {
        float begin;
        float end;
};

class ShadowPass : public RenderPass {
    public:
        explicit ShadowPass(Application* app);
        Pipeline* create(WGPUTextureFormat textureFormat);

        void createRenderPass(WGPUTextureFormat textureFormat) override;
        void createRenderPass(WGPUTextureFormat textureFormat, size_t cascadeNumber);

        // Getters
        WGPURenderPassDescriptor* getRenderPassDescriptor(size_t index);

        Pipeline* getPipeline();
        WGPUTextureView getShadowMapView();
        void render(ModelRegistry::ModelContainer& models, WGPURenderPassEncoder encoder, size_t which);
        void renderAllCascades(WGPUCommandEncoder encoder);
        std::vector<Scene>& getScene();

        glm::vec3 lightPos = glm::vec3{0.0f};

        std::vector<glm::vec4> corners;
        std::vector<glm::vec4> mNear;
        std::vector<glm::vec4> mFar;

        std::vector<Scene> createFrustumSplits(std::vector<glm::vec4>& corners, std::vector<FrustumParams> params);
        ColorAttachment mRenderPassColorAttachment = {};
        ColorAttachment mRenderPassColorAttachment2;
        DepthStencilAttachment mRenderPassDepthStencil2;
        DepthStencilAttachment mRenderPassDepthStencil;
        float MinZ = 0.0f;
        // sub frustums
        /*ShadowFrustum* mNearFrustum;*/
        /*ShadowFrustum* mFarFrustum;*/
        std::vector<ShadowFrustum*> mSubFrustums;

    private:
        Application* mApp;
        // pipeline
        Pipeline* mRenderPipeline;
        // render pass
        size_t mNumOfCascades;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData;
        BindingGroup mTextureBindingGroup;
        std::vector<WGPUBindGroupEntry> mTextureBindingData{3};

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTexture mDepthTexture;
        Texture* mRenderTarget;
        Texture* mShadowDepthTexture;
        Texture* mShadowDepthTexture2;
        // buffers
        Buffer mSceneUniformBuffer;

        Scene calculateFrustumScene(const std::vector<glm::vec4> frustum, float farZ, size_t cascadeIdx);
        // scene
        std::vector<Scene> mScenes;
};

class ShadowFrustum {
    public:
        ShadowFrustum(Application* app, WGPUTextureView renderTarget, WGPUTextureView depthTexture);
        WGPURenderPassDescriptor* getRenderPassDescriptor();

        WGPUTextureView mShadowDepthTexture;
        WGPUTextureView mRenderTarget = nullptr;
        WGPURenderPassDescriptor mRenderPassDesc;
        Application* mApp;
        ColorAttachment mColorAttachment{};
        DepthStencilAttachment mDepthStencilAttachment{};
};

#endif  // WEBGPUTEST_SHADOW_PASS_H
