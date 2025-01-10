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

void MyUniform::setCamera(Camera& camera) {
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

void Application::initializePipeline() {
    // ---------------------------- Render pipeline

#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    initDepthBuffer();

    // creating default diffuse texture
    mDefaultDiffuse = new Texture{mRendererResource.device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_diffuse_texture_view = mDefaultDiffuse->createView();
    (void)default_diffuse_texture_view;
    std::vector<uint8_t> texture_data = {255, 0, 255, 255};  // Purple color for Default texture color
    mDefaultDiffuse->setBufferData(texture_data);
    mDefaultDiffuse->uploadToGPU(mRendererResource.queue);

    // Creating default meatlic-roughness texture
    mDefaultMetallicRoughness = new Texture{mRendererResource.device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_metallic_roughness_texture_view = mDefaultMetallicRoughness->createView();
    texture_data = {255, 255, 255, 255};  // White color for Default specular texture
    mDefaultMetallicRoughness->setBufferData(texture_data);
    mDefaultMetallicRoughness->uploadToGPU(mRendererResource.queue);

    mBindingGroup.addBuffer(0,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(MyUniform));

    mBindingGroup.addTexture(1,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);

    mBindingGroup.addSampler(2,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    mBindingGroup.addBuffer(3,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(LightingUniforms));

    mBindingGroup.addBuffer(4,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(PointLight));

    mBindingGroup.addTexture(5,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);

    WGPUBindGroupLayout bind_group_layout = mBindingGroup.createLayout(this, "binding group layout");

    WGPUBindGroupLayoutEntry object_transformation = {};
    setDefault(object_transformation);
    object_transformation.binding = 0;
    object_transformation.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    object_transformation.buffer.type = WGPUBufferBindingType_Uniform;
    object_transformation.buffer.minBindingSize = sizeof(ObjectInfo);
    // mBindingGroup.add(binding_layout_entries);
    WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor1 = {};
    bind_group_layout_descriptor1.nextInChain = nullptr;
    bind_group_layout_descriptor1.label = "Object Tranformation Matrix uniform";
    bind_group_layout_descriptor1.entryCount = 1;
    bind_group_layout_descriptor1.entries = &object_transformation;

    mBindGroupLayouts[0] = bind_group_layout;
    mBindGroupLayouts[1] = wgpuDeviceCreateBindGroupLayout(mRendererResource.device, &bind_group_layout_descriptor1);

    mPipeline = new Pipeline{{bind_group_layout, mBindGroupLayouts[1]}};
    mPipeline->defaultConfiguration(this, mSurfaceFormat).createPipeline(this);

    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mUniformBuffer;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(MyUniform);

    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = default_diffuse_texture_view;

    WGPUSamplerDescriptor samplerDesc = {};
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

    mBindingData[4].nextInChain = nullptr;
    mBindingData[4].binding = 4;
    mBindingData[4].buffer = mBuffer1;
    mBindingData[4].offset = 0;
    mBindingData[4].size = sizeof(PointLight);

    mBindingData[5] = {};
    mBindingData[5].nextInChain = nullptr;
    mBindingData[5].binding = 5;
    mBindingData[5].textureView = default_metallic_roughness_texture_view;

    mBindingGroup.create(this, mBindingData);

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    buffer_descriptor.label = "ahgmadasdsad f";

    // Create Uniform buffers
    buffer_descriptor.size = sizeof(ObjectInfo);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBufferTransform = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    ///////// Transformation bind group
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = mUniformBufferTransform;
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(ObjectInfo);

    // WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = mBindGroupLayouts[1];

    bindGrouptrans = wgpuDeviceCreateBindGroup(mRendererResource.device, &mTrasBindGroupDesc);
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    boat_model
        .load("boat", mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/fourareen.obj",
              mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0})
        .scale(glm::vec3{0.3})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);
    tower_model
        .load("tower", mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/tower.obj",
              mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0})
        .scale(glm::vec3{0.3})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);

    arrow_model
        .load("arrow", mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/arrow.obj",
              mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, 0.333, 2.0})
        .scale(glm::vec3{0.3})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);

    desk_model
        .load("desk", mRendererResource.device, mRendererResource.queue, RESOURCE_DIR "/desk.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, 0.333, 0.72})

        .scale(glm::vec3{0.3})
        .uploadToGPU(mRendererResource.device, mRendererResource.queue);

    // std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>....\n";

    terrain.generate(100, 8).uploadToGpu(this);

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

    WGPUBufferDescriptor pointligth_buffer_descriptor = {};
    pointligth_buffer_descriptor.nextInChain = nullptr;
    pointligth_buffer_descriptor.label = ":::::";
    pointligth_buffer_descriptor.size = sizeof(PointLight);
    pointligth_buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    pointligth_buffer_descriptor.mappedAtCreation = false;
    mBuffer1 = wgpuDeviceCreateBuffer(mRendererResource.device, &pointligth_buffer_descriptor);

    glm::vec4 red = {1.0, 0.0, 0.0, 1.0};
    mPointlight = PointLight{{0.0, 0.0, 1.0, 1.0}, red, red, red, 1.0, 0.7, 1.8};

    wgpuQueueWriteBuffer(mRendererResource.queue, mBuffer1, 0, &mPointlight, pointligth_buffer_descriptor.size);

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

static int WINDOW_WIDTH = 1920;
static int WINDOW_HEIGHT = 1080;

Model* Application::getModelCounter() {
    static size_t counter = 0;
    if (mLoadedModel.size() == 0) {
        return nullptr;
    }

    if (counter >= mLoadedModel.size()) {
        counter = 0;
    }
    Model* temp = mLoadedModel[counter];
    counter++;
    std::cout << "the counter is " << counter << std::endl;
    return temp;
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

    GLFWwindow* provided_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Learn WebGPU", nullptr, nullptr);
    if (!provided_window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwSwapInterval(0);
    // Set up Callbacks
    glfwSetWindowUserPointer(provided_window, this);  // set user pointer to be used in the callback function
    glfwSetFramebufferSizeCallback(provided_window, onWindowResize);
    glfwSetCursorPosCallback(provided_window, [](GLFWwindow* window, double xpos, double ypos) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        // if (that != nullptr) that->onMouseMove(xpos, ypos);

        if (that != nullptr) {
            DragState& drag_state = that->getCamera().getDrag();
            if (drag_state.active) that->getCamera().processMouse(xpos, ypos);
        }

        if (ypos <= 0) {
            glfwSetCursorPos(window, xpos, WINDOW_HEIGHT - 1);
            if (that != nullptr) that->getCamera().updateCursor(xpos, WINDOW_HEIGHT - 1);
        } else if (ypos > WINDOW_HEIGHT - 20) {
            glfwSetCursorPos(window, xpos, 1);
            if (that != nullptr) that->getCamera().updateCursor(xpos, 1);
        }

        if (xpos <= 0) {
            glfwSetCursorPos(window, WINDOW_WIDTH - 1, ypos);
            if (that != nullptr) that->getCamera().updateCursor(WINDOW_WIDTH - 1, ypos);
        } else if (xpos >= WINDOW_WIDTH - 1) {
            glfwSetCursorPos(window, 1, ypos);
            if (that != nullptr) that->getCamera().updateCursor(1, ypos);
        }
    });
    glfwSetMouseButtonCallback(provided_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->onMouseButton(button, action, mods);
    });
    glfwSetScrollCallback(provided_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        (void)xoffset;
        // if (that != nullptr) that->onScroll(xoffset, yoffset);
        if (that != nullptr) that->getCamera().processScroll(yoffset);
    });

    glfwSetKeyCallback(provided_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        that->getCamera().processInput(key, scancode, action, mods);
        if (GLFW_KEY_KP_0 == key && action == GLFW_PRESS) {
            Model* model = that->getModelCounter();
            if (model) {
                glm::vec3 temp_pos = model->getPosition();
                glm::vec3 temp_aabb_size = model->getAABBSize();
                float temp_smallest_size = std::min(temp_aabb_size.x, std::min(temp_aabb_size.y, temp_aabb_size.z));
                temp_pos.z += temp_smallest_size;
                temp_pos.x += temp_smallest_size;
                temp_pos.y += temp_smallest_size;
                that->getCamera().setPosition(temp_pos);
                that->getCamera().setTarget(model->getPosition() - temp_pos);
                std::cout << "volume calculation result  is: " << model->calculateVolume() << std::endl;
            }
        }
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
    // mBuffer1 = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    buffer_descriptor.label = "output buffer";
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    mBuffer2 = wgpuDeviceCreateBuffer(mRendererResource.device, &buffer_descriptor);

    // Writing to buffer
    std::vector<uint8_t> numbers{16};
    for (uint8_t i = 0; i < 16; i++) numbers[i] = i;

    // wgpuQueueWriteBuffer(mRendererResource.queue, mBuffer1, 0, numbers.data(), 16);

    // copy from buffer1 to buffer2
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, nullptr);

    // wgpuCommandEncoderCopyBufferToBuffer(encoder, mBuffer1, 0, mBuffer2, 0, 16);

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
    tower_model.moveTo({-2.0, -1.0, 0.0});

    mLoadedModel = {&boat_model, &tower_model, &arrow_model, &desk_model};

    return true;
}

