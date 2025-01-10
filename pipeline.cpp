#include "pipeline.h"

#include "application.h"

Pipeline::Pipeline(std::vector<WGPUBindGroupLayout> bindGroupLayout)
    : mDescriptor({}), mBindGroupLayouts(bindGroupLayout) {}

Pipeline& Pipeline::createPipeline(Application* app) {
    WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.nextInChain = nullptr;
    pipeline_layout_descriptor.bindGroupLayoutCount = mBindGroupLayouts.size();
    pipeline_layout_descriptor.bindGroupLayouts = mBindGroupLayouts.data();

    mPipelineLayout = wgpuDeviceCreatePipelineLayout(app->getRendererResource().device, &pipeline_layout_descriptor);

    mDescriptor.layout = mPipelineLayout;

    mPipeline = wgpuDeviceCreateRenderPipeline(app->getRendererResource().device, &mDescriptor);
    return *this;
}

Pipeline& Pipeline::defaultConfiguration(Application* app, WGPUTextureFormat surfaceTexture) {
    // 0 - Load Default shader

    mShaderModule = loadShader(RESOURCE_DIR "/shader.wgsl", app->getRendererResource().device);
    // 1 - vertex state
    mlVertexBufferLayout = mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
                               .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
                               .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
                               .addAttribute(offsetof(VertexAttributes, uv), 3, WGPUVertexFormat_Float32x2)
                               .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);

    mDescriptor.nextInChain = nullptr;

    WGPUVertexState vertex_state = {};
    vertex_state.bufferCount = 1;
    vertex_state.buffers = &mlVertexBufferLayout;
    vertex_state.module = mShaderModule;
    vertex_state.entryPoint = "vs_main";
    vertex_state.constantCount = 0;
    vertex_state.constants = nullptr;
    (void)vertex_state;
    mDescriptor.vertex = vertex_state;

    // 3 - Primitive state
    WGPUPrimitiveState primitive_state = {};
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitive_state.frontFace = WGPUFrontFace_CCW;
    primitive_state.cullMode = WGPUCullMode_Back;
    mDescriptor.primitive = primitive_state;

    // 4 - depth stencil state
    mDepthStencilState = {};
    setDefault(mDepthStencilState);

    mDepthStencilState.depthCompare = WGPUCompareFunction_Less;
    mDepthStencilState.depthWriteEnabled = true;

    // WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    mDepthStencilState.format = mDepthTextureFormat;
    mDepthStencilState.stencilReadMask = 0;
    mDepthStencilState.stencilWriteMask = 0;
    mDescriptor.depthStencil = &mDepthStencilState;

    // 6 - blend state
    // WGPUBlendState blend_state = {};
    mBlendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    mBlendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    mBlendState.color.operation = WGPUBlendOperation_Add;

    mBlendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    mBlendState.alpha.dstFactor = WGPUBlendFactor_One;
    mBlendState.alpha.operation = WGPUBlendOperation_Add;

    // 5 - color target
    // WGPUColorTargetState color_target = {};
    mColorTargetState.format = surfaceTexture;
    mColorTargetState.blend = &mBlendState;
    mColorTargetState.writeMask = WGPUColorWriteMask_All;

    // 2 - Fragment state
    // WGPUFragmentState fragment_state = {};
    mFragmentState.module = mShaderModule;
    mFragmentState.entryPoint = "fs_main";
    mFragmentState.constants = nullptr;
    mFragmentState.constantCount = 0;
    mFragmentState.targetCount = 1;
    mFragmentState.targets = &mColorTargetState;
    mDescriptor.fragment = &mFragmentState;

    mDescriptor.multisample.count = 1;
    mDescriptor.multisample.mask = ~0u;
    mDescriptor.multisample.alphaToCoverageEnabled = false;

    return *this;
}

WGPURenderPipeline Pipeline::getPipeline() { return mPipeline; }

WGPUPipelineLayout Pipeline::getPipelineLayout() { return mPipelineLayout; }

WGPURenderPipelineDescriptor Pipeline::getDescriptor() { return mDescriptor; }