#include "pipeline.h"

#include <webgpu/webgpu.h>

#include "application.h"
#include "shader.h"
#include "wgpu_utils.h"

WGPURenderPassDescriptor createRenderPassDescriptor(WGPUTextureView colorAttachment, WGPUTextureView depthTextureView) {
    WGPURenderPassDescriptor render_pass_descriptor = {};
    render_pass_descriptor.nextInChain = nullptr;

    WGPURenderPassColorAttachment color_attachment = {};
    color_attachment.view = colorAttachment;
    color_attachment.resolveTarget = nullptr;
    color_attachment.loadOp = WGPULoadOp_Load;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = WGPUColor{0.52, 0.80, 0.92, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

    render_pass_descriptor.colorAttachmentCount = 1;
    render_pass_descriptor.colorAttachments = &color_attachment;

    WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
    depth_stencil_attachment.view = depthTextureView;
    depth_stencil_attachment.depthClearValue = 1.0f;
    depth_stencil_attachment.depthLoadOp = WGPULoadOp_Load;
    depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
    depth_stencil_attachment.depthReadOnly = false;
    depth_stencil_attachment.stencilClearValue = 0;
    depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
    depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
    depth_stencil_attachment.stencilReadOnly = true;
    render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
    render_pass_descriptor.timestampWrites = nullptr;
    return render_pass_descriptor;
}

Pipeline::Pipeline(Application* app, std::vector<WGPUBindGroupLayout> bindGroupLayout, std::string name)
    : mApp(app), mPipelineName(name), mDescriptor({}), mBindGroupLayouts(bindGroupLayout) {}

Pipeline& Pipeline::createPipeline(Application* app) {
    WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.label = createStringView(mPipelineName);
    pipeline_layout_descriptor.nextInChain = nullptr;
    pipeline_layout_descriptor.bindGroupLayoutCount = mBindGroupLayouts.size();
    pipeline_layout_descriptor.bindGroupLayouts = mBindGroupLayouts.data();
    std::cout << "****************************" << mPipelineName << mBindGroupLayouts.size() << std::endl;

    mPipelineLayout = wgpuDeviceCreatePipelineLayout(app->getRendererResource().device, &pipeline_layout_descriptor);

    mDescriptor.layout = mPipelineLayout;
    mDescriptor.label = createStringView(mPipelineName);
    mPipeline = wgpuDeviceCreateRenderPipeline(app->getRendererResource().device, &mDescriptor);
    return *this;
}

WGPUVertexBufferLayout Pipeline::getDefaultVertexBufferLayout() {
    return mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
        .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
        .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
        .addAttribute(12 * sizeof(float), 3, WGPUVertexFormat_Float32x3)
        .addAttribute(16 * sizeof(float), 4, WGPUVertexFormat_Float32x3)
        .addAttribute(offsetof(VertexAttributes, uv), 5, WGPUVertexFormat_Float32x2)
        .addAttribute(offsetof(VertexAttributes, boneIds), 6, WGPUVertexFormat_Sint32x4)
        .addAttribute(offsetof(VertexAttributes, weights), 7, WGPUVertexFormat_Float32x4)
        .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);
}