void Application::updateViewMatrix() {
    // auto& camera_state = mCamera.getSate();
    // float cx = cos(camera_state.angles.x);
    // float sx = sin(camera_state.angles.x);
    // float cy = cos(camera_state.angles.y);
    // float sy = sin(camera_state.angles.y);
    // glm::vec3 position = glm::vec3(cx * cy, sx * cy, sy) * std::exp(-camera_state.zoom);
    // mCamera.setViewMatrix(glm::lookAt(position, glm::vec3(0.0f), glm::vec3(0, 0, 1)));
    // mUniforms.setCamera(mCamera);
    // wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, viewMatrix),
    //                      &mUniforms.viewMatrix, sizeof(MyUniform::viewMatrix));
}

void Application::mainLoop() {
    glfwPollEvents();

    WGPUTextureView target_view = getNextSurfaceTextureView();
    if (target_view == nullptr) {
        return;
    }

    mUniforms.time = static_cast<float>(glfwGetTime());
    float viewZ = glm::mix(0.0f, 0.25f, cos(2 * Camera::PI * mUniforms.time / 4) * 0.5 + 0.5);
    glm::vec3 focal_point{0.0, viewZ, -2.5};

    mUniforms.setCamera(mCamera);
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, viewMatrix),
                         &mUniforms.viewMatrix, sizeof(MyUniform::viewMatrix));
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, cameraWorldPosition),
                         &mCamera.getPos(), sizeof(MyUniform::cameraWorldPosition));

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
    render_pass_color_attachment.clearValue = WGPUColor{0.52, 0.80, 0.92, 1.0};
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

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline->getPipeline());

    tower_model.draw(this, render_pass_encoder, mBindingData);
    boat_model.draw(this, render_pass_encoder, mBindingData);
    arrow_model.draw(this, render_pass_encoder, mBindingData);
    desk_model.draw(this, render_pass_encoder, mBindingData);
    terrain.draw(this, render_pass_encoder, mBindingData);
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
    wgpuRenderPipelineRelease(mPipeline->getPipeline());
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
    required_limits.limits.maxSampledTexturesPerShaderStage = 2;
    required_limits.limits.maxInterStageShaderComponents = 14;

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

    wgpuSurfaceConfigure(mRendererResource.surface, &surface_configuration);
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

    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, projectMatrix),
                         &mUniforms.projectMatrix, sizeof(MyUniform::projectMatrix));
}

