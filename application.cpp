#include "application.h"

#include <cstdint>
#include <format>
#include <iostream>
#include <vector>

#include "texture.h"
#include "utils.h"
#include "wgpu_utils.h"

void MyUniform::setCamera(const Camera& camera) {
    projectMatrix = camera.getProjection();
    viewMatrix = camera.getView();
    modelMatrix = camera.getModel();
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

    WGPUShaderModule shader_module = loadShader(RESOURCE_DIR "/shader.wgsl", mDevice);

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

    WGPUTextureDescriptor depth_texture_descriptor = {};
    depth_texture_descriptor.nextInChain = nullptr;
    depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    depth_texture_descriptor.format = depth_texture_format;
    depth_texture_descriptor.mipLevelCount = 1;
    depth_texture_descriptor.sampleCount = 1;
    depth_texture_descriptor.size = {640, 480, 1};
    depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment;
    depth_texture_descriptor.viewFormatCount = 1;
    depth_texture_descriptor.viewFormats = &depth_texture_format;
    WGPUTexture depth_texture = wgpuDeviceCreateTexture(mDevice, &depth_texture_descriptor);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depth_texture_view_desc;
    depth_texture_view_desc.aspect = WGPUTextureAspect_DepthOnly;
    depth_texture_view_desc.baseArrayLayer = 0;
    depth_texture_view_desc.arrayLayerCount = 1;
    depth_texture_view_desc.baseMipLevel = 0;
    depth_texture_view_desc.mipLevelCount = 1;
    depth_texture_view_desc.dimension = WGPUTextureViewDimension_2D;
    depth_texture_view_desc.format = depth_texture_format;
    mDepthTextureView = wgpuTextureCreateView(depth_texture, &depth_texture_view_desc);

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

    Texture simple_texture{mDevice, RESOURCE_DIR "/fourareen2K_albedo.jpg"};

    std::vector<uint8_t> pixels(4 * 256 * 256);
    for (uint32_t i = 0; i < 256; ++i) {
        for (uint32_t j = 0; j < 256; ++j) {
            uint8_t* p = &pixels[4 * (j * 256 + i)];
            p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0;  // r
            p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0;       // g
            p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0;       // b
            p[3] = 255;                                     // a
        }
    }
    // for (uint32_t i = 0; i < 256; ++i) {
    //     for (uint32_t j = 0; j < 256; ++j) {
    //         uint8_t* p = &pixels[4 * (j * 256 + i)];
    //         p[0] = (uint8_t)i;  // r
    //         p[1] = (uint8_t)j;  // g
    //         p[2] = 128;         // b
    //         p[3] = 255;         // a
    //     }
    // }
    // simple_texture.setBufferData(pixels).uploadToGPU(mQueue);
    simple_texture.uploadToGPU(mQueue);

    // Uniform variable and Binding groups
    // std::vector<WGPUBindGroupLayoutEntry> binding_layout_entries{3};
    std::cout << std::format("this log Stop Application from crashing");

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

    WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor = {};
    bind_group_layout_descriptor.nextInChain = nullptr;
    bind_group_layout_descriptor.label = "binding group layout";
    bind_group_layout_descriptor.entryCount = mBindingGroup.getEntryCount();
    bind_group_layout_descriptor.entries = mBindingGroup.getEntryData();
    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(mDevice, &bind_group_layout_descriptor);

    WGPUPipelineLayoutDescriptor pipeline_layout_descriptor = {};
    pipeline_layout_descriptor.nextInChain = nullptr;
    pipeline_layout_descriptor.bindGroupLayoutCount = 1;
    pipeline_layout_descriptor.bindGroupLayouts = &bind_group_layout;
    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(mDevice, &pipeline_layout_descriptor);
    pipeline_descriptor.layout = pipeline_layout;

    std::vector<WGPUBindGroupEntry> binding{3};

    binding[0].nextInChain = nullptr;
    binding[0].binding = 0;
    binding[0].buffer = mUniformBuffer;
    binding[0].offset = 0;
    binding[0].size = sizeof(MyUniform);

    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 8;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
    WGPUTextureView textureView = wgpuTextureCreateView(simple_texture.getTexture(), &textureViewDesc);

    binding[1].nextInChain = nullptr;
    binding[1].binding = 1;
    binding[1].textureView = textureView;

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
    WGPUSampler sampler = wgpuDeviceCreateSampler(mDevice, &samplerDesc);

    binding[2].binding = 2;
    binding[2].sampler = sampler;

    WGPUBindGroupDescriptor bind_group_descriptor = {};
    bind_group_descriptor.nextInChain = nullptr;
    bind_group_descriptor.layout = bind_group_layout;
    bind_group_descriptor.entryCount = bind_group_layout_descriptor.entryCount;
    bind_group_descriptor.entries = binding.data();

    mBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bind_group_descriptor);

    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipeline_descriptor);

    // ---------------------------- End of Render Pipeline
}

