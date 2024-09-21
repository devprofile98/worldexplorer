#include "application.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

#include "wgpu_utils.h"

constexpr const char* SHADER_SOURCE = R"(

    @vertex
    fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {

        return vec4f(in_vertex_position,0.0, 1.0);

    }

    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4(0.0, 0.4, 1.0, 1.0);
    }

)";

void Application::initializePipeline() {
    // ---------------------------- Render pipeline

    WGPURenderPipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.nextInChain = nullptr;

    // Creation of shader module
    WGPUShaderModuleDescriptor shader_descriptor = {};
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    WGPUShaderModuleWGSLDescriptor shader_code_descriptor = {};
    shader_code_descriptor.chain.next = nullptr;
    shader_code_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shader_descriptor.nextInChain = &shader_code_descriptor.chain;
    shader_code_descriptor.code = SHADER_SOURCE;

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(mDevice, &shader_descriptor);

    // --- describe the pipe line
    // describe the vertex state
    WGPUVertexBufferLayout vertex_buffer_layout = {};

    vertex_buffer_layout.attributeCount = 1;

    WGPUVertexAttribute position_attrib = {};
    position_attrib.offset = 0;
    position_attrib.shaderLocation = 0;
    position_attrib.format = WGPUVertexFormat_Float32x2;
    vertex_buffer_layout.attributes = &position_attrib;

    vertex_buffer_layout.arrayStride = 2 * sizeof(float);
    vertex_buffer_layout.stepMode = WGPUVertexStepMode_Vertex;

    pipeline_descriptor.vertex.bufferCount = 1;
    pipeline_descriptor.vertex.buffers = &vertex_buffer_layout;
    pipeline_descriptor.vertex.module = shader_module;
    pipeline_descriptor.vertex.entryPoint = "vs_main";
    pipeline_descriptor.vertex.constantCount = 0;
    pipeline_descriptor.vertex.constants = nullptr;

    // describe primitive assembly and rasterization
    pipeline_descriptor.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipeline_descriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pipeline_descriptor.primitive.frontFace = WGPUFrontFace_CCW;
    pipeline_descriptor.primitive.cullMode = WGPUCullMode_None;

    // describe and configure fragment shader
    WGPUFragmentState fragment_state = {};
    fragment_state.module = shader_module;
    fragment_state.entryPoint = "fs_main";
    fragment_state.constants = nullptr;
    fragment_state.constantCount = 0;
    pipeline_descriptor.fragment = &fragment_state;

    // depth and stencil
    pipeline_descriptor.depthStencil = nullptr;

    // blend state
    WGPUBlendState blend_state = {};
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.color.operation = WGPUBlendOperation_Add;

    blend_state.alpha.srcFactor = WGPUBlendFactor_Zero;
    blend_state.alpha.dstFactor = WGPUBlendFactor_One;
    blend_state.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState color_target = {};
    color_target.format = mSurfaceFormat;
    color_target.blend = &blend_state;
    color_target.writeMask = WGPUColorWriteMask_All;

    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.multisample.mask = ~0u;
    pipeline_descriptor.multisample.alphaToCoverageEnabled = false;

    pipeline_descriptor.layout = nullptr;

    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipeline_descriptor);

    // ---------------------------- End of Render Pipeline
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    std::vector<float> vertex_data = {// x0, y0
                                      -0.5, -0.5,

                                      // x1, y1
                                      +0.5, -0.5,

                                      // x2, y2
                                      +0.0, +0.5,

                                      // Add a second triangle:
                                      -0.55f, -0.5, -0.05f, +0.5, -0.55f, +0.5

    };
    mVertexCount = static_cast<uint32_t>(vertex_data.size() / 2);

    WGPUBufferDescriptor vertex_buffer_descriptor = {};
    vertex_buffer_descriptor.nextInChain = nullptr;
    vertex_buffer_descriptor.size = vertex_data.size() * sizeof(float);
    vertex_buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    vertex_buffer_descriptor.mappedAtCreation = false;

    mVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &vertex_buffer_descriptor);
    // Uploading data to GPU
    wgpuQueueWriteBuffer(mQueue, mVertexBuffer, 0, vertex_data.data(), vertex_buffer_descriptor.size);
}

// We define a function that hides implementation-specific variants of device polling:
void wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser) {
    wgpuDevicePoll(device, false, nullptr);
}

