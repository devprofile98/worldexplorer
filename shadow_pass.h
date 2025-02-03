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
        void setupScene(const glm::vec3 lightPos);
        Pipeline* getPipeline();
        WGPUTextureView getShadowMapView();
        void render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder);
        Scene& getScene();

        glm::vec3 lightPos = glm::vec3{0.0f};

    private:
        Application* mApp;

        // pipeline
        Pipeline* mRenderPipeline;
        // render pass
        WGPURenderPassDescriptor mRenderPassDesc;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{5};

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTextureView mShadowDepthTextureView;
        WGPUTexture mDepthTexture;
        WGPUTexture mShadowDepthTexture;

        // buffers
        WGPUBuffer mSceneUniformBuffer;

        // scene
        Scene mScene;
};

#endif  // WEBGPUTEST_SHADOW_PASS_H
