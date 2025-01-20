#ifndef TEST_WGPU_PIPELINE_H
#define TEST_WGPU_PIPELINE_H

#include <vector>

#include "utils.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

class Pipeline {
    public:
        Pipeline(Application* app, std::vector<WGPUBindGroupLayout> bindGroupLayout);

        Pipeline& defaultConfiguration(Application* app, WGPUTextureFormat surfaceTexture);
        Pipeline& createPipeline(Application* app);

        // Getters
        WGPUPipelineLayout getPipelineLayout();
        WGPURenderPipeline getPipeline();
        WGPURenderPipelineDescriptor getDescriptor();
        WGPURenderPipelineDescriptor* getDescriptorPtr();

        // Setter
        Pipeline& setShader(const std::filesystem::path& path);
        Pipeline& setVertexBufferLayout(WGPUVertexBufferLayout layout);
        Pipeline& setVertexState(size_t bufferCount = 1, const char* entryPoint = "vs_main",
                                 std::vector<WGPUConstantEntry> constants = {});
        Pipeline& setPrimitiveState(WGPUFrontFace frontFace = WGPUFrontFace_CCW,
                                    WGPUCullMode cullMode = WGPUCullMode_None);
        Pipeline& setDepthStencilState(bool depthWriteEnabled = false, uint32_t stencilReadMask = 0x0,
                                       uint32_t stencilWriteMask = 0x0);
        Pipeline& setBlendState();
        Pipeline& setColorTargetState();
        Pipeline& setFragmentState();
        Pipeline& setFragmentState(WGPUFragmentState fragmentState);
        Pipeline& setMultiSampleState(/*WGPUMultisampleState multiSampleState*/);

        VertexBufferLayout mVertexBufferLayout = {};

    private:
        Application* mApp;
        WGPURenderPipelineDescriptor mDescriptor = {};
        std::vector<WGPUBindGroupLayout> mBindGroupLayouts;
        WGPUPipelineLayout mPipelineLayout;
        WGPURenderPipeline mPipeline = nullptr;
        WGPUVertexBufferLayout mlVertexBufferLayout;

        WGPUTextureFormat mDepthTextureFormat = WGPUTextureFormat_Depth24Plus;

        WGPUShaderModule mShaderModule;
        // state
        WGPUDepthStencilState mDepthStencilState = {};
        WGPUBlendState mBlendState = {};
        WGPUColorTargetState mColorTargetState = {};
        WGPUFragmentState mFragmentState = {};
};

#endif  // TEST_WGPU_PIPELINE_H