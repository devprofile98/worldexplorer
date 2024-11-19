#include "application.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_wgpu.h"
#include "imgui/imgui.h"
#include "texture.h"
#include "utils.h"
#include "wgpu_utils.h"

// #define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

void MyUniform::setCamera(const Camera& camera) {
    projectMatrix = camera.getProjection();
    viewMatrix = camera.getView();
    modelMatrix = camera.getModel();
}

void setDefault(WGPULimits& limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
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

void setDefault(WGPUBindGroupLayoutEntry& bindingLayout) {
    bindingLayout = {};
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
    bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

void setDefault(WGPUStencilFaceState& stencilFaceState) {
    stencilFaceState.compare = WGPUCompareFunction_Always;
    stencilFaceState.failOp = WGPUStencilOperation_Keep;
    stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
    stencilFaceState.passOp = WGPUStencilOperation_Keep;
}

void setDefault(WGPUDepthStencilState& depthStencilState) {
    depthStencilState.format = WGPUTextureFormat_Undefined;
    depthStencilState.depthWriteEnabled = false;
    depthStencilState.depthCompare = WGPUCompareFunction_Always;
    depthStencilState.stencilReadMask = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask = 0xFFFFFFFF;
    depthStencilState.depthBias = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp = 0;
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
}

void Application::initializePipeline() {
    // ---------------------------- Render pipeline

    WGPURenderPipelineDescriptor pipeline_descriptor = {};
    pipeline_descriptor.nextInChain = nullptr;

    // Creation of shader module
    // WGPUShaderModuleDescriptor shader_descriptor = {};
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    WGPUShaderModule shader_module = loadShader(RESOURCE_DIR "/shader.wgsl", mRendererResource.device);

    // --- describe the pipe line
    // describe the vertex state
    WGPUVertexBufferLayout vertex_buffer_layout = {};

    vertex_buffer_layout.attributeCount = 4;

    std::vector<WGPUVertexAttribute> attribs{4};

    // attribute -> position
    attribs[0].offset = 0;
    attribs[0].shaderLocation = 0;
    attribs[0].format = WGPUVertexFormat_Float32x3;

    // attribute -> normal
    attribs[1].offset = 3 * sizeof(float);
    attribs[1].shaderLocation = 1;
    attribs[1].format = WGPUVertexFormat_Float32x3;

    // attribute -> color
    attribs[2].offset = 6 * sizeof(float);
    attribs[2].shaderLocation = 2;
    attribs[2].format = WGPUVertexFormat_Float32x3;

    // attribute->uv
    attribs[3].offset = offsetof(VertexAttributes, uv);
    attribs[3].shaderLocation = 3;
    attribs[3].format = WGPUVertexFormat_Float32x2;

    vertex_buffer_layout.attributes = attribs.data();
    vertex_buffer_layout.arrayStride = sizeof(VertexAttributes);
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
    WGPUDepthStencilState depth_stencil_state = {};
    setDefault(depth_stencil_state);

    depth_stencil_state.depthCompare = WGPUCompareFunction_Less;
    depth_stencil_state.depthWriteEnabled = true;

    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    depth_stencil_state.format = depth_texture_format;
    depth_stencil_state.stencilReadMask = 0;
    depth_stencil_state.stencilWriteMask = 0;

    pipeline_descriptor.depthStencil = &depth_stencil_state;

    initDepthBuffer();

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

    Texture simple_texture{mRendererResource.device, RESOURCE_DIR "/texture.jpg"};
    WGPUTextureView textureView = simple_texture.createView();
    simple_texture.uploadToGPU(mRendererResource.queue);
    // WGPUTextureView textureView = {};

    if (!textureView) {
        std::cout << "Failed to Create Texture view!!!\n";
    }

    WGPUBindGroupLayoutEntry binding_layout_entries = {};
    setDefault(binding_layout_entries);
    binding_layout_entries.binding = 0;
    binding_layout_entries.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    binding_layout_entries.buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout_entries.buffer.minBindingSize = sizeof(MyUniform);
    mBindingGroup.add(binding_layout_entries);

    // Binding Number 1
    WGPUBindGroupLayoutEntry binding_layout_entries2 = {};
    setDefault(binding_layout_entries2);
    binding_layout_entries2.binding = 1;
    binding_layout_entries2.visibility = WGPUShaderStage_Fragment;
    binding_layout_entries2.texture.sampleType = WGPUTextureSampleType_Float;
    binding_layout_entries2.texture.viewDimension = WGPUTextureViewDimension_2D;
    mBindingGroup.add(binding_layout_entries2);

    WGPUBindGroupLayoutEntry binding_layout_entries3 = {};
    setDefault(binding_layout_entries3);
    binding_layout_entries3.binding = 2;
    binding_layout_entries3.visibility = WGPUShaderStage_Fragment;
    binding_layout_entries3.sampler.type = WGPUSamplerBindingType_Filtering;
    mBindingGroup.add(binding_layout_entries3);

    WGPUBindGroupLayoutEntry lighting_information_entry = {};
    setDefault(lighting_information_entry);
    lighting_information_entry.binding = 3;
    lighting_information_entry.visibility = WGPUShaderStage_Fragment;
    lighting_information_entry.buffer.type = WGPUBufferBindingType_Uniform;
    binding_layout_entries.buffer.minBindingSize = sizeof(LightingUniforms);
    mBindingGroup.add(lighting_information_entry);

    // WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor = {};
    WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor = {};
    bind_group_layout_descriptor.nextInChain = nullptr;
    bind_group_layout_descriptor.label = "binding group layout";
    bind_group_layout_descriptor.entryCount = mBindingGroup.getEntryCount();
    bind_group_layout_descriptor.entries = mBindingGroup.getEntryData();

    WGPUBindGroupLayout bind_group_layout =
        wgpuDeviceCreateBindGroupLayout(mRendererResource.device, &bind_group_layout_descriptor);

    WGPUBindGroupLayoutEntry object_transformation = {};
    setDefault(object_transformation);
    object_transformation.binding = 0;
    object_transformation.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    object_transformation.buffer.type = WGPUBufferBindingType_Uniform;
    object_transformation.buffer.minBindingSize = sizeof(glm::mat4);
    // mBindingGroup.add(binding_layout_entries);
    WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor1 = {};
    bind_group_layout_descriptor1.nextInChain = nullptr;
    bind_group_layout_descriptor1.label = "Object Tranformation Matrix uniform";
    bind_group_layout_descriptor1.entryCount = 1;
    bind_group_layout_descriptor1.entries = &object_transformation;

    mBindGroupLayouts[0] = bind_group_layout;
    mBindGroupLayouts[1] = wgpuDeviceCreateBindGroupLayout(mRendererResource.device, &bind_group_layout_descriptor1);

    WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.nextInChain = nullptr;
    pipeline_layout_descriptor.bindGroupLayoutCount = mBindGroupLayouts.size();
    pipeline_layout_descriptor.bindGroupLayouts = mBindGroupLayouts.data();
    WGPUPipelineLayout pipeline_layout =
        wgpuDeviceCreatePipelineLayout(mRendererResource.device, &pipeline_layout_descriptor);
    pipeline_descriptor.layout = pipeline_layout;

    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mUniformBuffer;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(MyUniform);

    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = textureView;

    WGPUSamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    WGPUSampler sampler = wgpuDeviceCreateSampler(mRendererResource.device, &samplerDesc);

    mBindingData[2].binding = 2;
    mBindingData[2].sampler = sampler;

    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].buffer = mDirectionalLightBuffer;
    mBindingData[3].offset = 0;
    mBindingData[3].size = sizeof(LightingUniforms);

    mBindGroupDescriptor.nextInChain = nullptr;
    mBindGroupDescriptor.layout = bind_group_layout;
    mBindGroupDescriptor.entryCount = bind_group_layout_descriptor.entryCount;
    mBindGroupDescriptor.entries = mBindingData.data();

    mBindingGroup.create(this, mBindGroupDescriptor);

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    buffer_descriptor.size = sizeof(glm::mat4);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBufferTransform = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    ///////// Transformation bind group
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = mUniformBufferTransform;
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(glm::mat4);

    // WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = mBindGroupLayouts[1];

    bindGrouptrans = wgpuDeviceCreateBindGroup(mRendererResource.device, &mTrasBindGroupDesc);

    mPipeline = wgpuDeviceCreateRenderPipeline(mRendererResource.device, &pipeline_descriptor);

    // ---------------------------- End of Render Pipeline
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    boat_model
        .load(mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/fourareen.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);
    tower_model.load(mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/tower.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>....\n";

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    buffer_descriptor.size = sizeof(MyUniform);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBuffer = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);
    mUniforms.time = 1.0f;
    mUniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
    // setupCamera(mUniforms);
    mCamera = Camera{{-0.0f, -0.0f, 0.0f}, glm::vec3{0.8f}, {1.0, 0.0, 0.0}, 0.0};
    mUniforms.setCamera(mCamera);
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, 0, &mUniforms, buffer_descriptor.size);

    WGPUBufferDescriptor lighting_buffer_descriptor = {};
    lighting_buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    lighting_buffer_descriptor.label = ":::::";
    lighting_buffer_descriptor.size = sizeof(LightingUniforms);
    lighting_buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    lighting_buffer_descriptor.mappedAtCreation = false;
    mDirectionalLightBuffer = wgpuDeviceCreateBuffer(mRendererResource.device, &lighting_buffer_descriptor);

    mLightingUniforms.directions = {glm::vec4{0.5, -0.9, 0.1, 1.0}, glm::vec4{0.2, 0.4, 0.3, 1.0}};
    mLightingUniforms.colors = {glm::vec4{1.0, 0.9, 0.6, 1.0}, glm::vec4{0.6, 0.9, 1.0, 1.0}};
    // std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>....\n";
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                         lighting_buffer_descriptor.size);

    // updateViewMatrix();
}

