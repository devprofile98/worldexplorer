
#ifndef WORLD_EXPLORER_CORE_FULL_QUAD_CONVERTER_PASS_H
#define WORLD_EXPLORER_CORE_FULL_QUAD_CONVERTER_PASS_H

#include "binding_group.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"

class Application;
class NewRenderPass;

class FullQuadConverter {
    public:
        void initialize(Application* app, WGPUTextureView sourceTexture, WGPUSampler sampler);
        Pipeline* getPipeline();
        void executePass(Texture* target);

    private:
        Pipeline* mPipeline = nullptr;
        Application* mApp = nullptr;

        Buffer mVertexBuffer;
        NewRenderPass* mRenderPass = nullptr;
        BindingGroup mBindGroup{};
        VertexBufferLayout mVertexBufferLayout;
        std::vector<WGPUBindGroupEntry> mBindingData;
};

#endif