void Application::onMouseMove(double xpos, double ypos) {
    (void)xpos;
    (void)ypos;

    // CameraState& camera_state = mCamera.getSate();
    // DragState& drag_state = mCamera.getDrag();
    // if (drag_state.active) {
    //     glm::vec2 currentMouse = glm::vec2(-(float)xpos, (float)ypos);
    //     glm::vec2 delta = (currentMouse - drag_state.startMouse) * drag_state.sensitivity;
    //     camera_state.angles = drag_state.startCameraState.angles + delta;
    //     // Clamp to avoid going too far when orbitting up/down
    //     camera_state.angles.y = glm::clamp(camera_state.angles.y, -Camera::PI / 2 + 1e-5f, Camera::PI / 2 - 1e-5f);
    //     updateViewMatrix();
    //     // Inertia
    //     drag_state.velocity = delta - drag_state.previousDelta;
    //     drag_state.previousDelta = delta;
    // }
}

bool intersection(const glm::vec3& ray_origin, const glm::vec3& ray_dir, const glm::vec3& box_min,
                  const glm::vec3& box_max) {
    float tmin = 0.0, tmax = INFINITY;

    for (int d = 0; d < 3; ++d) {
        float t1 = (box_min[d] - ray_origin[d]) / ray_dir[d];
        float t2 = (box_max[d] - ray_origin[d]) / ray_dir[d];

        tmin = std::max(tmin, std::min(std::min(t1, t2), tmax));
        tmax = std::min(tmax, std::max(std::max(t1, t2), tmin));
    }

    return tmin < tmax;
}