// We define a function that hides implementation-specific variants of device polling:
void wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser) {
    wgpuDevicePoll(device, false, nullptr);
}

void Application::onResize() {
    // Terminate in reverse order
    terminateDepthBuffer();
    terminateSwapChain();

    // Re-init

    initSwapChain();
    initDepthBuffer();
    updateProjectionMatrix();
}

void onWindowResize(GLFWwindow* window, int /* width */, int /* height */) {
    // We know that even though from GLFW's point of view this is
    // "just a pointer", in our case it is always a pointer to an
    // instance of the class `Application`
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    // Call the actual class-member callback
    if (that != nullptr) that->onResize();
}

bool Application::initialize() {
    // We create a descriptor

    glfwInit();

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* provided_window = glfwCreateWindow(1920, 1080, "Learn WebGPU", nullptr, nullptr);
    if (!provided_window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set up Callbacks
    glfwSetWindowUserPointer(provided_window, this);  // set user pointer to be used in the callback function
    glfwSetFramebufferSizeCallback(provided_window, onWindowResize);
    glfwSetCursorPosCallback(provided_window, [](GLFWwindow* window, double xpos, double ypos) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onMouseMove(xpos, ypos);
    });
    glfwSetMouseButtonCallback(provided_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onMouseButton(button, action, mods);
    });
    glfwSetScrollCallback(provided_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onScroll(xoffset, yoffset);
    });

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
    WGPUSurface provided_surface;
    provided_surface = glfwGetWGPUSurface(instance, provided_window);

    auto adapter = requestAdapterSync(instance, provided_surface);
    std::cout << std::format("WGPU instance: {:p} {:p} {:p}", (void*)instance, (void*)adapter, (void*)provided_surface)
              << std::endl;
    // Release wgpu instance
    // wgpuInstanceRelease(instance);

    inspectAdapter(adapter);
    inspectFeatures(adapter);
    inspectProperties(adapter);

    // requesting device from adaptor
    WGPUDevice render_device;
    render_device = requestDeviceSync(adapter, GetRequiredLimits(adapter));
    inspectDevice(render_device);

    // relase the adapter
    WGPUQueue render_queue = getDeviceQueue(render_device);

    // initialize RenderResources
    mRendererResource.device = render_device;
    mRendererResource.queue = render_queue;
    mRendererResource.surface = provided_surface;
    mRendererResource.window = provided_window;

    // Create windows to show our daste gol

    // Configuring the surface
    WGPUSurfaceConfiguration surface_configuration = {};
    surface_configuration.nextInChain = nullptr;
    // Configure the texture created for swap chain

    surface_configuration.width = 1920;
    surface_configuration.height = 1080;
    surface_configuration.usage = WGPUTextureUsage_RenderAttachment;

    WGPUSurfaceCapabilities capabilities = {};
    std::cout << std::format("Successfuly initialized GLFW and the surface is {:p}", (void*)mRendererResource.surface)
              << std::endl;
    wgpuSurfaceGetCapabilities(mRendererResource.surface, adapter, &capabilities);
    surface_configuration.format = capabilities.formats[0];
    std::cout << "Surface texture format is : " << surface_configuration.format << std::endl;
    mSurfaceFormat = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    surface_configuration.viewFormatCount = 0;
    surface_configuration.viewFormats = nullptr;
    surface_configuration.device = mRendererResource.device;
    surface_configuration.presentMode = WGPUPresentMode_Fifo;
    surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(mRendererResource.surface, &surface_configuration);
    wgpuAdapterRelease(adapter);

    // ImGui stuff
    initGui();
    std::cout << " failed to run" << std::endl;

    initializeBuffers();
    initializePipeline();

    // Playing with buffers
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    buffer_descriptor.label = "Some GPU-side data buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    buffer_descriptor.size = 16;
    buffer_descriptor.mappedAtCreation = false;
    mBuffer1 = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    buffer_descriptor.label = "output buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    mBuffer2 = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    // Writing to buffer
    std::vector<uint8_t> numbers{16};
    for (uint8_t i = 0; i < 16; i++) numbers[i] = i;

    wgpuQueueWriteBuffer(mRendererResource.queue, mBuffer1, 0, numbers.data(), 16);

    // copy from buffer1 to buffer2
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, nullptr);

    wgpuCommandEncoderCopyBufferToBuffer(encoder, mBuffer1, 0, mBuffer2, 0, 16);

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(mRendererResource.queue, 1, &command);
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
        wgpuPollEvents(mRendererResource.device, true);
    }

    uint8_t* buffer_mapped_data = (uint8_t*)wgpuBufferGetConstMappedRange(user_data.buffer, 0, 16);

    for (size_t i = 0; i < 16; i++) {
        std::cout << std::format("Element {} is {}\n", i, buffer_mapped_data[i]);
    }

    boat_model.moveTo({1.0, -4.0, 0.0});
    tower_model.moveTo({-2.0, 0.0, 0.0});

    return true;
}

