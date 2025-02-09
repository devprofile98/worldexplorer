#ifndef WEBGPUTEST_TRANSPARENCY_PASS_H
#define WEBGPUTEST_TRANSPARENCY_PASS_H

#include <utility>
#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "gpu_buffer.h"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

struct LinkedListElement {
        uint32_t color;
        float alpha = 0.0;
        uint32_t next = 0;
};

class TransparencyPass {
    public:
        void render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder,
                    WGPUTextureView opaqueDepthTextureView);
        TransparencyPass(Application* app);
        void initializePass();
        // Getters
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        Pipeline* getPipeline();
        WGPURenderPassDepthStencilAttachment mRenderPassDepthStencilAttachment;
        std::pair<WGPUBuffer, WGPUBuffer> getSSBOBuffers();

        std::vector<uint32_t> headsData;
        std::vector<LinkedListElement> linkedListData;

    private:
        Application* mApp;

        // pipeline
        Pipeline* mRenderPipeline;
        // render pass
        WGPURenderPassDescriptor mRenderPassDesc;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{5};

        Buffer mUniformBuffer;
        Buffer mHeadsBuffer;
        Buffer mLinkedlistBuffer;

        // textures and views
        WGPUTextureView mDepthTextureView;
        WGPUTextureView mShadowDepthTextureView;
        WGPUTexture mDepthTexture;
        WGPUTexture mShadowDepthTexture;

        // buffers
        /*WGPUBuffer mSceneUniformBuffer;*/
        // scene
        /*Scene mScene;*/
};

#endif  // !WEBGPUTEST_TRANSPARENCY_PASS_H
