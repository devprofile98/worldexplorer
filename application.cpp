#include "application.h"

#include <array>
#include <cstdint>
#include <format>
#include <iostream>
#include <random>
#include <vector>

#include "GLFW/glfw3.h"
#include "composition_pass.h"
#include "frustum_culling.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_wgpu.h"
#include "imgui/imgui.h"
#include "instance.h"
#include "shapes.h"
#include "texture.h"
#include "transparency_pass.h"
#include "utils.h"
#include "webgpu.h"
#include "wgpu.h"
#include "wgpu_utils.h"

// #define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

static bool look_as_light = false;

void MyUniform::setCamera(Camera& camera) {
    projectMatrix = camera.getProjection();
    viewMatrix = camera.getView();
    modelMatrix = camera.getModel();
}

WGPUTextureFormat Application::getTextureFormat() { return mSurfaceFormat; }

WGPUSampler Application::getDefaultSampler() { return mDefaultSampler; }

void Application::initializePipeline() {
    // ---------------------------- Render pipeline

#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    initDepthBuffer();

    Texture grass_texture = Texture{mRendererResource.device, RESOURCE_DIR "/forrest_ground_diff.jpg"};
    grass_texture.createView();
    grass_texture.uploadToGPU(mRendererResource.queue);

    Texture rock_texture = Texture{mRendererResource.device, RESOURCE_DIR "/tiger_rock_diff.jpg"};
    rock_texture.createView();
    rock_texture.uploadToGPU(mRendererResource.queue);

    Texture sand_texture = Texture{mRendererResource.device, RESOURCE_DIR "/gravelly_sand_diff.jpg"};
    sand_texture.createView();
    sand_texture.uploadToGPU(mRendererResource.queue);

    Texture snow_texture = Texture{mRendererResource.device, RESOURCE_DIR "/snow_diff.jpg"};
    snow_texture.createView();
    snow_texture.uploadToGPU(mRendererResource.queue);

    // creating default diffuse texture
    mDefaultDiffuse = new Texture{mRendererResource.device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_diffuse_texture_view = mDefaultDiffuse->createView();
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
    mBindingGroup.addTexture(6,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);
    mBindingGroup.addTexture(7,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);
    mBindingGroup.addTexture(8,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);
    mBindingGroup.addTexture(9,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);
    mBindingGroup.addTexture(10,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH,
                             TextureViewDimension::VIEW_2D);
    mBindingGroup.addBuffer(11,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(Scene));
    mBindingGroup.addSampler(12,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Compare);

    mBindingGroup.addBuffer(13,  //
                            BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                            mInstanceManager->mBufferSize);

    mBindingGroup.addBuffer(14,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(float));

    WGPUBindGroupLayout bind_group_layout = mBindingGroup.createLayout(this, "binding group layout");

    WGPUBindGroupLayoutEntry object_transformation = {};
    setDefault(object_transformation);
    object_transformation.binding = 0;
    object_transformation.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    object_transformation.buffer.type = WGPUBufferBindingType_Uniform;
    object_transformation.buffer.minBindingSize = sizeof(ObjectInfo);

    WGPUBindGroupLayoutDescriptor bind_group_layout_descriptor1 = {};
    bind_group_layout_descriptor1.nextInChain = nullptr;
    bind_group_layout_descriptor1.label = "Object Tranformation Matrix uniform";
    bind_group_layout_descriptor1.entryCount = 1;
    bind_group_layout_descriptor1.entries = &object_transformation;

    mBindGroupLayouts[0] = bind_group_layout;
    mBindGroupLayouts[1] = wgpuDeviceCreateBindGroupLayout(mRendererResource.device, &bind_group_layout_descriptor1);

    mPipeline = new Pipeline{this, {bind_group_layout, mBindGroupLayouts[1]}};
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
    samplerDesc.maxAnisotropy = 16.0;
    mDefaultSampler = wgpuDeviceCreateSampler(mRendererResource.device, &samplerDesc);

    mBindingData[2].binding = 2;
    mBindingData[2].sampler = mDefaultSampler;

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

    mBindingData[6] = {};
    mBindingData[6].nextInChain = nullptr;
    mBindingData[6].binding = 6;
    mBindingData[6].textureView = grass_texture.getTextureView();

    mBindingData[7] = {};
    mBindingData[7].nextInChain = nullptr;
    mBindingData[7].binding = 7;
    mBindingData[7].textureView = rock_texture.getTextureView();

    mBindingData[8] = {};
    mBindingData[8].nextInChain = nullptr;
    mBindingData[8].binding = 8;
    mBindingData[8].textureView = sand_texture.getTextureView();

    mBindingData[9] = {};
    mBindingData[9].nextInChain = nullptr;
    mBindingData[9].binding = 9;
    mBindingData[9].textureView = snow_texture.getTextureView();

    mTransparencyPass = new TransparencyPass{this};
    mTransparencyPass->initializePass();

    mCompositionPass = new CompositionPass{this};
    mCompositionPass->initializePass();

    mShadowPass = new ShadowPass{this};
    mShadowPass->createRenderPass();
    // mShadowPass->setupScene(glm::vec3{0.5, -0.9, 0.1});

    mBindingData[10] = {};
    mBindingData[10].nextInChain = nullptr;
    mBindingData[10].binding = 10;
    mBindingData[10].textureView = mShadowPass->getShadowMapView();

    mBindingData[11].nextInChain = nullptr;
    mBindingData[11].binding = 11;
    mBindingData[11].buffer = mLightSpaceTransformation.getBuffer();
    mBindingData[11].offset = 0;
    mBindingData[11].size = sizeof(Scene);

    WGPUSamplerDescriptor shadow_sampler_desc = {};
    shadow_sampler_desc.addressModeU = WGPUAddressMode_ClampToEdge;
    shadow_sampler_desc.addressModeV = WGPUAddressMode_ClampToEdge;
    shadow_sampler_desc.addressModeW = WGPUAddressMode_ClampToEdge;
    shadow_sampler_desc.magFilter = WGPUFilterMode_Linear;
    shadow_sampler_desc.minFilter = WGPUFilterMode_Linear;
    shadow_sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    shadow_sampler_desc.lodMinClamp = 0.0f;
    shadow_sampler_desc.lodMaxClamp = 1.0f;
    shadow_sampler_desc.compare = WGPUCompareFunction_Greater;
    shadow_sampler_desc.maxAnisotropy = 1;
    WGPUSampler shadow_sampler = wgpuDeviceCreateSampler(mRendererResource.device, &shadow_sampler_desc);

    mBindingData[12] = {};
    mBindingData[12].nextInChain = nullptr;
    mBindingData[12].binding = 12;
    mBindingData[12].sampler = shadow_sampler;

    std::vector<glm::mat4> dddata = {};
    std::vector<glm::mat4> dddata_tree = {};
    /*dddata.reserve(100000);*/
    std::random_device rd;   // Seed the random number generator
    std::mt19937 gen(rd());  // Mersenne Twister PRNG
    std::uniform_real_distribution<float> dist(1.5, 2.5);
    std::uniform_real_distribution<float> dist_for_rotation(0.0, 180.0);
    std::uniform_real_distribution<float> dist_for_tree(1.0, 1.4);
    if (output.size() > 63690) {
        std::cout << "EEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRRRRRROOOOOOOOOOOOOOOOORRRRRRRRRRRRRRRR\n";
    }

    for (size_t i = 0; i < output.size(); i++) {
        glm::vec3 position = glm::vec3(output[i].x, output[i].y, output[i].z);
        auto trans = glm::translate(glm::mat4{1.0f}, position);
        auto rotate = glm::rotate(glm::mat4{1.0f}, glm::radians(dist_for_rotation(gen)), glm::vec3{0.0, 0.0, 1.0});
        auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f * dist(gen)});
        dddata.push_back(trans * rotate * scale);
        if (i % 40 == 0) {
            scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.9f * dist_for_tree(gen)});
            dddata_tree.push_back(trans * rotate * scale);
        }
    }
    glm::vec3 position = glm::vec3(-30.0, -30.0f, 0.0f);
    glm::mat4 transform = glm::mat4{1.0f};
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, glm::radians(0.0f), glm::vec3{1.0, 0.0, 0.0});
    transform = glm::scale(transform, glm::vec3{1.0, 1.0, 1.0});
    dddata[0] = transform;
    dddata_tree[0] = transform;

    wgpuQueueWriteBuffer(this->getRendererResource().queue, mInstanceManager->getInstancingBuffer().getBuffer(), 0,
                         dddata.data(), sizeof(glm::mat4) * (dddata.size() - 1));

    wgpuQueueWriteBuffer(this->getRendererResource().queue, mInstanceManager->getInstancingBuffer().getBuffer(),
                         100000 * sizeof(glm::mat4), dddata_tree.data(), sizeof(glm::mat4) * (dddata_tree.size() - 1));

    auto* ins = new Instance{dddata};
    auto* ins_tree = new Instance{dddata_tree};
    grass_model.setInstanced(ins);
    grass_model.mObjectInfo.instanceOffsetId = 0;

    tree_model.setInstanced(ins_tree);
    tree_model.mObjectInfo.instanceOffsetId = 1;

    mBindingData[13] = {};
    mBindingData[13].nextInChain = nullptr;
    mBindingData[13].buffer = mInstanceManager->getInstancingBuffer().getBuffer();
    mBindingData[13].binding = 13;
    mBindingData[13].offset = 0;
    mBindingData[13].size = mInstanceManager->mBufferSize;

    mTimeBuffer.setLabel("time buffer")
        .setSize(sizeof(float))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion(false)
        .create(this);

    mBindingData[14] = {};
    mBindingData[14].nextInChain = nullptr;
    mBindingData[14].buffer = mTimeBuffer.getBuffer();
    mBindingData[14].binding = 14;
    mBindingData[14].offset = 0;
    mBindingData[14].size = sizeof(float);

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

    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = mBindGroupLayouts[1];

    bindGrouptrans = wgpuDeviceCreateBindGroup(mRendererResource.device, &mTrasBindGroupDesc);
    mSkybox = new SkyBox{this, RESOURCE_DIR "/skybox"};
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    // initialize The instancing buffer
    mInstanceManager = new InstanceManager{this, sizeof(glm::mat4) * 100000 * 15, 100000};

    boat_model.load("boat", this, RESOURCE_DIR "/fourareen.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{-10.0, 0.0, 0.0})
        .scale(glm::vec3{0.3});
    boat_model.uploadToGPU(this);

    tower_model.load("tower", this, RESOURCE_DIR "/tower.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0})
        .scale(glm::vec3{0.3});
    tower_model.uploadToGPU(this);

    arrow_model.load("arrow", this, RESOURCE_DIR "/arrow.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{1.0f, 1.0f, 4.0f})
        .scale(glm::vec3{0.3});
    arrow_model.uploadToGPU(this);

    desk_model.load("desk", this, RESOURCE_DIR "/desk.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, 0.333, 0.72})
        .scale(glm::vec3{0.3});
    desk_model.uploadToGPU(this);

    grass_model.load("grass", this, RESOURCE_DIR "/grass.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, -1.0, 0.72})
        .scale(glm::vec3{0.3});
    grass_model.uploadToGPU(this);
    grass_model.setTransparent(false);
    grass_model.setFoliage();

    grass2_model.load("grass2", this, RESOURCE_DIR "/grass3.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.0, 0.0, 0.0})
        .scale(glm::vec3{0.5f});
    grass2_model.uploadToGPU(this);
    grass2_model.setTransparent(false);

    car.load("car", this, RESOURCE_DIR "/car.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, -1.0, 0.72})
        .scale(glm::vec3{0.2})
        .rotate(glm::vec3{0.0, 0.0, 1.0}, 90.0f);
    car.uploadToGPU(this);
    car.setTransparent(false);
    car.useTexture(false);

    tree_model.load("tree", this, RESOURCE_DIR "/tree1.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{0.725, -7.640, 1.125})
        .scale(glm::vec3{0.9});
    tree_model.uploadToGPU(this);
    tree_model.setTransparent(false);
    tree_model.setFoliage();

    water.load("water", this, RESOURCE_DIR "/bluecube.obj", mBindGroupLayouts[1])
        .moveTo(glm::vec3{-3.725, -7.640, -3.425})
        .scale(glm::vec3{30.0, 30.0, 1.0});
    water.uploadToGPU(this);
    water.setTransparent(false);
    water.useTexture(false);

    terrain.generate(100, 8, output).uploadToGpu(this);
    std::cout << "Generate is " << output.size() << '\n';

    shapes = new Cube{this};
    shapes->moveTo(glm::vec3{10.0f, 1.0f, 4.0f});
    /*shapes->setTransparent();*/

    plane = new Plane{this};
    plane->mName = "Plane";
    plane->moveTo(glm::vec3{3.0f, 1.0f, 4.0f}).scale(glm::vec3{0.01, 1.0, 1.0});
    /*plane->setTransparent();*/

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

    mLightSpaceTransformation.setLabel("Light space transform buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Scene))
        .setMappedAtCraetion()
        .create(this);

    mLightingUniforms.directions = {glm::vec4{0.5, 0.5, 0.4, 1.0}, glm::vec4{0.2, 0.4, 0.3, 1.0}};
    mLightingUniforms.colors = {glm::vec4{1.0, 0.9, 0.6, 1.0}, glm::vec4{0.6, 0.9, 1.0, 1.0}};
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

BaseModel* Application::getModelCounter() {
    static size_t counter = 0;
    if (mLoadedModel.size() == 0) {
        return nullptr;
    }

    if (counter >= mLoadedModel.size()) {
        counter = 0;
    }
    BaseModel* temp = mLoadedModel[counter];
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

    GLFWwindow* provided_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "World Explorer", nullptr, nullptr);
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
            BaseModel* model = that->getModelCounter();
            if (model) {
                glm::vec3 temp_pos = model->getPosition();
                glm::vec3 temp_aabb_size = model->getAABBSize();
                float temp_smallest_size = std::min(temp_aabb_size.x, std::min(temp_aabb_size.y, temp_aabb_size.z));
                temp_pos.z += temp_smallest_size;
                temp_pos.x += temp_smallest_size;
                temp_pos.y += temp_smallest_size;
                that->getCamera().setPosition(temp_pos);
                that->getCamera().setTarget(model->getPosition() - temp_pos);
                std::cout << "volume calculation result for " << model->getName() << " is: " << model->calculateVolume()
                          << std::endl;
            }
        } else if (GLFW_KEY_KP_1 == key && action == GLFW_PRESS) {
            look_as_light = !look_as_light;
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

    initializeBuffers();
    initializePipeline();

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, nullptr);
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(mRendererResource.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    boat_model.moveTo({-10.0, -4.0, 0.0});
    tower_model.moveTo({-2.0, -1.0, 0.0});

    mLoadedModel = {&boat_model, &tower_model, &desk_model, &arrow_model, &grass_model, &grass2_model,
                    &tree_model, &car,         &water,      shapes,       plane};

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
    mUniforms.setCamera(mCamera);
    if (look_as_light) {
        mUniforms.projectMatrix = mShadowPass->getScene().projection;
        mUniforms.viewMatrix = mShadowPass->getScene().view;
        mUniforms.modelMatrix = mShadowPass->getScene().model;
    }

    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, viewMatrix),
                         &mUniforms.viewMatrix, sizeof(MyUniform::viewMatrix));
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, cameraWorldPosition),
                         &mCamera.getPos(), sizeof(MyUniform::cameraWorldPosition));

    float time = glfwGetTime();
    wgpuQueueWriteBuffer(mRendererResource.queue, mTimeBuffer.getBuffer(), 0, &time, sizeof(float));

    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = "command encoder descriptor";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, &encoder_descriptor);
    updateDragInertia();

    // preprocessing
    // doing frustum culling

    Frustum frustum{};
    frustum.extractPlanes(mCamera.getProjection());

    // ---------------- 1 - Preparing for shadow pass ---------------
    // The first pass is the shadow pass, only based on the opaque objects

    /*mShadowPass->mRenderPassColorAttachment.view = target_view;*/
    WGPURenderPassEncoder shadow_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, mShadowPass->getRenderPassDescriptor());
    wgpuRenderPassEncoderSetPipeline(shadow_pass_encoder, mShadowPass->getPipeline()->getPipeline());
    mShadowPass->setupScene({1.0f, 1.0f, 4.0f});
    mShadowPass->render(mLoadedModel, shadow_pass_encoder);

    wgpuRenderPassEncoderEnd(shadow_pass_encoder);
    wgpuRenderPassEncoderRelease(shadow_pass_encoder);

    //-------------- End of shadow pass

    // ---------------- 2 - begining of the opaque object color pass ---------------
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
    (void)render_pass_descriptor;

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(mUniforms.viewMatrix));
    glm::mat4 mvp = mUniforms.projectMatrix * viewNoTranslation;
    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mSkybox->getPipeline()->getPipeline());
    mSkybox->draw(this, render_pass_encoder, mvp);

    wgpuQueueWriteBuffer(mRendererResource.queue, mLightSpaceTransformation.getBuffer(), 0, &mShadowPass->getScene(),
                         sizeof(Scene));

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline->getPipeline());

    /*for (const auto& model : mLoadedModel) {*/
    /*    if (model->getName() != "tower"){*/
    /*   	continue;*/
    /*    }*/
    /*    auto [in_world_min, in_world_max] = model->getWorldMin();*/
    /*    if (frustum.AABBTest(in_world_min, in_world_max)) {*/
    /*        model->selected(true);*/
    /*    } else {*/
    /*        model->selected(false);*/
    /*    }*/
    /*}*/
    // Drawing opaque objects in the world
    for (const auto& model : mLoadedModel) {
        auto [in_world_min, in_world_max] = model->getWorldMin();
        if (!model->isTransparent() && frustum.AABBTest(in_world_min, in_world_max)) {
            model->draw(this, render_pass_encoder, mBindingData);
        }
    }

    terrain.draw(this, render_pass_encoder, mBindingData);
    updateGui(render_pass_encoder);

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    // end of color render pass

    // ------------ 3- Transparent pass
    // Calculate the Accumulation Buffer from the transparent object, this pass does not draw
    // on the render Target
    auto transparency_pass_desc = mTransparencyPass->getRenderPassDescriptor();
    mTransparencyPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;
    WGPURenderPassEncoder transparency_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, transparency_pass_desc);
    wgpuRenderPassEncoderSetPipeline(transparency_pass_encoder, mTransparencyPass->getPipeline()->getPipeline());

    /*mShadowPass->setupScene({1.0f, 1.0f, 4.0f});*/
    /*mTransparencyPass->render(mLoadedModel, transparency_pass_encoder, mDepthTextureView);*/

    wgpuRenderPassEncoderEnd(transparency_pass_encoder);
    wgpuRenderPassEncoderRelease(transparency_pass_encoder);

    // ------------ 4- Composition pass
    // In this pass we will compose the result from opaque pass and the transparent pass
    auto ssbo_buffers = mTransparencyPass->getSSBOBuffers();
    mCompositionPass->setSSBOBuffers(ssbo_buffers.first, ssbo_buffers.second);
    mCompositionPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;
    mCompositionPass->mRenderPassColorAttachment.view = target_view;
    auto composition_pass_desc = mCompositionPass->getRenderPassDescriptor();
    WGPURenderPassEncoder composition_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, composition_pass_desc);
    wgpuRenderPassEncoderSetPipeline(composition_pass_encoder, mCompositionPass->getPipeline()->getPipeline());

    /*mCompositionPass->render(mLoadedModel, composition_pass_encoder, &render_pass_color_attachment);*/

    wgpuRenderPassEncoderEnd(composition_pass_encoder);
    wgpuRenderPassEncoderRelease(composition_pass_encoder);

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
    wgpuBufferRelease(mUniformBuffer);
    terminateGui();

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
    required_limits.limits.maxBufferSize = 134217728;  // 1000000 * sizeof(VertexAttributes);
    required_limits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    required_limits.limits.maxSampledTexturesPerShaderStage = 7;
    required_limits.limits.maxInterStageShaderComponents = 116;

    // Binding groups
    required_limits.limits.maxBindGroups = 3;
    required_limits.limits.maxUniformBuffersPerShaderStage = 5;
    required_limits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);

    required_limits.limits.maxTextureDimension1D = 2048;
    required_limits.limits.maxTextureDimension2D = 2048;
    required_limits.limits.maxTextureArrayLayers = 6;
    required_limits.limits.maxSamplersPerShaderStage = 2;

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
    depth_texture_descriptor.label = "Depth texture for framebuffer";
    depth_texture_descriptor.dimension = WGPUTextureDimension_2D;
    depth_texture_descriptor.format = depth_texture_format;
    depth_texture_descriptor.mipLevelCount = 1;
    depth_texture_descriptor.sampleCount = 1;
    depth_texture_descriptor.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    depth_texture_descriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
    depth_texture_descriptor.viewFormatCount = 1;
    depth_texture_descriptor.viewFormats = &depth_texture_format;
    mDepthTexture = wgpuDeviceCreateTexture(mRendererResource.device, &depth_texture_descriptor);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depth_texture_view_desc = {};
    depth_texture_view_desc.label = "opaque path depth texture";
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
    mUniforms.projectMatrix = glm::perspective(60 * Camera::PI / 180, ratio, 0.01f, 1000.0f);

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
                /*auto obj = dynamic_cast<BaseModel*>(obj1);*/
                /*if (obj) {*/
                auto obj_in_world_min = obj->getTranformMatrix() * glm::vec4(obj->min, 1.0);
                auto obj_in_world_max = obj->getTranformMatrix() * glm::vec4(obj->max, 1.0);
                bool does_intersect = intersection(ray_origin, normalized, obj_in_world_min, obj_in_world_max);
                if (does_intersect) {
                    if (mSelectedModel) {
                        mSelectedModel->selected(false);
                    }
                    mSelectedModel = obj;
                    mSelectedModel->selected(true);
                    std::cout << std::format("the ray {}intersect with {}\n", "", obj->getName());
                    break;
                }
            }
        }

    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        switch (action) {
            case GLFW_PRESS:
                drag_state.active = true;
                drag_state.firstMouse = true;
                break;
            case GLFW_RELEASE:
                drag_state.active = false;
                drag_state.firstMouse = false;
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

    if (mSelectedModel) {
        ImGui::Begin(mSelectedModel->getName().c_str());  // Create a window called "Hello, world!" and append into it.

#ifdef DEVELOPMENT_BUILD
        mSelectedModel->userInterface();
#endif  // DEVELOPMENT_BUILD

        ImGui::SameLine();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    static glm::vec3 pointlightshadow = glm::vec3{5.6f, -2.1f, 6.0f};
    ImGui::Begin("Lighting");
    ImGui::ColorEdit3("Color #0", glm::value_ptr(mLightingUniforms.colors[0]));
    ImGui::DragFloat3("Direction #0", glm::value_ptr(mLightingUniforms.directions[0]), 0.1, -1.0, 1.0);
    // ImGui::ColorEdit3("Color #1", glm::value_ptr(mLightingUniforms.colors[1]));
    ImGui::DragFloat3("sun pos direction", glm::value_ptr(pointlightshadow), 0.1, -10.0, 10.0);
    ImGui::End();
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                         sizeof(LightingUniforms));
    mShadowPass->lightPos = pointlightshadow;
    mShadowPass->setupScene({1.0f, 1.0f, 4.0f});

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