void Application::updateViewMatrix() {
    auto& camera_state = mCamera.getSate();
    float cx = cos(camera_state.angles.x);
    float sx = sin(camera_state.angles.x);
    float cy = cos(camera_state.angles.y);
    float sy = sin(camera_state.angles.y);
    glm::vec3 position = glm::vec3(cx * cy, sx * cy, sy) * std::exp(-camera_state.zoom);
    mCamera.setViewMatrix(glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0, 0, 1)));
    mUniforms.setCamera(mCamera);
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, viewMatrix),
                         &mUniforms.viewMatrix, sizeof(MyUniform::viewMatrix));
}

void Application::mainLoop() {
    glfwPollEvents();

    WGPUTextureView target_view = getNextSurfaceTextureView();
    if (target_view == nullptr) {
        return;
    }

    mUniforms.time = static_cast<float>(glfwGetTime());
    // setupCamera(mUniforms);
    float viewZ = glm::mix(0.0f, 0.25f, cos(2 * Camera::PI * mUniforms.time / 4) * 0.5 + 0.5);
    glm::vec3 focal_point{0.0, viewZ, -2.5};
    glm::mat4 T2 = glm::translate(glm::mat4{1.0f}, -focal_point);

    // Rotate the view point
    float angle2 = 3.0 * Camera::PI / 4.0;
    glm::mat4 R2 = glm::rotate(glm::mat4{1.0f}, -angle2, glm::vec3{1.0, 0.0, 0.0});
    mCamera.setViewMatrix(T2 * R2);
    // mCamera.setViewMatrix(glm::lookAt(glm::vec3(-0.5f + mUniforms.time, -1.5f + mUniforms.time, viewZ + 0.25f),
    //                                   glm::vec3(0.0f), glm::vec3(0, 0, 1)));
    // mUniforms.setCamera(mCamera);
    // wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, 0, &mUniforms, sizeof(MyUniform));

    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = "my command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, &encoder_descriptor);
    updateDragInertia();

    WGPURenderPassDescriptor render_pass_descriptor = {};
    render_pass_descriptor.nextInChain = nullptr;

    WGPURenderPassColorAttachment render_pass_color_attachment = {};
    render_pass_color_attachment.view = target_view;
    render_pass_color_attachment.resolveTarget = nullptr;
    render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
    render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
    render_pass_color_attachment.clearValue = WGPUColor{0.05, 0.05, 0.05, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
    render_pass_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

    render_pass_descriptor.colorAttachmentCount = 1;
    render_pass_descriptor.colorAttachments = &render_pass_color_attachment;

    WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
    depth_stencil_attachment.view = mDepthTextureView;
    depth_stencil_attachment.depthClearValue = 1.0f;
    depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
    depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
    depth_stencil_attachment.depthReadOnly = false;

    depth_stencil_attachment.stencilClearValue = 0;
    depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
    depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
    depth_stencil_attachment.stencilReadOnly = true;

    render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
    render_pass_descriptor.timestampWrites = nullptr;

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline);

    // wgpuRenderPassEncoderDrawIndexed(render_pass_encoder, mIndexCount, 1, 0, 0, 0);

    // mUniforms.modelMatrix = tower_model.getModelMatrix();
    // wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, modelMatrix),
    //                      &mUniforms.modelMatrix, sizeof(MyUniform::modelMatrix));

    // wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBufferTransform, 0, glm::value_ptr(mtransmodel),
    //                      sizeof(glm::mat4));

    tower_model.draw(this, render_pass_encoder, mBindingData);
    boat_model.draw(this, render_pass_encoder, mBindingData);

    updateGui(render_pass_encoder);

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);

    WGPUCommandBufferDescriptor command_buffer_descriptor = {};
    command_buffer_descriptor.nextInChain = nullptr;
    command_buffer_descriptor.label = "command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &command_buffer_descriptor);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(mRendererResource.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    // Select which render pipeline to use

    wgpuTextureViewRelease(target_view);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(mRendererResource.surface);
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
    wgpuBufferRelease(mUniformBuffer);
    terminateGui();

    // wgpuBufferRelease(mIndexBuffer);
    // wgpuBufferRelease(mVertexBuffer);
    wgpuRenderPipelineRelease(mPipeline);
    wgpuSurfaceUnconfigure(mRendererResource.surface);
    wgpuQueueRelease(mRendererResource.queue);
    wgpuSurfaceRelease(mRendererResource.surface);
    wgpuDeviceRelease(mRendererResource.device);
    glfwDestroyWindow(mRendererResource.window);
    glfwTerminate();
}

