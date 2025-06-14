
#ifndef WEBGPUTEST_TERRAIN_PASS_H
#define WEBGPUTEST_TERRAIN_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "shadow_pass.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

class RenderPass {
    public:
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        void setRenderPassDescriptor(WGPURenderPassDescriptor desc);
        Pipeline* getPipeline();

    private:
        // render pass
        WGPURenderPassDescriptor mRenderPassDesc;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData;

        Texture* mRenderTarget;
        // buffers
        Buffer mSceneUniformBuffer;

        // scene
        std::vector<Scene> mScenes;

    protected:
        Pipeline* mRenderPipeline;
        virtual void createRenderPass(WGPUTextureFormat textureFormat) = 0;
};

class TerrainPass : public RenderPass {
        void createRenderPass(WGPUTextureFormat textureFormat) override;

    public:
        explicit TerrainPass(Application* app);
        Pipeline* create(WGPUTextureFormat textureFormat);

    private:
        Application* mApp;
};

#endif  // !WEBGPUTEST_TERRAIN_PASS_H
