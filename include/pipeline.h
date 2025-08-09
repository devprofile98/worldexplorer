#ifndef TEST_WGPU_PIPELINE_H
#define TEST_WGPU_PIPELINE_H

#include <vector>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "utils.h"

class Application;

WGPURenderPassDescriptor createRenderPassDescriptor(WGPUTextureView colorAttachment, WGPUTextureView depthTextureView);
class Pipeline {
    public:
        Pipeline(Application* app, std::vector<WGPUBindGroupLayout> bindGroupLayout, std::string name);

        Pipeline& defaultConfiguration(Application* app, WGPUTextureFormat surfaceTexture,
                                       WGPUTextureFormat depthTextureFormat = WGPUTextureFormat_Depth24Plus,
                                       const char* shaderPath = "./resources/shaders/shader.wgsl");
        Pipeline& createPipeline(Application* app);

        // Getters
        WGPUPipelineLayout getPipelineLayout();
        WGPURenderPipeline getPipeline();
        WGPURenderPipelineDescriptor getDescriptor();
        WGPURenderPipelineDescriptor* getDescriptorPtr();
        WGPUVertexBufferLayout getDefaultVertexBufferLayout();
        // Setter
        Pipeline& setShader(const std::filesystem::path& path);
        Pipeline& setVertexBufferLayout(WGPUVertexBufferLayout layout);
        Pipeline& setVertexState(size_t bufferCount = 1, const char* entryPoint = "vs_main",
                                 std::vector<WGPUConstantEntry> constants = {});
        Pipeline& setPrimitiveState(WGPUFrontFace frontFace = WGPUFrontFace_CCW,
                                    WGPUCullMode cullMode = WGPUCullMode_None);
        Pipeline& setDepthStencilState(bool depthWriteEnabled = false, uint32_t stencilReadMask = 0x0,
                                       uint32_t stencilWriteMask = 0x0,
                                       WGPUTextureFormat depthTextureFormat = WGPUTextureFormat_Depth24Plus);
        Pipeline& setDepthStencilState(WGPUDepthStencilState state);
        Pipeline& setBlendState();
        Pipeline& setBlendState(WGPUBlendState blendState);
        Pipeline& setColorTargetState(WGPUTextureFormat format = WGPUTextureFormat_Undefined);
        Pipeline& setColorTargetState(WGPUColorTargetState colorTargetState);
        Pipeline& setFragmentState();
        Pipeline& setFragmentState(WGPUFragmentState fragmentState);
        Pipeline& setMultiSampleState(/*WGPUMultisampleState multiSampleState*/);

        WGPUDepthStencilState& getDepthStencilState();

        VertexBufferLayout mVertexBufferLayout = {};

    private:
        Application* mApp;
        std::string mPipelineName{};
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