void Application::onMouseButton(int button, int action, int /* modifiers */) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        // Don't rotate the camera if the mouse is already captured by an ImGui
        // interaction at this frame.
        return;
    }
    // CameraState& camera_state = mCamera.getSate();
    DragState& drag_state = mCamera.getDrag();
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        // calculating the NDC for x and y
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(mRendererResource.window, &xpos, &ypos);
            double xndc = 2.0 * xpos / 1920.0 - 1.0;
            double yndc = 1.0 - (2.0 * ypos) / 1024.0;
            glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
            // arrow_model.moveTo(mCamera.getPos());
            glm::vec4 eyecoord = glm::inverse(mCamera.getProjection()) * clip_coords;
            eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

            auto ray_origin = mCamera.getPos();

            glm::vec4 worldray = glm::inverse(mCamera.getView()) * eyecoord;
            auto normalized = glm::normalize(worldray);

            for (auto* obj : mLoadedModel) {
                auto obj_in_world_min = obj->getModelMatrix() * glm::vec4(obj->getAABB().min, 1.0);
                auto obj_in_world_max = obj->getModelMatrix() * glm::vec4(obj->getAABB().max, 1.0);
                bool does_intersect = intersection(ray_origin, normalized, obj_in_world_min, obj_in_world_max);
                if (does_intersect) {
                    mSelectedModel = obj;
                    std::cout << std::format("the ray {}intersect with {}\n", does_intersect ? "" : "does not ",
                                             obj->getName());
                }
            }
        }

    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        switch (action) {
            case GLFW_PRESS:
                drag_state.active = true;
                // double xpos, ypos;
                // glfwGetCursorPos(mRendererResource.window, &xpos, &ypos);
                // drag_state.startMouse = glm::vec2(-(float)xpos, (float)ypos);
                // drag_state.startCameraState = camera_state;
                break;
            case GLFW_RELEASE:
                drag_state.active = false;
                break;
        }
    }
}

void Application::onScroll(double /* xoffset */, double yoffset) {
    (void)yoffset;
    // CameraState& camera_state = mCamera.getSate();
    // DragState& drag_state = mCamera.getDrag();
    // camera_state.zoom += drag_state.scrollSensitivity * static_cast<float>(yoffset);
    // camera_state.zoom = glm::clamp(camera_state.zoom, -30.0f, 30.0f);
    updateViewMatrix();
}

