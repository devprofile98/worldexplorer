#ifndef WEBGPUTEST_TRANSPARENCY_PASS_H
#define WEBGPUTEST_TRANSPARENCY_PASS_H

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

class TransparencyPass {
    public:
        TransparencyPass(Application* app);
        void initializePass();

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