void setupCamera(MyUniform& uniform) {
    constexpr float PI = 3.14159265358979323846f;
    float angle = (float)glfwGetTime();

    glm::mat4 S = glm::scale(glm::mat4{1.0f}, glm::vec3{0.2f});
    glm::mat4 T = glm::translate(glm::mat4{1.0f}, glm::vec3{-0.1f, 0.25f, 0.2f});
    glm::mat4 R1 = glm::rotate(glm::mat4{1.0f}, -angle, glm::vec3{0.0, 0.0, 1.0});

    uniform.modelMatrix = R1 * T * S;

    glm::vec3 focal_point{0.0, 0.0, -2.0};
    glm::mat4 T2 = glm::translate(glm::mat4{1.0f}, -focal_point);

    // Rotate the view point
    float angle2 = 3.0 * PI / 4.0;
    glm::mat4 R2 = glm::rotate(glm::mat4{1.0f}, -angle2, glm::vec3{1.0, 0.0, 0.0});

    uniform.viewMatrix = T2 * R2;

    float ratio = 640.0f / 480.0f;
    float focal_length = 2.0;
    float near = 0.01f;
    float far = 100.0f;
    float divider = 1.0f / (focal_length * (far - near));
    float fov = 2 * glm::atan(1 / focal_length);
    glm::mat4 P = glm::perspective(fov, ratio, near, far);

    P = glm::transpose(glm::mat4{
        1.0, 0.0, 0.0, 0.0,                              //
        0.0, ratio, 0.0, 0.0,                            //
        0.0, 0.0, far * divider, -far * near * divider,  //
        0.0, 0.0, 1.0 / focal_length, 0.0,               //
    });

    uniform.projectMatrix = P;
};

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    std::vector<float> point_data = {
        // x,   y,     r,   g,   b
        -0.5, -0.5, 1.0, 0.0, 0.0,  //
        +0.5, -0.5, 0.0, 1.0, 0.0,  //
        +0.5, +0.5, 0.0, 0.0, 1.0,  //
        -0.5, +0.5, 1.0, 1.0, 0.0   //
    };

    std::vector<uint16_t> index_data = {
        0, 1, 2,  // Triangle #0 connects points #0, #1 and #2
        0, 2, 3   // Triangle #1 connects points #0, #2 and #3
    };
    std::vector<VertexAttributes> vertex_data;
    if (!loadGeometryFromObj(RESOURCE_DIR "/fourareen.obj", vertex_data)) {
        std::cerr << "Could not load geometry!" << std::endl;
        return;
    }
    // loadGeometry(RESOURCE_DIR "/pyramid.txt", point_data, index_data, 6);
    // std::cout << index_data.size() << " Size of the point buffe is : " << point_data.size() << std::endl;
    // for (size_t i = 0; i < point_data.size(); i++) {
    //     std::cout << point_data[i] << ",";
    //     if (i % 5 == 0 && i != 0) {
    //         std::cout << "\n";
    //     }
    // }
    mIndexCount = static_cast<int>(vertex_data.size());

    // mVertexCount = static_cast<uint32_t>(point_data.size() / 5);

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    buffer_descriptor.size = vertex_data.size() * sizeof(VertexAttributes);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    buffer_descriptor.mappedAtCreation = false;

    mVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &buffer_descriptor);
    // Uploading data to GPU
    wgpuQueueWriteBuffer(mQueue, mVertexBuffer, 0, vertex_data.data(), buffer_descriptor.size);

    buffer_descriptor.size = mIndexCount * sizeof(uint16_t);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    buffer_descriptor.size = (buffer_descriptor.size + 3) & ~3;
    mIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &buffer_descriptor);
    wgpuQueueWriteBuffer(mQueue, mIndexBuffer, 0, index_data.data(), buffer_descriptor.size);

    // Create Uniform buffers
    buffer_descriptor.size = sizeof(MyUniform);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBuffer = wgpuDeviceCreateBuffer(mDevice, &buffer_descriptor);

    mUniforms.time = 1.0f;
    mUniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
    // setupCamera(mUniforms);

    // glm::mat4 S = glm::scale(glm::mat4{1.0f}, glm::vec3{0.2f});
    // glm::mat4 T = glm::translate(glm::mat4{1.0f}, glm::vec3{-0.1f, 0.25f, 0.2f});
    // glm::mat4 R1 = glm::rotate(glm::mat4{1.0f}, -angle, glm::vec3{0.0, 0.0, 1.0});
    mCamera = Camera{{-0.1f, -0.4f, 0.0f}, glm::vec3{0.8f}, {1.0, 0.0, 0.0}, 0.0};
    std::cerr << "Could not initialize GLFW! ------------ " << std::endl;
    mUniforms.setCamera(mCamera);
    wgpuQueueWriteBuffer(mQueue, mUniformBuffer, 0, &mUniforms, buffer_descriptor.size);
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
    wgpuQueueWriteBuffer(mQueue, mUniformBuffer, 0, &mUniforms, sizeof(MyUniform));

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
    wgpuRenderPassEncoderSetVertexBuffer(render_pass_encoder, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(render_pass_encoder, mIndexBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mIndexBuffer));
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mBindGroup, 0, nullptr);

    // Draw 1 instance of a 3-vertices shape
    // wgpuRenderPassEncoderDrawIndexed(render_pass_encoder, mIndexCount, 1, 0, 0, 0);
    wgpuRenderPassEncoderDraw(render_pass_encoder, mIndexCount, 1, 0, 0);

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
    wgpuBufferRelease(mUniformBuffer);
    wgpuBufferRelease(mIndexBuffer);
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
    required_limits.limits.maxBindGroups = 1;
    required_limits.limits.maxUniformBuffersPerShaderStage = 1;
    required_limits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

    required_limits.limits.maxTextureDimension1D = 2048;
    required_limits.limits.maxTextureDimension2D = 2048;
    required_limits.limits.maxTextureArrayLayers = 1;
    required_limits.limits.maxSamplersPerShaderStage = 1;

    return required_limits;
}