void Application::updateDragInertia() {
    // constexpr float eps = 1e-4f;
    // CameraState& camera_state = mCamera.getSate();
    // DragState& drag_state = mCamera.getDrag();
    // // Apply inertia only when the user released the click.
    // if (!drag_state.active) {
    //     // Avoid updating the matrix when the velocity is no longer noticeable
    //     if (std::abs(drag_state.velocity.x) < eps && std::abs(drag_state.velocity.y) < eps) {
    //         return;
    //     }
    //     camera_state.angles += drag_state.velocity;
    //     camera_state.angles.y = glm::clamp(camera_state.angles.y, -Camera::PI / 2 + 1e-5f, Camera::PI / 2 - 1e-5f);
    //     // Dampen the velocity so that it decreases exponentially and stops
    //     // after a few frames.
    //     drag_state.velocity *= drag_state.intertia;
    //     updateViewMatrix();
    // }
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
    // static bool show_demo_window = true;
    // static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    if (mSelectedModel) {
        ImGui::Begin(mSelectedModel->getName().c_str());  // Create a window called "Hello, world!" and append into it.

        float scale = 1;
        glm::vec3 position = mSelectedModel->getPosition();
        float scale_factor = scale;

        ImGui::SliderFloat("X position", &position.x, -10.0f,
                           10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("Y position", &position.y, -10.0f,
                           10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("Z position", &position.z, -10.0f,
                           10.0f);  // Edit 1 float using a slider from 0.0f to 1.0f

        ImGui::SliderFloat("Scale", &scale, 0.0f, 10.0f);

        if (scale_factor != scale) {
            mSelectedModel->scale(glm::vec3{(float)scale});
        }
        ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color
        mSelectedModel->moveTo(position);

        static int counter = 0;
        // if (ImGui::Button("Button22")) {
        // std::cout << "Scale Changed " << &boat_model << " " << mSelectedModel << '\n';
        // boat_model.moveTo(glm::vec3{2.0, 2.0, 2.0});
        // };
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    ImGui::Begin("Lighting");
    ImGui::ColorEdit3("Color #0", glm::value_ptr(mLightingUniforms.colors[0]));
    ImGui::DragFloat3("Direction #0", glm::value_ptr(mLightingUniforms.directions[0]), 0.1, -1.0, 1.0);
    ImGui::ColorEdit3("Color #1", glm::value_ptr(mLightingUniforms.colors[1]));
    ImGui::DragFloat3("Direction #1", glm::value_ptr(mLightingUniforms.directions[1]));
    ImGui::End();
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                         sizeof(LightingUniforms));

    ImGui::Begin("Point Light");

    glm::vec4 tmp_ambient = mPointlight.mAmbient;
    glm::vec3 tmp_pos = arrow_model.getPosition();
    float tmp_linear = mPointlight.mLinear;
    float tmp_quadratic = mPointlight.mQuadratic;
    ImGui::ColorEdit3("Point Light Color #0", glm::value_ptr(mPointlight.mAmbient));
    ImGui::DragFloat3("Point Ligth Position #0", glm::value_ptr(mPointlight.mPosition), 0.1, -10.0, 10.0);
    ImGui::SliderFloat("Linear", &tmp_linear, -10.0f, 10.0f);
    ImGui::SliderFloat("Quadratic", &tmp_quadratic, -10.0f, 10.0f);

    if (tmp_ambient != mPointlight.mAmbient ||
        (tmp_pos.x != mPointlight.mPosition.x || tmp_pos.y != mPointlight.mPosition.y ||
         tmp_pos.z != mPointlight.mPosition.z || tmp_linear != mPointlight.mLinear ||
         tmp_quadratic != mPointlight.mQuadratic)) {
        mPointlight.mPosition = glm::vec4{tmp_pos, 1.0};
        mPointlight.mLinear = tmp_linear;
        mPointlight.mQuadratic = tmp_quadratic;
        wgpuQueueWriteBuffer(mRendererResource.queue, mBuffer1, 0, &mPointlight, sizeof(mPointlight));
    }
    ImGui::End();

    // Draw the UI
    ImGui::EndFrame();
    // Convert the UI defined above into low-level drawing commands
    ImGui::Render();
    // Execute the low-level drawing commands on the WebGPU backend
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

Camera& Application::getCamera() { return mCamera; }

RendererResource& Application::getRendererResource() { return mRendererResource; }

BindingGroup& Application::getBindingGroup() { return mBindingGroup; }

WGPUBuffer& Application::getUniformBuffer() { return mUniformBuffer; }

MyUniform& Application::getUniformData() { return mUniforms; }

const WGPUBindGroupLayout& Application::getObjectBindGroupLayout() const { return mBindGroupLayouts[1]; }