bool Application::isRunning() { return !glfwWindowShouldClose(mRendererResource.window); }

WGPUTextureView Application::getNextSurfaceTextureView() {
    WGPUSurfaceTexture surface_texture = {};
    wgpuSurfaceGetCurrentTexture(mRendererResource.surface, &surface_texture);
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

WGPURequiredLimits Application::GetRequiredLimits(WGPUAdapter adapter) const {
    WGPUSupportedLimits supported_limits = {};
    supported_limits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supported_limits);

    WGPURequiredLimits required_limits = {};
    setDefault(required_limits.limits);

    required_limits.limits.maxVertexAttributes = 4;
    required_limits.limits.maxVertexBuffers = 1;
    required_limits.limits.maxBufferSize = 1000000 * sizeof(VertexAttributes);
    required_limits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    required_limits.limits.maxSampledTexturesPerShaderStage = 1;
    required_limits.limits.maxInterStageShaderComponents = 8;

    // Binding groups
    required_limits.limits.maxBindGroups = 3;
    required_limits.limits.maxUniformBuffersPerShaderStage = 3;
    required_limits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

    required_limits.limits.maxTextureDimension1D = 2048;
    required_limits.limits.maxTextureDimension2D = 2048;
    required_limits.limits.maxTextureArrayLayers = 1;
    required_limits.limits.maxSamplersPerShaderStage = 1;

    return required_limits;
}

