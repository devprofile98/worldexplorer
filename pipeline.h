#ifndef TEST_WGPU_PIPELINE_H
#define TEST_WGPU_PIPELINE_H

#include <vector>

#include "utils.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

class Pipeline {
    public:
        Pipeline(std::vector<WGPUBindGroupLayout> bindGroupLayout, WGPURenderPipelineDescriptor desc);

        Pipeline& defaultConfiguration(Application* app, WGPUTextureFormat surfaceTexture);
        Pipeline& createPipeline(Application* app);

        // Getters
        WGPUPipelineLayout getPipelineLayout();
        WGPURenderPipeline getPipeline();
        WGPURenderPipelineDescriptor getDescriptor();

    private:
        WGPURenderPipelineDescriptor mDescriptor = {};
        std::vector<WGPUBindGroupLayout> mBindGroupLayouts;
        WGPUPipelineLayout mPipelineLayout;
        WGPURenderPipeline mPipeline = nullptr;
        WGPUVertexBufferLayout mlVertexBufferLayout;

        WGPUTextureFormat mDepthTextureFormat = WGPUTextureFormat_Depth24Plus;

        WGPUShaderModule mShaderModule;
        // state
        VertexBufferLayout mVertexBufferLayout = {};
        WGPUDepthStencilState mDepthStencilState = {};
        WGPUBlendState mBlendState = {};
        WGPUColorTargetState mColorTargetState = {};
        WGPUFragmentState mFragmentState = {};
};

#endif  // TEST_WGPU_PIPELINE_H