#ifndef WEBGPUTEST_SHADOW_PASS_H
#define WEBGPUTEST_SHADOW_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "renderpass.h"
#include "texture.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

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

        // Getters
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        WGPURenderPassDescriptor* getRenderPassDescriptor2();

        Pipeline* getPipeline();
        WGPUTextureView getShadowMapView();
        WGPUTextureView getShadowMapView2();
        void render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder, size_t which);
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
        ShadowFrustum* mNearFrustum;
        ShadowFrustum* mFarFrustum;


    private:
        Application* mApp;
        // pipeline
        Pipeline* mRenderPipeline;
        // render pass


        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData;
        BindingGroup mTextureBindingGroup;
        std::vector<WGPUBindGroupEntry> mTextureBindingData{3};

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTexture mDepthTexture;
        Texture* render_target;
        Texture* mShadowDepthTexture;
        Texture* mShadowDepthTexture2;
        // buffers
        Buffer mSceneUniformBuffer;

        Scene calculateFrustumScene(const std::vector<glm::vec4> frustum);
        // scene
        std::vector<Scene> mScenes;
};

class ShadowFrustum {
    public:
        ShadowFrustum(Application* app, size_t width, size_t height, Texture* renderTarget = nullptr);
        /*WGPUTextureView getShadowMapView();*/
        /*WGPUTextureView getShadowMapViewArray();*/
        WGPURenderPassDescriptor* getRenderPassDescriptor();

        Texture* mShadowDepthTexture;
        Texture* mRenderTarget = nullptr;
        WGPURenderPassDescriptor mRenderPassDesc;
        Application* mApp;
        ColorAttachment mColorAttachment{};
        DepthStencilAttachment mDepthStencilAttachment{};
        size_t mWidth = 1024;
        size_t mHeight = 1024;
};

#endif  // WEBGPUTEST_SHADOW_PASS_H
