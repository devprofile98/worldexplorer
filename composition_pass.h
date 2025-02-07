#ifndef WEBGPUTEST_COMPOSITION_PASS_H
#define WEBGPUTEST_COMPOSITION_PASS_H

#include <vector>

#include "binding_group.h"
#include "gpu_buffer.h"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"

class Application;

class CompositionPass {
    public:
        void render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder,
                    WGPURenderPassColorAttachment* colorAttachment);
        CompositionPass(Application* app);
        void initializePass();
        // Getters
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        Pipeline* getPipeline();
        WGPURenderPassDepthStencilAttachment mRenderPassDepthStencilAttachment;
        WGPURenderPassColorAttachment mRenderPassColorAttachment;

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
        /*WGPUTextureView mDepthTextureView;*/
        /*WGPUTextureView mShadowDepthTextureView;*/
        /*WGPUTexture mDepthTexture;*/
        /*WGPUTexture mShadowDepthTexture;*/
        /**/
};

#endif  //! WEBGPUTEST_COMPOSITION_PASS_H