bool Application::initSwapChain() {
    // Get the current size of the window's framebuffer:
    int width, height;
    glfwGetFramebufferSize(mRendererResource.window, &width, &height);

    WGPUSurfaceConfiguration surface_configuration = {};
    surface_configuration.nextInChain = nullptr;
    // Configure the texture created for swap chain

    surface_configuration.width = width;
    surface_configuration.height = height;
    surface_configuration.usage = WGPUTextureUsage_RenderAttachment;
    surface_configuration.format = WGPUTextureFormat_BGRA8UnormSrgb;
    surface_configuration.viewFormatCount = 0;
    surface_configuration.viewFormats = nullptr;
    surface_configuration.device = mRendererResource.device;
    surface_configuration.presentMode = WGPUPresentMode_Fifo;
    surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;

    std::cout << "LLLLLLLLLLLLLLLLLLLLL\n";
    // glfwGetWGPUSurface(mRendererResource.insta)
    wgpuSurfaceConfigure(mRendererResource.surface, &surface_configuration);
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>2222\n";
    return mRendererResource.surface != nullptr;
}

void Application::terminateSwapChain() {
    // wgpuSurfaceRelease(mRendererResource.surface);
}

bool Application::initDepthBuffer() {
    // Get the current size of the window's framebuffer:
    int width, height;
    glfwGetFramebufferSize(mRendererResource.window, &width, &height);

    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24Plus;
    WGPUTextureDescriptor depth_texture_descriptor = {};
    depth_texture_descriptor.nextInChain = nullptr;
    depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    depth_texture_descriptor.format = depth_texture_format;
    depth_texture_descriptor.mipLevelCount = 1;
    depth_texture_descriptor.sampleCount = 1;
    depth_texture_descriptor.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment;
    depth_texture_descriptor.viewFormatCount = 1;
    depth_texture_descriptor.viewFormats = &depth_texture_format;
    mDepthTexture = wgpuDeviceCreateTexture(mRendererResource.device, &depth_texture_descriptor);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depth_texture_view_desc = {};
    depth_texture_view_desc.aspect = WGPUTextureAspect_DepthOnly;
    depth_texture_view_desc.baseArrayLayer = 0;
    depth_texture_view_desc.arrayLayerCount = 1;
    depth_texture_view_desc.baseMipLevel = 0;
    depth_texture_view_desc.mipLevelCount = 1;
    depth_texture_view_desc.dimension = WGPUTextureViewDimension_2D;
    depth_texture_view_desc.format = depth_texture_format;
    mDepthTextureView = wgpuTextureCreateView(mDepthTexture, &depth_texture_view_desc);
    return mDepthTextureView != nullptr;
}