Pipeline& Pipeline::defaultConfiguration(Application* app, WGPUTextureFormat surfaceTexture,
                                         WGPUTextureFormat depthTexture, const char* shaderPath

) {
    // 0 - Load Default shader

    std::cout << "Defalt confiiguraton for " << shaderPath << std::endl;

    mShaderModule = loadShader(shaderPath, app->getRendererResource().device);
    // 1 - vertex state
    mlVertexBufferLayout = getDefaultVertexBufferLayout();
    mDescriptor.nextInChain = nullptr;
    mDescriptor.label = createStringViewC("default pipeline layout");

    WGPUVertexState vertex_state = {};
    vertex_state.bufferCount = 1;
    vertex_state.buffers = &mlVertexBufferLayout;
    vertex_state.module = mShaderModule;
    vertex_state.entryPoint = createStringViewC("vs_main");
    vertex_state.constantCount = 0;
    vertex_state.constants = nullptr;
    (void)vertex_state;
    mDescriptor.vertex = vertex_state;

    // 3 - Primitive state
    WGPUPrimitiveState primitive_state = {};
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    /*primitive_state.topology = WGPUPrimitiveTopology_LineStrip;*/
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitive_state.frontFace = WGPUFrontFace_CCW;
    primitive_state.cullMode = WGPUCullMode_None;
    mDescriptor.primitive = primitive_state;

    // 4 - depth stencil state
    mDepthStencilState = {};
    setDefault(mDepthStencilState);

    mDepthStencilState.depthCompare = WGPUCompareFunction_Less;
    mDepthStencilState.depthWriteEnabled = WGPUOptionalBool_True;

    // WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    mDepthStencilState.format = depthTexture;
    mDepthStencilState.stencilReadMask = 0;
    mDepthStencilState.stencilWriteMask = 0xff;
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
    mFragmentState.entryPoint = createStringViewC("fs_main");
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
WGPURenderPipelineDescriptor* Pipeline::getDescriptorPtr() { return &mDescriptor; }

Pipeline& Pipeline::setShader(const std::filesystem::path& path) {
    mShaderModule = loadShader(path, mApp->getRendererResource().device);
    mDescriptor.vertex.module = mShaderModule;
    mFragmentState.module = mShaderModule;
    return *this;
}

Pipeline& Pipeline::setVertexBufferLayout(WGPUVertexBufferLayout layout) {
    mlVertexBufferLayout = layout;
    return *this;
}

Pipeline& Pipeline::setVertexState(size_t bufferCount, const char* entryPoint,
                                   std::vector<WGPUConstantEntry> constants) {
    WGPUVertexState vertex_state = {};
    vertex_state.bufferCount = bufferCount;
    vertex_state.buffers = &mlVertexBufferLayout;
    vertex_state.module = mShaderModule;
    vertex_state.entryPoint = createStringViewC(entryPoint);
    vertex_state.constantCount = constants.size();
    vertex_state.constants = constants.data();
    mDescriptor.vertex = vertex_state;
    return *this;
}

Pipeline& Pipeline::setPrimitiveState(WGPUFrontFace frontFace, WGPUCullMode cullMode) {
    WGPUPrimitiveState primitive_state = {};
    primitive_state.topology = WGPUPrimitiveTopology_TriangleList;
    primitive_state.stripIndexFormat = WGPUIndexFormat_Undefined;
    primitive_state.frontFace = frontFace;
    primitive_state.cullMode = cullMode;
    mDescriptor.primitive = primitive_state;
    return *this;
}

Pipeline& Pipeline::setDepthStencilState(bool depthWriteEnabled, uint32_t stencilReadMask, uint32_t stencilWriteMask,
                                         WGPUTextureFormat depthTextureFormat) {
    mDepthStencilState = {};
    setDefault(mDepthStencilState);

    mDepthStencilState.depthCompare = WGPUCompareFunction_Less;
    mDepthStencilState.depthWriteEnabled = depthWriteEnabled ? WGPUOptionalBool_True : WGPUOptionalBool_False;

    mDepthStencilState.format = depthTextureFormat;
    // mDepthStencilState.stencilFront = {};
    // mDepthStencilState.stencilBack = {};
    mDepthStencilState.stencilReadMask = stencilReadMask;
    mDepthStencilState.stencilWriteMask = stencilWriteMask;
    // mDepthStencilState.depthBias = 2.0;
    mDepthStencilState.stencilFront.compare = WGPUCompareFunction_Always;
    mDepthStencilState.stencilBack.compare = WGPUCompareFunction_Always;
    mDepthStencilState.stencilBack.passOp = WGPUStencilOperation_Keep;
    mDepthStencilState.stencilFront.passOp = WGPUStencilOperation_Keep;
    mDescriptor.depthStencil = &mDepthStencilState;
    return *this;
}

Pipeline& Pipeline::setDepthStencilState(WGPUDepthStencilState state) {
    mDepthStencilState = state;
    mDescriptor.depthStencil = &mDepthStencilState;
    return *this;
}

Pipeline& Pipeline::setBlendState() {
    mBlendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    mBlendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    mBlendState.color.operation = WGPUBlendOperation_Add;

    mBlendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    mBlendState.alpha.dstFactor = WGPUBlendFactor_One;
    mBlendState.alpha.operation = WGPUBlendOperation_Add;
    return *this;
}

Pipeline& Pipeline::setBlendState(WGPUBlendState blendState) {
    mBlendState = blendState;
    return *this;
}

Pipeline& Pipeline::setColorTargetState(WGPUTextureFormat format) {
    mColorTargetState.format = format == WGPUTextureFormat_Undefined ? mApp->getTextureFormat() : format;
    mColorTargetState.blend = &mBlendState;
    mColorTargetState.writeMask = WGPUColorWriteMask_All;
    return *this;
}

Pipeline& Pipeline::setColorTargetState(WGPUColorTargetState colorTargetState) {
    /*mColorTargetState = colorTargetState;*/
    (void)colorTargetState;
    mFragmentState.targetCount = 0;
    mFragmentState.targets = nullptr;
    return *this;
}

Pipeline& Pipeline::setFragmentState() {
    mFragmentState.module = mShaderModule;
    mFragmentState.entryPoint = createStringViewC("fs_main");
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

Pipeline& Pipeline::setMultiSampleState(/*WGPUMultisampleState multiSampleState*/) {
    mDescriptor.multisample.count = 1;
    mDescriptor.multisample.mask = ~0u;
    mDescriptor.multisample.alphaToCoverageEnabled = false;
    return *this;
}

Pipeline& Pipeline::setFragmentState(WGPUFragmentState fragmentState) {
    // mFragmentState.module = mShaderModule;
    // mFragmentState.entryPoint = "fs_main";
    // mFragmentState.constants = nullptr;
    // mFragmentState.constantCount = 0;
    // mFragmentState.targetCount = 1;
    // mFragmentState.targets = &mColorTargetState;
    mDescriptor.fragment = &fragmentState;

    mDescriptor.multisample.count = 1;
    mDescriptor.multisample.mask = ~0u;
    mDescriptor.multisample.alphaToCoverageEnabled = false;

    return *this;
}

WGPUDepthStencilState& Pipeline::getDepthStencilState() { return mDepthStencilState; }