bool Application::initialize() {
    // We create a descriptor

    glfwInit();

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mWindow = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
    if (!mWindow) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return false;
    }

    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    // We create the instance using this descriptor
    WGPUInstance instance = wgpuCreateInstance(&desc);
    // We can check whether there is actually an instance created
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    //   UserData user_data;
    mSurface = glfwGetWGPUSurface(instance, mWindow);

    auto adapter = requestAdapterSync(instance, mSurface);
    std::cout << std::format("WGPU instance: {:p} {:p} {:p}", (void*)instance, (void*)adapter, (void*)mSurface)
              << std::endl;
    // Release wgpu instance
    // wgpuInstanceRelease(instance);

    inspectAdapter(adapter);
    inspectFeatures(adapter);
    inspectProperties(adapter);

    // requesting device from adaptor
    mDevice = requestDeviceSync(adapter, GetRequiredLimits(adapter));
    inspectDevice(mDevice);

    // relase the adapter

    mQueue = getDeviceQueue(mDevice);

    // Create windows to show our daste gol

    // Configuring the surface
    WGPUSurfaceConfiguration surface_configuration = {};
    surface_configuration.nextInChain = nullptr;
    // Configure the texture created for swap chain

    surface_configuration.width = 640;
    surface_configuration.height = 480;
    surface_configuration.usage = WGPUTextureUsage_RenderAttachment;

    WGPUSurfaceCapabilities capabilities = {};
    std::cout << std::format("Successfuly initialized GLFW and the surface is {:p}", (void*)mSurface) << std::endl;
    wgpuSurfaceGetCapabilities(mSurface, adapter, &capabilities);
    surface_configuration.format = capabilities.formats[0];
    mSurfaceFormat = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    surface_configuration.viewFormatCount = 0;
    surface_configuration.viewFormats = nullptr;
    surface_configuration.device = mDevice;
    surface_configuration.presentMode = WGPUPresentMode_Fifo;
    surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(mSurface, &surface_configuration);
    wgpuAdapterRelease(adapter);

    initializeBuffers();
    initializePipeline();

    // Playing with buffers
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    buffer_descriptor.label = "Some GPU-side data buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    buffer_descriptor.size = 16;
    buffer_descriptor.mappedAtCreation = false;
    mBuffer1 = wgpuDeviceCreateBuffer(mDevice, &buffer_descriptor);

    buffer_descriptor.label = "output buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    mBuffer2 = wgpuDeviceCreateBuffer(mDevice, &buffer_descriptor);

    // Writing to buffer
    std::vector<uint8_t> numbers{16};
    for (uint8_t i = 0; i < 16; i++) numbers[i] = i;

    wgpuQueueWriteBuffer(mQueue, mBuffer1, 0, numbers.data(), 16);

    // copy from buffer1 to buffer2
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, nullptr);

    wgpuCommandEncoderCopyBufferToBuffer(encoder, mBuffer1, 0, mBuffer2, 0, 16);

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(mQueue, 1, &command);
    wgpuCommandBufferRelease(command);

    struct Context {
            bool ready;
            WGPUBuffer buffer;
    };

    auto on_buffer2_mapped = [](WGPUBufferMapAsyncStatus status, void* userData) {
        Context* context = reinterpret_cast<Context*>(userData);
        if (status != WGPUBufferMapAsyncStatus_Success) {
            return;
        }
        std::cout << std::format("the status for Async buffer 2 mapping is: {:d}\n", (size_t)status);
        context->ready = true;
    };
    Context user_data = {};
    user_data.buffer = mBuffer2;
    user_data.ready = false;

    wgpuBufferMapAsync(mBuffer2, WGPUMapMode_Read, 0, 16, on_buffer2_mapped, &user_data);
    while (!user_data.ready) {
        wgpuPollEvents(mDevice, true);
    }

    uint8_t* buffer_mapped_data = (uint8_t*)wgpuBufferGetConstMappedRange(user_data.buffer, 0, 16);

    for (size_t i = 0; i < 16; i++) {
        std::cout << std::format("Element {} is {}\n", i, buffer_mapped_data[i]);
    }

    return true;
}

void Application::mainLoop() {
    std::cout << "Inside Main Loop !" << std::endl;
    glfwPollEvents();

    WGPUTextureView target_view = getNextSurfaceTextureView();
    if (target_view == nullptr) {
        return;
    }

    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = "my command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoder_descriptor);

    WGPURenderPassDescriptor render_pass_descriptor = {};
    render_pass_descriptor.nextInChain = nullptr;

    WGPURenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view = target_view;
    render_pass_color_attachment.resolveTarget = nullptr;
    render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
    render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
    render_pass_color_attachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

    render_pass_descriptor.colorAttachmentCount = 1;
    render_pass_descriptor.colorAttachments = &render_pass_color_attachment;
    render_pass_descriptor.depthStencilAttachment = nullptr;
    render_pass_descriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass_encoder, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDraw(render_pass_encoder, mVertexCount, 1, 0, 0);

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);

    WGPUCommandBufferDescriptor command_buffer_descriptor = {};
    command_buffer_descriptor.nextInChain = nullptr;
    command_buffer_descriptor.label = "command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &command_buffer_descriptor);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(mQueue, 1, &command);
    wgpuCommandBufferRelease(command);

    // Select which render pipeline to use

    wgpuTextureViewRelease(target_view);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(mSurface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#endif
}

void Application::terminate() {
    wgpuBufferRelease(mBuffer1);
    wgpuBufferRelease(mBuffer2);
    wgpuBufferRelease(mVertexBuffer);
    wgpuRenderPipelineRelease(mPipeline);
    wgpuSurfaceUnconfigure(mSurface);
    wgpuQueueRelease(mQueue);
    wgpuSurfaceRelease(mSurface);
    wgpuDeviceRelease(mDevice);
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

bool Application::isRunning() { return !glfwWindowShouldClose(mWindow); }

WGPUTextureView Application::getNextSurfaceTextureView() {
    WGPUSurfaceTexture surface_texture = {};
    wgpuSurfaceGetCurrentTexture(mSurface, &surface_texture);
    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return nullptr;
    }

    WGPUTextureViewDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.label = "surface texture view ";
    descriptor.format = wgpuTextureGetFormat(surface_texture.texture);
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;
    descriptor.aspect = WGPUTextureAspect_All;
    return wgpuTextureCreateView(surface_texture.texture, &descriptor);
}

void setDefault(WGPULimits& limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    // [...] Set everything to WGPU_LIMIT_U32_UNDEFINED or WGPU_LIMIT_U64_UNDEFINED to mean no limit

    limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

WGPURequiredLimits Application::GetRequiredLimits(WGPUAdapter adapter) const {
    WGPUSupportedLimits supported_limits = {};
    supported_limits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supported_limits);

    WGPURequiredLimits required_limits = {};
    setDefault(required_limits.limits);

    required_limits.limits.maxVertexAttributes = 1;
    required_limits.limits.maxVertexBuffers = 1;
    required_limits.limits.maxBufferSize = 6 * 2 * sizeof(float);
    required_limits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);

    return required_limits;
}