void Application::terminateDepthBuffer() {
    wgpuTextureViewRelease(mDepthTextureView);
    wgpuTextureDestroy(mDepthTexture);
    wgpuTextureRelease(mDepthTexture);
}

void Application::updateProjectionMatrix() {
    int width, height;
    glfwGetFramebufferSize(mRendererResource.window, &width, &height);
    float ratio = width / (float)height;
    mUniforms.projectMatrix = glm::perspective(60 * Camera::PI / 180, ratio, 0.01f, 100.0f);

    // float ratio = width / height;
    // float focal_length = 2.0;
    // float near = 0.01f;
    // float far = 1000.0f;
    // float divider = 1.0f / (focal_length * (far - near));
    // mUniforms.projectMatrix = glm::transpose(glm::mat4{
    //     1.0, 0.0, 0.0, 0.0,                              //
    //     0.0, ratio, 0.0, 0.0,                            //
    //     0.0, 0.0, far * divider, -far * near * divider,  //
    //     0.0, 0.0, 1.0 / focal_length, 0.0,               //
    // });

    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, projectMatrix),
                         &mUniforms.projectMatrix, sizeof(MyUniform::projectMatrix));
}

void Application::onMouseMove(double xpos, double ypos) {
    CameraState& camera_state = mCamera.getSate();
    DragState& drag_state = mCamera.getDrag();
    if (drag_state.active) {
        glm::vec2 currentMouse = glm::vec2(-(float)xpos, (float)ypos);
        glm::vec2 delta = (currentMouse - drag_state.startMouse) * drag_state.sensitivity;
        camera_state.angles = drag_state.startCameraState.angles + delta;
        // Clamp to avoid going too far when orbitting up/down
        camera_state.angles.y = glm::clamp(camera_state.angles.y, -Camera::PI / 2 + 1e-5f, Camera::PI / 2 - 1e-5f);
        updateViewMatrix();
        // Inertia
        drag_state.velocity = delta - drag_state.previousDelta;
        drag_state.previousDelta = delta;
    }
}

void Application::onMouseButton(int button, int action, int /* modifiers */) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        // Don't rotate the camera if the mouse is already captured by an ImGui
        // interaction at this frame.
        return;
    }
    CameraState& camera_state = mCamera.getSate();
    DragState& drag_state = mCamera.getDrag();
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        switch (action) {
            case GLFW_PRESS:
                drag_state.active = true;
                double xpos, ypos;
                glfwGetCursorPos(mRendererResource.window, &xpos, &ypos);
                drag_state.startMouse = glm::vec2(-(float)xpos, (float)ypos);
                drag_state.startCameraState = camera_state;
                break;
            case GLFW_RELEASE:
                drag_state.active = false;
                break;
        }
    }
}

void Application::onScroll(double /* xoffset */, double yoffset) {
    CameraState& camera_state = mCamera.getSate();
    DragState& drag_state = mCamera.getDrag();
    camera_state.zoom += drag_state.scrollSensitivity * static_cast<float>(yoffset);
    camera_state.zoom = glm::clamp(camera_state.zoom, -30.0f, 30.0f);
    updateViewMatrix();
}

void Application::updateDragInertia() {
    constexpr float eps = 1e-4f;
    CameraState& camera_state = mCamera.getSate();
    DragState& drag_state = mCamera.getDrag();
    // Apply inertia only when the user released the click.
    if (!drag_state.active) {
        // Avoid updating the matrix when the velocity is no longer noticeable
        if (std::abs(drag_state.velocity.x) < eps && std::abs(drag_state.velocity.y) < eps) {
            return;
        }
        camera_state.angles += drag_state.velocity;
        camera_state.angles.y = glm::clamp(camera_state.angles.y, -Camera::PI / 2 + 1e-5f, Camera::PI / 2 - 1e-5f);
        // Dampen the velocity so that it decreases exponentially and stops
        // after a few frames.
        drag_state.velocity *= drag_state.intertia;
        updateViewMatrix();
    }
}

// called in onInit
bool Application::initGui() {
    static ImGui_ImplWGPU_InitInfo imgui_device{};
    imgui_device.Device = mRendererResource.device;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(mRendererResource.window, true);
    //, 3, WGPUTextureFormat_BGRA8UnormSrgb, WGPUTextureFormat_Depth24Plus
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = mRendererResource.device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = WGPUTextureFormat_BGRA8UnormSrgb;
    init_info.DepthStencilFormat = WGPUTextureFormat_Depth24Plus;
    ImGui_ImplWGPU_Init(&init_info);
    std::cout << " failed to run1111" << std::endl;
    return true;
}
void Application::terminateGui() {
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplWGPU_Shutdown();
}  // called in onFinish

void Application::updateGui(WGPURenderPassEncoder renderPass) {
    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // [...] Build our UI
    // Build our UI
    // static float x = 0.0f, y = 0.0f, z = 0.0f;
    static glm::vec3& position = boat_model.getPosition();
    static float scale = 1;
    static int counter = 0;
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    glm::vec3 temp_pos = position;
    float scale_factor = scale;

    ImGui::SliderFloat("Boat x position", &position.x, -10.0f,
                       10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::SliderFloat("Boat y position", &position.y, -10.0f,
                       10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::SliderFloat("Boat z position", &position.z, -10.0f,
                       10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f

    ImGui::SliderFloat("Boat Scale", &scale, 0.0f, 10.0f);
    if (temp_pos != position) {
        std::cout << "value Changed" << '\n';
        tower_model.moveTo(position);
    }
    if (scale_factor != scale) {
        std::cout << "Scale Changed" << '\n';
        tower_model.scale(glm::vec3{(float)scale});
    }
    ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

    if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Begin("Lighting");
    ImGui::ColorEdit3("Color #0", glm::value_ptr(mLightingUniforms.colors[0]));
    ImGui::DragFloat3("Direction #0", glm::value_ptr(mLightingUniforms.directions[0]));
    ImGui::ColorEdit3("Color #1", glm::value_ptr(mLightingUniforms.colors[1]));
    ImGui::DragFloat3("Direction #1", glm::value_ptr(mLightingUniforms.directions[1]));
    ImGui::End();
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                         sizeof(LightingUniforms));

    // Draw the UI
    ImGui::EndFrame();
    // Convert the UI defined above into low-level drawing commands
    ImGui::Render();
    // Execute the low-level drawing commands on the WebGPU backend
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

RendererResource& Application::getRendererResource() { return mRendererResource; }

BindingGroup& Application::getBindingGroup() { return mBindingGroup; }

WGPUBuffer& Application::getUniformBuffer() { return mUniformBuffer; }

MyUniform& Application::getUniformData() { return mUniforms; }

const WGPUBindGroupLayout& Application::getObjectBindGroupLayout() const { return mBindGroupLayouts[1]; }