#include "application.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "animation.h"
#include "binding_group.h"
#include "camera.h"
#include "glm/detail/qualifier.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/matrix.hpp"
#include "mesh.h"
#include "physics.h"
#include "renderpass.h"
#include "shapes.h"
#include "skybox.h"
#include "world.h"

#define GLM_ENABLE_EXPERIMENTAL
// #include <Jolt/Core/Factory.h>
#include <Jolt/Jolt.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <nfd.hpp>

#include "GLFW/glfw3.h"
#include "composition_pass.h"
#include "editor.h"
#include "frustum_culling.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "gpu_buffer.h"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "pipeline.h"
#include "point_light.h"
#include "profiling.h"
#include "rendererResource.h"
#include "shadow_pass.h"
#include "shapes.h"
#include "terrain_pass.h"
#include "texture.h"
#include "transparency_pass.h"
#include "utils.h"
#include "water_pass.h"
#include "webgpu/webgpu.h"
#include "wgpu_utils.h"
#include "window.h"

nfdu8char_t* outPath;
nfdopendialogu8args_t args = {0};

static bool cull_frustum = false;
static bool show_physic_objects = false;
static bool show_physic_debugs = false;
static bool runPhysics = false;
static Model* selectedPhysicModel = nullptr;

bool flip_x = false;
bool flip_y = false;
bool flip_z = false;

static float middle_plane_length = 15.0f;
static float far_plane_length = 20.5f;
bool should_update_csm = true;

static auto position_offset = glm::vec3{5, 0.5, 0.7};
static auto scale_offset = glm::vec3{0.9};
static auto rotation_offset = glm::quat{1.0, {0.0, -.1, 0.7}};

WGPUTextureView getNextSurfaceTextureView(RendererResource& resources);
WGPULimits GetRequiredLimits(WGPUAdapter adapter);
bool initSwapChain(RendererResource& resources, uint32_t width, uint32_t height);

static LineGroup debuglinegroup;
static LineGroup spheredebuglines;
static LineGroup aabbDebugLines;

WGPUTextureFormat Application::getTextureFormat() { return mSurfaceFormat; }

WGPUSampler Application::getDefaultSampler() { return mDefaultSampler; }

void Application::initializePipeline() {
    // ---------------------------- Render pipeline

#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    // creating default diffuse texture
    mDefaultDiffuse = new Texture{this->getRendererResource().device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_diffuse_texture_view = mDefaultDiffuse->createView();
    std::vector<uint8_t> texture_data = {255, 0, 255, 255};  // Purple color for Default texture color
    mDefaultDiffuse->setBufferData(texture_data);
    mDefaultDiffuse->uploadToGPU(this->getRendererResource().queue);

    // Creating default meatlic-roughness texture
    mDefaultMetallicRoughness = new Texture{this->getRendererResource().device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_metallic_roughness_texture_view = mDefaultMetallicRoughness->createView();
    texture_data = {255, 120, 10, 255};
    mDefaultMetallicRoughness->setBufferData(texture_data);
    mDefaultMetallicRoughness->uploadToGPU(this->getRendererResource().queue);

    // Creating default normal-map texture
    mDefaultNormalMap = new Texture{this->getRendererResource().device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_normal_map_view = mDefaultNormalMap->createView();
    texture_data = {0, 255, 0, 255};
    mDefaultNormalMap->setBufferData(texture_data);
    mDefaultNormalMap->uploadToGPU(this->getRendererResource().queue);

    auto resource = getRendererResource();
    // Initializing Default bindgroups
    WGPUBindGroupLayout bind_group_layout =
        mBindingGroup
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                       sizeof(CameraInfo) * 10)
            .addBuffer(1, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t))
            .addSampler(2, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering)
            .addBuffer(3, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(LightingUniforms))
            .addBuffer(4, BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(Light) * 10)
            .addBuffer(5, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t))
            .addTexture(6, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH, TextureViewDimension::ARRAY_2D)
            .addBuffer(7, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(Scene) * 5)
            .addSampler(8, BindGroupEntryVisibility::FRAGMENT, SampleType::Compare)
            .addBuffer(9, BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                       mInstanceManager->mBufferSize)
            .createLayout(resource, "binding group layout");

    /* Default textures for the render pass, if a model doenst have textures, these will be used */
    WGPUBindGroupLayout texture_bind_group_layout =
        mDefaultTextureBindingGroup
            .addTexture(0, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::VIEW_2D)
            .addTexture(1, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::VIEW_2D)
            .addTexture(2, BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::FLAOT,
                        TextureViewDimension::VIEW_2D)
            .createLayout(resource, "default texture bindgroup layout");

    /* Default camera index buffer
     * Each render pass could have its own camear index to use a different camera in shader
     * */
    WGPUBindGroupLayout camera_bind_group_layout =
        mDefaultCameraIndexBindgroup
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t))
            .createLayout(resource, "default camera index bindgroup layout");

    WGPUBindGroupLayout clipplane_bind_group_layout =
        mDefaultClipPlaneBG
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(glm::vec4))
            .createLayout(resource, "default clip plane bindgroup layout");

    WGPUBindGroupLayout visible_bind_group_layout =
        mDefaultVisibleBuffer
            .addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                       sizeof(uint32_t) * 13000)
            .createLayout(resource, "default visible index layout");

    /* Default Object Detail bindgroup*/
    BindingGroup object_information;
    WGPUBindGroupLayout obj_transform_layout =
        object_information
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(ObjectInfo))
            .addBuffer(1, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, 100 * sizeof(glm::mat4))
            .addBuffer(2, BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY, 20 * sizeof(glm::mat4))
            .createLayout(resource, "Object Tranformation Matrix uniform");

    BindingGroup default_mesh_information;
    WGPUBindGroupLayout default_mesh_mat_layout =
        default_mesh_information
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(Material))
            .addBuffer(1, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t))
            .createLayout(resource, "mesh material Matrix uniform");

    mBindGroupLayouts = {bind_group_layout,        obj_transform_layout,        texture_bind_group_layout,
                         camera_bind_group_layout, clipplane_bind_group_layout, visible_bind_group_layout,
                         default_mesh_mat_layout};

    mPipeline = new Pipeline{this,
                             {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2], mBindGroupLayouts[3],
                              mBindGroupLayouts[4], mBindGroupLayouts[5], mBindGroupLayouts[6]},
                             "standard pipeline"};

    mPipeline->defaultConfiguration(resource, mSurfaceFormat);
    setDefaultActiveStencil(mPipeline->getDepthStencilState());
    mPipeline->setDepthStencilState(mPipeline->getDepthStencilState());
    mPipeline->createPipeline(resource);

    mStenctilEnabledPipeline =
        new Pipeline{this,
                     {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2], mBindGroupLayouts[3],
                      mBindGroupLayouts[4], mBindGroupLayouts[5], mBindGroupLayouts[6]},
                     "Draw outline pipe"};
    mStenctilEnabledPipeline->defaultConfiguration(resource, mSurfaceFormat, WGPUTextureFormat_Depth24PlusStencil8)
        .createPipeline(resource);

    mDefaultTextureBindingData[0] = {};
    mDefaultTextureBindingData[0].nextInChain = nullptr;
    mDefaultTextureBindingData[0].binding = 0;
    mDefaultTextureBindingData[0].textureView = default_diffuse_texture_view;

    mDefaultTextureBindingData[1] = {};
    mDefaultTextureBindingData[1].nextInChain = nullptr;
    mDefaultTextureBindingData[1].binding = 1;
    mDefaultTextureBindingData[1].textureView = default_metallic_roughness_texture_view;

    mDefaultTextureBindingData[2] = {};
    mDefaultTextureBindingData[2].nextInChain = nullptr;
    mDefaultTextureBindingData[2].binding = 2;
    mDefaultTextureBindingData[2].textureView = default_normal_map_view;

    mDefaultCameraIndex.setLabel("defualt camera index buffer")
        .setMappedAtCraetion()
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mRendererResource);

    static uint32_t cidx = 0;
    mDefaultCameraIndex.queueWrite(0, &cidx, sizeof(uint32_t));
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, mDefaultCameraIndex.getBuffer(), 0, &cidx,
    //                      sizeof(uint32_t));

    mDefaultCameraIndexBindingData[0] = {};
    mDefaultCameraIndexBindingData[0].nextInChain = nullptr;
    mDefaultCameraIndexBindingData[0].binding = 0;
    mDefaultCameraIndexBindingData[0].buffer = mDefaultCameraIndex.getBuffer();
    mDefaultCameraIndexBindingData[0].size = sizeof(uint32_t);

    // Create Default clip plane buffer
    mDefaultClipPlaneBuf.setMappedAtCraetion()
        .setLabel("default cilp plane buffer")
        .setSize(sizeof(glm::vec4))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(mRendererResource);

    mDefaultClipPlaneBuf.queueWrite(0, glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));
    // glm::vec4 default_clip_plane{0.0, 0.0, 1.0, 100};
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, mDefaultClipPlaneBuf.getBuffer(), 0,
    //                      glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));

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
    mDefaultSampler = wgpuDeviceCreateSampler(this->getRendererResource().device, &samplerDesc);

    mDefaultClipPlaneBGData[0].nextInChain = nullptr;
    mDefaultClipPlaneBGData[0].binding = 0;
    mDefaultClipPlaneBGData[0].buffer = mDefaultClipPlaneBuf.getBuffer();
    mDefaultClipPlaneBGData[0].offset = 0;
    mDefaultClipPlaneBGData[0].size = sizeof(glm::vec4);

    /* Initializing Render Passes */
    mShadowPass = new ShadowPass{this, "Shadow pass"};
    mShadowPass->createRenderPass(WGPUTextureFormat_RGBA8Unorm, 3);

    mDepthPrePass = new DepthPrePass{this, "Depth PrePass", mDepthTextureView};
    mDepthPrePass->createRenderPass(WGPUTextureFormat_RGBA8Unorm);

    mTerrainPass = new TerrainPass{this, "Terrain Render Pass"};
    mTerrainPass->create(mSurfaceFormat);

    m3DviewportPass = new ViewPort3DPass{this, "ViewPort 3D Render Pass"};

    initDepthBuffer();

    m3DviewportPass->create(mSurfaceFormat);

    mOutlinePass = new OutlinePass{this, "Outline Render Pass"};
    mOutlinePass->create(mSurfaceFormat, mDepthTextureViewDepthOnly);

    // mWaterRenderPass = new WaterPass{this, "Water pass"};
    // mWaterRenderPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

    /*mTransparencyPass = new TransparencyPass{this};*/
    /*mTransparencyPass->initializePass();*/
    /**/
    /*mCompositionPass = new CompositionPass{this};*/
    /*mCompositionPass->initializePass();*/

    /* Initializing Binding Data for bindgroups */
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mUniformBuffer.getBuffer();
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(CameraInfo) * 10;

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = mLightManager->getCountBuffer().getBuffer();
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = sizeof(uint32_t);

    mBindingData[2].binding = 2;
    mBindingData[2].sampler = mDefaultSampler;

    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].buffer = mDirectionalLightBuffer.getBuffer();
    mBindingData[3].offset = 0;
    mBindingData[3].size = sizeof(LightingUniforms);

    mBindingData[4].nextInChain = nullptr;
    mBindingData[4].binding = 4;
    mBindingData[4].buffer = mLightBuffer.getBuffer();
    mBindingData[4].offset = 0;
    mBindingData[4].size = sizeof(Light) * 10;

    mTimeBuffer.setLabel("number of cascades buffer")
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion(false)
        .create(mRendererResource);

    mBindingData[5] = {};
    mBindingData[5].nextInChain = nullptr;
    mBindingData[5].buffer = mTimeBuffer.getBuffer();
    mBindingData[5].binding = 5;
    mBindingData[5].offset = 0;
    mBindingData[5].size = sizeof(uint32_t);

    mBindingData[6] = {};
    mBindingData[6].nextInChain = nullptr;
    mBindingData[6].binding = 6;
    mBindingData[6].textureView = mShadowPass->getShadowMapView();

    mBindingData[7].nextInChain = nullptr;
    mBindingData[7].binding = 7;
    mBindingData[7].buffer = mLightSpaceTransformation.getBuffer();
    mBindingData[7].offset = 0;
    mBindingData[7].size = sizeof(Scene) * 5;

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
    WGPUSampler shadow_sampler = wgpuDeviceCreateSampler(this->getRendererResource().device, &shadow_sampler_desc);

    mBindingData[8] = {};
    mBindingData[8].nextInChain = nullptr;
    mBindingData[8].binding = 8;
    mBindingData[8].sampler = shadow_sampler;

    mBindingData[9] = {};
    mBindingData[9].nextInChain = nullptr;
    mBindingData[9].buffer = mInstanceManager->getInstancingBuffer().getBuffer();
    mBindingData[9].binding = 9;
    mBindingData[9].offset = 0;
    mBindingData[9].size = mInstanceManager->mBufferSize;

    mDefaultVisibleBGData[0] = {};
    mDefaultVisibleBGData[0].nextInChain = nullptr;
    mDefaultVisibleBGData[0].buffer = mVisibleIndexBuffer.getBuffer();
    mDefaultVisibleBGData[0].binding = 0;
    mDefaultVisibleBGData[0].offset = 0;
    mDefaultVisibleBGData[0].size = sizeof(uint32_t) * 100'000 * 5;

    /* Creating actual bindgroups */
    // auto& resource = getRendererResource();
    mBindingGroup.create(resource, mBindingData);
    mDefaultTextureBindingGroup.create(resource, mDefaultTextureBindingData);
    mDefaultCameraIndexBindgroup.create(resource, mDefaultCameraIndexBindingData);
    mDefaultClipPlaneBG.create(resource, mDefaultClipPlaneBGData);
    mDefaultVisibleBuffer.create(resource, mDefaultVisibleBGData);

    mSkybox = new SkyBox{this, RESOURCE_DIR "/skybox"};
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    // initialize The instancing buffer
    mLightManager = LightManager::init(this);
    mInstanceManager = new InstanceManager{mRendererResource, sizeof(InstanceData) * 100000 * 10, 100000};

    mUniformBuffer.setLabel("MVP matrices matrix")
        .setSize(sizeof(CameraInfo) * 10)
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(mRendererResource);

    mUniforms.time = 1.0f;
    mUniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
    mUniformBuffer.queueWrite(0, &mUniforms, sizeof(CameraInfo));
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, mUniformBuffer.getBuffer(), 0, &mUniforms,
    //                      sizeof(CameraInfo) * 1);

    mDirectionalLightBuffer.setLabel("Directional light buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(LightingUniforms))
        .setMappedAtCraetion()
        .create(mRendererResource);

    mLightSpaceTransformation.setLabel("Light space transform buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Scene) * 5)
        .setMappedAtCraetion()
        .create(mRendererResource);

    mLightingUniforms.directions = {glm::vec4{-0.7, 1.0, 1.0, 1.0}, glm::vec4{0.2, 0.4, 0.3, 1.0}};
    mLightingUniforms.colors = {glm::vec4{0.99, 1.0, 0.88, 1.0}, glm::vec4{0.6, 0.9, 1.0, 1.0}};
    mDirectionalLightBuffer.queueWrite(0, &mLightingUniforms, sizeof(LightingUniforms));
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, mDirectionalLightBuffer.getBuffer(), 0,
    // &mLightingUniforms,
    //                      sizeof(LightingUniforms));

    mLightBuffer.setLabel("Lights Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Light) * 10)
        .setMappedAtCraetion(false)
        .create(mRendererResource);

    mLightManager->uploadToGpu(this, mLightBuffer.getBuffer());

    setupComputePass(this, mInstanceManager->getInstancingBuffer().getBuffer());

    mDefaultBoneFinalTransformData.setLabel("default bone final transform")
        .setSize(100 * sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
        .setMappedAtCraetion(false)
        .create(mRendererResource);

    static std::vector<glm::mat4> bones;
    bones.reserve(100);
    for (int i = 0; i < 100; i++) {
        bones.emplace_back(glm::mat4{1.0});
    }
    mDefaultBoneFinalTransformData.queueWrite(0, bones.data(), sizeof(glm::mat4) * bones.size());
    // wgpuQueueWriteBuffer(getRendererResource().queue, mDefaultBoneFinalTransformData.getBuffer(), 0, bones.data(),
    //                      sizeof(glm::mat4) * bones.size());

    mDefaultMeshGlobalTransformData.setLabel("default global mesh transform")
        .setSize(20 * sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
        .setMappedAtCraetion(false)
        .create(mRendererResource);
}

void Application::onResize() {
    size_t width = mWindow->mWindowSize.x;
    size_t height = mWindow->mWindowSize.y;
    setWindowSize(width, height);

    // Terminate in reverse order
    terminateDepthBuffer();
    terminateSwapChain();

    // imgui

    // Re-init
    initSwapChain(getRendererResource(), width, height);
    initDepthBuffer();

    mOutlinePass->mTextureView = mDepthTexture->createViewDepthOnly();
    mOutlinePass->createSomeBinding();

    mCamera.update(mUniforms, width, height);
}

void onWindowResize(GLFWwindow* window, int width, int height) {
    // We know that even though from GLFW's point of view this is
    // "just a pointer", in our case it is always a pointer to an
    // instance of the class `Application`
    auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    that->setWindowSize(width, height);

    // Call the actual class-member callback
    if (that != nullptr) that->onResize();
}

// WGPUBuffer resolveBuffer;
// Create staging buffer for CPU read (WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst)
// WGPUBuffer stagingBuffer;

static float last_frame_time = 0.0;
bool Application::initialize(const char* windowName, uint16_t width, uint16_t height) {
    // Creating the window
    mWindow = new Window<GLFWwindow>();
    // auto [window_width, window_height] = getWindowSize();
    mWindow->mName = windowName;
    mWindow->mWindowSize = {width, height};
    mWindow->mResizeCallback = [&]() {
        this->setWindowSize(mWindow->mWindowSize.x, mWindow->mWindowSize.y);
        this->onResize();
    };
    auto res = mWindow->create({width, height});

    if (!res.has_value()) {
        std::cout << "Faield to create windows\n";
        return false;
    }
    // GLFWwindow* provided_window = glfwCreateWindow(window_width, window_height, "World Explorer", nullptr, nullptr);
    GLFWwindow* provided_window = res.value();

    mEditor = new Editor{};

    // Set up Callbacks
    // TODO refactor these into their own files
    glfwSetWindowUserPointer(mWindow->getWindow(), this);  // set user pointer to be used in the callback function
    glfwSetFramebufferSizeCallback(mWindow->getWindow(), onWindowResize);
    glfwSetCursorPosCallback(mWindow->getWindow(), InputManager::handleMouseMove);
    glfwSetMouseButtonCallback(mWindow->getWindow(), InputManager::handleButton);
    glfwSetScrollCallback(mWindow->getWindow(), InputManager::handleScroll);
    glfwSetKeyCallback(mWindow->getWindow(), InputManager::handleKeyboard);

    auto& input_manager = InputManager::instance();
    input_manager.mMouseMoveListeners.push_back(&mEditor->gizmo);
    input_manager.mMouseButtonListeners.push_back(&mEditor->gizmo);

    Screen::instance().initialize(this);

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
    WGPUSurface provided_surface{};
    std::cout << glfwGetPlatform() << std::endl;
    provided_surface = glfwCreateWindowWGPUSurface(instance, provided_window);

    setWindowSize(mWindow->mWindowSize.x, mWindow->mWindowSize.y);

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

    mRendererResource = new RendererResource{};

    // initialize RenderResources
    this->getRendererResource().device = render_device;
    this->getRendererResource().queue = render_queue;
    this->getRendererResource().surface = provided_surface;
    this->getRendererResource().window = provided_window;

    mTextureRegistery = new Registery<std::string, Texture>{};
    mTextureRegistery->mLoader.device = render_device;

    // Configuring the surface
    WGPUSurfaceConfiguration surface_configuration = {};
    surface_configuration.nextInChain = nullptr;
    // Configure the texture created for swap chain
    auto [w_width, w_height] = getWindowSize();
    surface_configuration.width = w_width;
    surface_configuration.height = w_height;
    surface_configuration.usage = WGPUTextureUsage_RenderAttachment;

    WGPUSurfaceCapabilities capabilities = {};
    std::cout << std::format("Successfuly initialized GLFW and the surface is {:p}",
                             (void*)this->getRendererResource().surface)
              << std::endl;
    wgpuSurfaceGetCapabilities(this->getRendererResource().surface, adapter, &capabilities);
    surface_configuration.format = capabilities.formats[0];
    std::cout << "Surface texture format is : " << surface_configuration.format << std::endl;
    mSurfaceFormat = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);

    surface_configuration.viewFormatCount = 0;
    surface_configuration.viewFormats = nullptr;
    surface_configuration.device = this->getRendererResource().device;
    surface_configuration.presentMode = WGPUPresentMode_Fifo;
    surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(this->getRendererResource().surface, &surface_configuration);
    wgpuAdapterRelease(adapter);

    initGui();

    initializeBuffers();
    initializePipeline();

    physics::prepareJolt();

    mWorld = new World{this};
    // mWorld->actor = mEditor->mEditorActive ? nullptr : mWorld->actor;

    // WGPUBufferDescriptor resolveBufferDesc{};
    // resolveBufferDesc.label = {"timing buffer", WGPU_STRLEN};
    // resolveBufferDesc.size = 4 * sizeof(uint64_t);  // Matches querySet.count
    // resolveBufferDesc.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
    // resolveBuffer = wgpuDeviceCreateBuffer(getRendererResource().device, &resolveBufferDesc);
    //
    // // Create staging buffer for CPU read (WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst)
    // WGPUBufferDescriptor stagingBufferDesc{};
    // stagingBufferDesc.label = {"timing buffer2", WGPU_STRLEN};
    // stagingBufferDesc.size = resolveBufferDesc.size;
    // stagingBufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    // stagingBuffer = wgpuDeviceCreateBuffer(getRendererResource().device, &stagingBufferDesc);

    mLineEngine = new LineEngine{};
    mLineEngine->initialize(this);
    // create and hide debug lines
    debuglinegroup = mLineEngine->create(generateBox(), glm::mat4{0.0}, {0.2, 0.0, 8.0}).updateVisibility(false);

    spheredebuglines =
        mLineEngine->create(generateSphere(), glm::mat4{0.0}, glm::vec3{0.5, 0.5, 0.0}).updateVisibility(false);

    aabbDebugLines = mLineEngine->create(generateBox(), glm::mat4{1.0}, {0.8, 0.5, 0.0}).updateVisibility(false);

    debugbox.create(mLineEngine, glm::mat4{0.0}, glm::vec3{1.0});

    mCamera = Camera{{-0.0f, -0.0f, 0.0f}, glm::vec3{0.8f}, {1.0, 0.0, 0.0}, 0.0};
    mUniforms.setCamera(mCamera);

    mWorld->loadWorld();
    last_frame_time = glfwGetTime();

    //
    //
    //

    NFD_Init();

    static nfdu8filteritem_t filters[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};
    args.filterList = filters;
    args.filterCount = 2;

    //
    //
    //

    return true;
}

static bool first_time = true;

void Application::mainLoop() {
    double time = glfwGetTime();
    glfwPollEvents();

    ZoneScopedNC("Main Render Loop", 0x0000FF);

    {
        ZoneScopedNC("polling events", 0xF0F0F0);
        glfwPollEvents();
    }

    double delta_time = time - last_frame_time;
    last_frame_time = time;
    mWorld->delta = delta_time;

    {
        // PerfTimer timer{"loop timer"};
        ZoneScopedNC("next surface", 0xFFFA00);
        mCurrentTargetView = getNextSurfaceTextureView(getRendererResource());
        if (mCurrentTargetView == nullptr) {
            return;
        }
    }

    if (runPhysics) {
        physics::JoltLoop(delta_time);
    } else {
        for (const auto& cube : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
            // if (cube->mName == "cube" || cube->mName == "smallcube" || cube->mName == "cube2") {
            // auto rot = glm::normalize(cube->mTransform.mOrientation);
            // if (flip_x) {
            //     rot.x *= -1;
            // }
            // if (flip_y) {
            //     rot.y *= -1;
            // }
            // if (flip_z) {
            //     rot.z *= -1;
            // }
            // rot = glm::normalize(rot);
            // physics::setRotation(cube->mPhysicComponent->bodyId, rot);
            if (cube->mPhysicComponent != nullptr) {
                physics::setRotation(cube->mPhysicComponent->bodyId, glm::normalize(cube->mTransform.mOrientation));
                physics::setPosition(cube->mPhysicComponent->bodyId, cube->mTransform.getPosition());
            }
            // }
        }
    }

    if (mSelectedModel != nullptr) {
        auto [min, max] = mSelectedModel->getWorldSpaceAABB();
        aabbDebugLines.updateLines(generateAABBLines(min, max));
    }

    if (show_physic_objects) {
        for (auto& collider : physics::PhysicSystem::mColliders) {
            auto t = collider.getTransformation();
            collider.getDebugLines().updateTransformation(t);
        }
        if (selectedPhysicModel != nullptr) {
            auto [pos, rot] = physics::getPositionAndRotationyId(selectedPhysicModel->mPhysicComponent->bodyId);
            debuglinegroup
                .updateTransformation(
                    glm::translate(glm::mat4{1.0}, pos) * glm::toMat4(rot) *
                    glm::scale(glm::mat4{1.0}, selectedPhysicModel->mTransform.getScale() * glm::vec3{2.0}))
                .updateVisibility(true);
        }
    }
    // if (mWorld->actor && mWorld->actor->mBehaviour) {
    for (auto& m : mWorld->rootContainer) {
        // auto* m = mWorld->actor->mBehaviour->getWeapon();
        // auto* m = model;
        if (m != nullptr && m->mSocket != nullptr) {
            // Model* base = m;
            Model* base = m->mSocket->model;

            glm::mat4 offsetMat = glm::mat4(1.0f);

            offsetMat = glm::translate(offsetMat, m->mSocket->positionOffset);
            offsetMat *= glm::mat4_cast(m->mSocket->rotationOffset);
            offsetMat = glm::scale(offsetMat, m->mSocket->scaleOffset);

            auto socket_global = glm::mat4{1.0f};

            if (m->mSocket->type == AnchorType::Bone) {
                if (base->anim->activeAction->calculatedTransform.contains(m->mSocket->anchorName)) {
                    auto trans = base->anim->activeAction->calculatedTransform[m->mSocket->anchorName];

                    auto [t, s, r] = decomposeTransformation(base->mTransform.mTransformMatrix * trans);
                    // socket_global = base->mTransform.mTransformMatrix * trans;
                    auto final_trans = glm::translate(glm::mat4{1.0}, t);
                    auto final_scale = glm::scale(glm::mat4{1.0}, s);
                    auto final_rot = glm::mat4_cast(r);

                    socket_global = final_trans * final_rot * final_scale;
                }
            } else if (m->mSocket->type == AnchorType::Model) {
                auto trans = base->mTransform.mTransformMatrix;
                auto [t, s, r] = decomposeTransformation(trans);
                // t = (t) + (r * m->mSocket->positionOffset);
                // auto final_trans = glm::translate(glm::mat4{1.0}, t);
                // auto final_scale = glm::scale(glm::mat4{1.0}, s);
                // r = glm::normalize(r * m->mSocket->rotationOffset);
                // s = s * m->mSocket->scaleOffset;
                // auto final_rot = glm::mat4_cast(r);
                // socket_global = final_trans * final_rot * final_scale;
                // // socket_global = trans;  // final_trans * final_rot * final_scale;
                // m->moveTo(t);
                // m->scale(s);
                // m->rotate(normalize(r));

                auto final_trans = glm::translate(glm::mat4{1.0}, t);
                auto final_scale = glm::scale(glm::mat4{1.0}, s);
                auto final_rot = glm::mat4_cast(r);

                socket_global = final_trans * final_rot * final_scale;
            }
            auto new_trans = socket_global * offsetMat;

            auto [t, s, r] = decomposeTransformation(new_trans);

            m->moveTo(t);
            m->scale(s);
            m->rotate(normalize(r));
        }
    }
    // }

    {
        // PerfTimer timer{"tick"};
        for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
            reinterpret_cast<BaseModel*>(model)->updateHirarchy();
            model->update(this, delta_time, runPhysics);
        }
        for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_Editor)) {
            model->update(this, delta_time, runPhysics);
        }
        auto* actor = mWorld->actor;
        if (!mEditor->mEditorActive && actor != nullptr && actor->mBehaviour != nullptr) {
            actor->mBehaviour->handleAttachedCamera(actor, &mCamera);
            actor->mBehaviour->update(actor, delta_time);
        }
    }

    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = createStringViewC("command encoder descriptor");
    WGPUCommandEncoder encoder =
        wgpuDeviceCreateCommandEncoder(this->getRendererResource().device, &encoder_descriptor);
    this->getRendererResource().commandEncoder = encoder;

    auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());

    // Dispaching Compute shaders to cull everything that is outside the frustum
    // -------------------------------------------------------------------------
    auto fp = create2FrustumPlanes(corners);
    getFrustumPlaneBuffer().queueWrite(0, fp.data(), sizeof(FrustumPlanesUniform));
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, getFrustumPlaneBuffer().getBuffer(), 0, fp.data(),
    //                      sizeof(FrustumPlanesUniform));

    if (cull_frustum) {
        // runFrustumCullingTask(this, encoder);
    }

    // -------------------------------------------------------------------------

    // ---------------- 1 - Preparing for shadow pass ---------------
    // The first pass is the shadow pass, only based on the opaque objects
    if (should_update_csm) {
        auto all_scenes = mShadowPass->createFrustumSplits(
            corners, {{0.0, middle_plane_length},
                      {middle_plane_length, middle_plane_length + far_plane_length},
                      {middle_plane_length + far_plane_length, middle_plane_length + far_plane_length + 10}});
        // });

        uint32_t cascades_count = all_scenes.size();
        mTimeBuffer.queueWrite(0, &cascades_count, sizeof(uint32_t));
        // wgpuQueueWriteBuffer(this->getRendererResource().queue, mTimeBuffer.getBuffer(), 0, &cascades_count,
        //                      sizeof(uint32_t));
        mLightSpaceTransformation.queueWrite(0, all_scenes.data(), sizeof(Scene) * all_scenes.size());
        // wgpuQueueWriteBuffer(this->getRendererResource().queue, mLightSpaceTransformation.getBuffer(), 0,
        //                      all_scenes.data(), sizeof(Scene) * all_scenes.size());
    }

    {
        ZoneScoped;
        mShadowPass->lightPos = mLightingUniforms.directions[0];
        mShadowPass->renderAllCascades(encoder);
    }
    //
    //-------------- End of shadow pass
    mUniforms.setCamera(mCamera);
    mUniformBuffer.queueWrite(0, &mUniforms, sizeof(CameraInfo));
    // wgpuQueueWriteBuffer(this->getRendererResource().queue, mUniformBuffer.getBuffer(), 0, &mUniforms,
    //                      sizeof(CameraInfo));

    // mWaterRenderPass->drawWater();
    for (const auto& model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
        if (model->mName == "human") {
        }
    }

    // Depth pre-pass to reduce number of overdraws
    {
        WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &mDepthPrePass->mDesc);
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mDepthPrePass->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mBindingGroup.getBindGroup(), 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                          nullptr);
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 4, mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 5, mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

        {
            // PerfTimer timer{"test"};
            wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mDepthPrePass->getPipeline()->getPipeline());
            for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
                // model->drawHirarchy(this, render_pass_encoder);
            }
        }

        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }
    // ----------------------------------------------

    // ---- Skybox and PBR render pass
    // WGPUQuerySetDescriptor querySetDesc{};
    // querySetDesc.type = WGPUQueryType_Timestamp;
    // querySetDesc.count = 4;  // e.g., 2 passes * 2 timestamps each
    // WGPUQuerySet querySet = wgpuDeviceCreateQuerySet(getRendererResource().device, &querySetDesc);

    WGPURenderPassDescriptor render_pass_descriptor = {};

    {
        render_pass_descriptor.nextInChain = nullptr;

        static WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view = mCurrentTargetView;
        color_attachment.resolveTarget = nullptr;
        color_attachment.loadOp = WGPULoadOp_Clear;
        color_attachment.storeOp = WGPUStoreOp_Store;
        color_attachment.clearValue = WGPUColor{0.52, 0.80, 0.92, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
        color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

        render_pass_descriptor.colorAttachmentCount = 1;
        render_pass_descriptor.colorAttachments = &color_attachment;

        static WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
        depth_stencil_attachment.view = mDepthTextureView;
        depth_stencil_attachment.depthClearValue = 1.0f;
        depth_stencil_attachment.depthLoadOp = WGPULoadOp_Load;
        depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.depthReadOnly = false;
        depth_stencil_attachment.stencilClearValue = 0;
        depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Load;
        depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.stencilReadOnly = false;
        render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
        render_pass_descriptor.timestampWrites = nullptr;

        // Timestamp writes: querySet, starting index, count (2: begin + end)
        // WGPURenderPassTimestampWrites timestampWrites[2];
        // timestampWrites[0].querySet = querySet;
        // timestampWrites[0].beginningOfPassWriteIndex = 0;  // Start timestamp index
        // timestampWrites[0].endOfPassWriteIndex = 1;        // End timestamp index
        // render_pass_descriptor.timestampWrites = timestampWrites;  // Array of writes
    }

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(mUniforms.viewMatrix));
    glm::mat4 mvp = mUniforms.projectMatrix * viewNoTranslation;
    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mSkybox->getPipeline()->getPipeline());
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mSkybox->mBindingGroup.getBindGroup(), 0, nullptr);
    mSkybox->draw(this, render_pass_encoder, mvp);
    int32_t stencilReferenceValue = 240;
    wgpuRenderPassEncoderSetStencilReference(render_pass_encoder, stencilReferenceValue);

    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 4, mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 5, mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

    {
        // {
        ZoneScopedNC("Color Pass", 0xFF);
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline->getPipeline());
        for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
            // if (!model->isTransparent()) {
            // if (model->getName() != "ghamgham" && model->getName() != "human") {
            // model->draw(this, render_pass_encoder);
            // } else {
            model->drawHirarchy(this, render_pass_encoder);
            // }
            // }
        }
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);

    // wgpuQuerySetRelease(querySet);
    // ---------------------------------------------------------------------
    mLineEngine->executePass();
    // ---------------------------------------------------------------------
    // mWaterRenderPass->waterBlend();
    // ---------------------------------------------------------------------
    // mTerrainPass->executePass();
    // ---------------------------------------------------------------------

    {
        WGPURenderPassDescriptor render_pass_descriptor = {};
        render_pass_descriptor.nextInChain = nullptr;

        static WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view = mCurrentTargetView;
        color_attachment.resolveTarget = nullptr;
        color_attachment.loadOp = WGPULoadOp_Load;
        color_attachment.storeOp = WGPUStoreOp_Store;
        color_attachment.clearValue = WGPUColor{0.52, 0.80, 0.92, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
        color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif  // NOT WEBGPU_BACKEND_WGPU

        render_pass_descriptor.colorAttachmentCount = 1;
        render_pass_descriptor.colorAttachments = &color_attachment;

        static WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
        depth_stencil_attachment.view = mDepthTextureView;
        depth_stencil_attachment.depthClearValue = 1.0f;
        depth_stencil_attachment.depthLoadOp = WGPULoadOp_Load;
        depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.depthReadOnly = false;
        depth_stencil_attachment.stencilClearValue = 0;
        depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Load;
        depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.stencilReadOnly = false;
        render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
        render_pass_descriptor.timestampWrites = nullptr;

        WGPURenderPassEncoder terrain_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);
        wgpuRenderPassEncoderSetPipeline(terrain_pass_encoder, mTerrainPass->getPipeline()->getPipeline());

        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                          nullptr);

        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 4, mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 5, mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 6, mTerrainPass->mTexturesBindgroup.getBindGroup(), 0,
                                          nullptr);

        // if (mWorld->actor == nullptr) {
        if (mEditor->mEditorActive) {
            updateGui(terrain_pass_encoder, delta_time);
        }

        wgpuRenderPassEncoderEnd(terrain_pass_encoder);
        wgpuRenderPassEncoderRelease(terrain_pass_encoder);
    }

    // outline pass
    // mOutlinePass->setColorAttachment(
    //     {mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    // mOutlinePass->setDepthStencilAttachment({mDepthTextureView, StoreOp::Undefined, LoadOp::Undefined, false,
    //                                          StoreOp::Undefined, LoadOp::Undefined, true, 0.0});
    // mOutlinePass->init();
    //
    // WGPURenderPassEncoder outline_pass_encoder =
    //     wgpuCommandEncoderBeginRenderPass(encoder, mOutlinePass->getRenderPassDescriptor());
    // wgpuRenderPassEncoderSetStencilReference(outline_pass_encoder, stencilReferenceValue);
    //
    // wgpuRenderPassEncoderSetBindGroup(outline_pass_encoder, 3,
    // mOutlinePass->mDepthTextureBindgroup.getBindGroup(), 0,
    //                                   nullptr);
    // wgpuRenderPassEncoderSetBindGroup(outline_pass_encoder, 4, mDefaultCameraIndexBindgroup.getBindGroup(), 0,
    // nullptr);
    //
    // for (const auto& [name, model] : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User))
    // {
    //     if (model->isSelected()) {
    //         wgpuRenderPassEncoderSetPipeline(outline_pass_encoder, mOutlinePass->getPipeline()->getPipeline());
    //         model->draw(this, outline_pass_encoder, mBindingData);
    //     }
    // }
    //
    // wgpuRenderPassEncoderEnd(outline_pass_encoder);
    // wgpuRenderPassEncoderRelease(outline_pass_encoder);

    // if (mWorld->actor == nullptr) {
    if (mEditor->mEditorActive) {
        ZoneScopedNC("3D viewport and loader", 0xF0F00F);
        // 3D editor elements pass
        m3DviewportPass->execute(encoder);

        // polling if any model loading process is done and append it to loaded model list
        ModelRegistry::instance().tick(this);
    }

    // ------------ 3- Transparent pass
    // Calculate the Accumulation Buffer from the transparent object, this pass does not draw
    // on the render Target
    /*auto transparency_pass_desc = mTransparencyPass->getRenderPassDescriptor();*/
    /*mTransparencyPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;*/
    /*WGPURenderPassEncoder transparency_pass_encoder =*/
    /*    wgpuCommandEncoderBeginRenderPass(encoder, transparency_pass_desc);*/
    /*wgpuRenderPassEncoderSetPipeline(transparency_pass_encoder,
     * mTransparencyPass->getPipeline()->getPipeline());*/

    /*mShadowPass->setupScene({1.0f, 1.0f, 4.0f});*/
    /*mTransparencyPass->render(mLoadedModel, transparency_pass_encoder, mDepthTextureView);*/

    /*wgpuRenderPassEncoderEnd(transparency_pass_encoder);*/
    /*wgpuRenderPassEncoderRelease(transparency_pass_encoder);*/

    // ------------ 4- Composition pass
    // In this pass we will compose the result from opaque pass and the transparent pass
    /*auto ssbo_buffers = mTransparencyPass->getSSBOBuffers();*/
    /*mCompositionPass->setSSBOBuffers(ssbo_buffers.first, ssbo_buffers.second);*/
    /*mCompositionPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;*/
    /*mCompositionPass->mRenderPassColorAttachment.view = mCurrentTargetView;*/
    /*auto composition_pass_desc = mCompositionPass->getRenderPassDescriptor();*/
    /*WGPURenderPassEncoder composition_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder,
     * composition_pass_desc);*/
    /*wgpuRenderPassEncoderSetPipeline(composition_pass_encoder, mCompositionPass->getPipeline()->getPipeline());*/

    /*mCompositionPass->render(mLoadedModel, composition_pass_encoder, &render_pass_color_attachment);*/

    /*wgpuRenderPassEncoderEnd(composition_pass_encoder);*/
    /*wgpuRenderPassEncoderRelease(composition_pass_encoder);*/

    static WGPUCommandBufferDescriptor command_buffer_descriptor = {};
    command_buffer_descriptor.nextInChain = nullptr;
    command_buffer_descriptor.label = {"command buffer", WGPU_STRLEN};
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &command_buffer_descriptor);

    {
        // {
        ZoneScopedNC("terrain for refraction", 0x00F00F);
        // }
        // PerfTimer timer{"test"};
        wgpuDevicePoll(this->getRendererResource().device, true, nullptr);
    }
    wgpuQueueSubmit(this->getRendererResource().queue, 1, &command);
    // static WGPUQueueWorkDoneCallbackInfo cbinfo{};
    // cbinfo.nextInChain = nullptr;
    // cbinfo.callback = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
    //     Application* app = (Application*)userdata1;
    //     if (status == WGPUQueueWorkDoneStatus_Success) {
    //     }
    // };
    //
    // cbinfo.userdata1 = this;

    // wgpuQueueOnSubmittedWorkDone(this->getRendererResource().queue, cbinfo);
    wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(encoder);

    wgpuTextureViewRelease(mCurrentTargetView);

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(this->getRendererResource().surface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#endif
    // std::cout << "Perf timer shows " << std::endl;
}

void Application::terminate() {
    wgpuBufferRelease(mLightBuffer.getBuffer());
    wgpuBufferRelease(mUniformBuffer.getBuffer());
    terminateGui();
    wgpuRenderPipelineRelease(mPipeline->getPipeline());
    wgpuSurfaceUnconfigure(this->getRendererResource().surface);
    wgpuQueueRelease(this->getRendererResource().queue);
    wgpuSurfaceRelease(this->getRendererResource().surface);
    wgpuDeviceRelease(this->getRendererResource().device);
    glfwDestroyWindow(this->getRendererResource().window);
    glfwTerminate();
}

bool Application::isRunning() { return !glfwWindowShouldClose(this->getRendererResource().window); }

WGPUTextureView getNextSurfaceTextureView(RendererResource& resources) {
    WGPUSurfaceTexture surface_texture = {};
    wgpuSurfaceGetCurrentTexture(resources.surface, &surface_texture);
    if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
        surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        return nullptr;
    }

    WGPUTextureViewDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;
    descriptor.label = createStringViewC("surface texture view");
    descriptor.format = wgpuTextureGetFormat(surface_texture.texture);
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;
    descriptor.aspect = WGPUTextureAspect_All;
    return wgpuTextureCreateView(surface_texture.texture, &descriptor);
}

WGPULimits GetRequiredLimits(WGPUAdapter adapter) {
    WGPULimits supported_limits = {};
    supported_limits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supported_limits);

    WGPULimits required_limits = {};
    setDefault(required_limits);

    required_limits.maxVertexAttributes = 10;
    required_limits.maxVertexBuffers = 1;
    required_limits.maxBufferSize = 134217728;  // 1000000 * sizeof(VertexAttributes);
    required_limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    required_limits.maxSampledTexturesPerShaderStage = 8;
    // required_limits.maxInterStageShaderComponents = 116;

    // Binding groups
    required_limits.maxBindGroups = 7;
    required_limits.maxUniformBuffersPerShaderStage = 6;
    required_limits.maxUniformBufferBindingSize = 2048 * 4;  // 16 * 4 * sizeof(float);

    required_limits.maxTextureDimension1D = 4096;
    required_limits.maxTextureDimension2D = 4096;
    required_limits.maxTextureArrayLayers = 6;
    required_limits.maxSamplersPerShaderStage = 2;

    return required_limits;
}

bool initSwapChain(RendererResource& resources, uint32_t width, uint32_t height) {
    WGPUSurfaceConfiguration surface_configuration = {};
    // Configure the texture created for swap chain
    surface_configuration.nextInChain = nullptr;
    surface_configuration.width = width;
    surface_configuration.height = height;
    surface_configuration.usage = WGPUTextureUsage_RenderAttachment;
    surface_configuration.format = WGPUTextureFormat_BGRA8UnormSrgb;
    surface_configuration.viewFormatCount = 0;
    surface_configuration.viewFormats = nullptr;
    surface_configuration.device = resources.device;
    surface_configuration.presentMode = WGPUPresentMode_Fifo;
    surface_configuration.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(resources.surface, &surface_configuration);
    return resources.surface != nullptr;
}

void Application::terminateSwapChain() {
    // wgpuSurfaceRelease(this->getRendererResource().surface);
}

bool Application::initDepthBuffer() {
    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24PlusStencil8;
    mDepthTexture = new Texture{this->getRendererResource().device,
                                static_cast<uint32_t>(mWindow->mWindowSize.x),
                                static_cast<uint32_t>(mWindow->mWindowSize.y),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                depth_texture_format,
                                1,
                                "Standard depth texture"};

    // Create the view of the depth texture manipulated by the rasterizer
    mDepthTextureView = mDepthTexture->createViewDepthStencil();

    mDepthPrePass->getRenderDesc(mDepthTextureView);
    m3DviewportPass->initTargets();

    // 2. Create a WGPUTextureView for the DEPTH aspect only
    WGPUTextureViewDescriptor depthViewDesc = {};
    depthViewDesc.format = WGPUTextureFormat_Depth24Plus;  // Must match the base texture format
    depthViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthViewDesc.baseMipLevel = 0;
    depthViewDesc.mipLevelCount = 1;
    depthViewDesc.baseArrayLayer = 0;
    depthViewDesc.arrayLayerCount = 1;
    depthViewDesc.aspect = WGPUTextureAspect_DepthOnly;         // Specify Depth Only
    depthViewDesc.label = createStringView("Depth-Only View");  // Label for debugging

    mDepthTextureViewDepthOnly = wgpuTextureCreateView(mDepthTexture->getTexture(), &depthViewDesc);

    return mDepthTextureView != nullptr;
}

void Application::terminateDepthBuffer() {
    wgpuTextureViewRelease(mDepthTextureView);
    wgpuTextureDestroy(mDepthTexture->getTexture());
    delete mDepthTexture;
}

// called in onInit
bool Application::initGui() {
    static ImGui_ImplWGPU_InitInfo imgui_device{};
    imgui_device.Device = this->getRendererResource().device;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(this->getRendererResource().window, true);
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = this->getRendererResource().device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = WGPUTextureFormat_BGRA8UnormSrgb;
    init_info.DepthStencilFormat = WGPUTextureFormat_Depth24PlusStencil8;
    ImGui_ImplWGPU_Init(&init_info);
    return true;
}

void Application::terminateGui() {
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplWGPU_Shutdown();
}  // called in onFinish

void Application::updateGui(WGPURenderPassEncoder renderPass, double time) {
    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginTabBar("ObjectTabs")) {
        if (ImGui::BeginTabItem("Scene")) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", time * 1000.0f, 1.0 / time);

            ImGui::Checkbox("flip x", &flip_x);
            ImGui::Checkbox("flip y", &flip_y);
            ImGui::Checkbox("flip z", &flip_z);

            if (ImGui::CollapsingHeader("Cameras", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::DragFloat("z-near", &mCamera.mZnear, 1.0, 0.0, 180.0f) ||
                    ImGui::DragFloat("z-far", &mCamera.mZfar, 1.0, 0.0, 1000.0f) ||
                    ImGui::DragFloat("FOV", &mCamera.mFov, 0.1, 0.0, 1000.0f)) {
                    mCamera.update(mUniforms, mWindow->mWindowSize.x, mWindow->mWindowSize.y);
                }

                if (ImGui::Button("Reset to default Params")) {
                    mCamera.mFov = 60.0f;
                    mCamera.mZnear = 0.01f;
                    mCamera.mZfar = 200.0f;
                    mCamera.update(mUniforms, mWindow->mWindowSize.x, mWindow->mWindowSize.y);
                }

                ImGui::Checkbox("cull frustum", &cull_frustum);
            }

            // static glm::vec3 pointlightshadow = glm::vec3{5.6f, -2.1f, 6.0f};
            if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat("frustum split factor", &middle_plane_length, 1.0, 100);
                ImGui::SliderFloat("far split factor", &far_plane_length, 1.0, 200);
                ImGui::SliderFloat("visualizer", &mUniforms.time, -1.0, 1.0);
            }
            ImGui::Checkbox("shoud update csm", &should_update_csm);
            ImGui::Text("\nClip Plane");

            if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen)) {
                static bool open_popup = false;
                static nfdu8char_t* output;
                static const nfdpathset_t* paths;
                if (ImGui::Button("Open file")) {
                    nfdresult_t result = NFD_OpenDialogMultipleU8(&paths, nullptr, 0, nullptr);
                    if (result == NFD_OKAY) {
                        puts("Success!");
                        open_popup = true;

                    } else if (result == NFD_CANCEL) {
                        puts("User pressed cancel.");
                    } else {
                        printf("Error: %s\n", NFD_GetError());
                    }
                }
                if (open_popup) {
                    ImGui::OpenPopup("Import Settings");

                    if (ImGui::BeginPopupModal("Import Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::TextWrapped("Importing:\n%s", output);
                        ImGui::Separator();

                        static bool isAnimated = true;
                        static int coordinate_system = 0;
                        static CoordinateSystem cs = Z_UP;

                        ImGui::Checkbox("Is Animated", &isAnimated);
                        if (ImGui::Combo("Coordinate System", &coordinate_system, "Z UP\0Y UP\0")) {
                            switch (coordinate_system) {
                                case 0:
                                    cs = Z_UP;
                                    break;
                                case 1:
                                    cs = Y_UP;
                                    break;
                                default:
                                    break;
                            }
                        }

                        ImGui::Separator();
                        if (ImGui::Button("Import")) {
                            nfdpathsetsize_t listcount = 0;
                            NFD_PathSet_GetCount(paths, &listcount);
                            for (size_t i = 0; i < listcount; ++i) {
                                NFD_PathSet_GetPath(paths, i, &output);
                                std::cout << output << std::endl;
                                // ObjectLoaderParam param{name,  path,   is_animated, cs,           translate,
                                //                         scale, rotate, childs,      default_clip, socket_param};

                                mWorld->loadModel(ObjectLoaderParam{"ahmad",
                                                                    output,
                                                                    isAnimated,
                                                                    cs,
                                                                    std::array<float, 3>{0.0, 0.0, 0.0},
                                                                    std::array<float, 3>{1.0, 1.0, 1.0},
                                                                    std::array<float, 3>{1.0, 1.0, 1.0},
                                                                    {},
                                                                    "",
                                                                    {}});
                            }

                            ImGui::CloseCurrentPopup();
                            open_popup = false;  // openImportPopup = false;
                            NFD_PathSet_Free(paths);
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Cancel")) {
                            ImGui::CloseCurrentPopup();
                            open_popup = false;
                            NFD_PathSet_Free(paths);
                        }

                        ImGui::EndPopup();
                    }
                }
            }
            if (ImGui::DragFloat4("clip plane:", glm::value_ptr(mDefaultPlane))) {
                mDefaultClipPlaneBuf.queueWrite(0, glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));
                // wgpuQueueWriteBuffer(this->getRendererResource().queue, mDefaultClipPlaneBuf.getBuffer(), 0,
                //                      glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Objects")) {
            ImGui::BeginChild("ScrollableList", ImVec2(0, 200),
                              ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

            for (const auto& item :
                 ModelRegistry::instance().getLoadedModel(Visibility_User) /*mWorld->rootContainer*/) {
                // Create a unique ID for each selectable item based on its unique item.id
                ImGui::PushID((void*)item);

                if (ImGui::Selectable(item->getName().c_str(),
                                      mSelectedModel && item->getName() == mSelectedModel->getName())) {
                    ImGui::Text("%s", item->getName().c_str());

                    if (mSelectedModel) {
                        mSelectedModel->selected(false);
                    }
                    mSelectedModel = item;
                    mSelectedModel->selected(true);

                    if (item->mName == "pistol") {
                        auto [pos, scale, rot] = decomposeTransformation(mSelectedModel->getGlobalTransform());
                        std::cout << "\n-------------------" << mSelectedModel->getName()
                                  << "----------------------------\n";
                        std::cout << "pos:" << glm::to_string(pos) << '\n';
                        std::cout << "scale:" << glm::to_string(scale) << '\n';
                        std::cout << "rot:" << glm::to_string(rot);
                        std::cout << "\n------------------------------------------------\n";
                    }
                }

                ImGui::PopID();  // Pop the unique ID for this item
            }
            ImGui::EndChild();  // End the scrollable list

            if (mSelectedModel) {
                ImGui::Text("Model: %s, Meshes: %zu", mSelectedModel->getName().c_str(),
                            mSelectedModel->mFlattenMeshes.size());

#ifdef DEVELOPMENT_BUILD
                mSelectedModel->userInterface();
#endif  // DEVELOPMENT_BUILD
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lights")) {
            mShadowPass->lightPos = mLightingUniforms.directions[0];

            ImGui::Text("Directional Light");
            if (ImGui::ColorEdit3("Color", glm::value_ptr(mLightingUniforms.colors[0])) ||
                ImGui::DragFloat3("Direction", glm::value_ptr(mLightingUniforms.directions[0]), 0.1, -1.0, 1.0)) {
                mDirectionalLightBuffer.queueWrite(0, &mLightingUniforms, sizeof(LightingUniforms));
                // wgpuQueueWriteBuffer(this->getRendererResource().queue, mDirectionalLightBuffer.getBuffer(), 0,
                //                      &mLightingUniforms, sizeof(LightingUniforms));
            }

            ImGui::Text("    ");
            ImGui::Text("Environment Lights");

            mLightManager->renderGUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("socket")) {
            static BaseModel* target_socket = nullptr;

            for (const auto& item : mWorld->rootContainer) {
                if (item->mSocket != nullptr) {
                    ImGui::PushID((void*)item);
                    // Create a unique ID for each selectable item based on its unique item.id
                    bool modified = false;
                    static glm::vec3 euler_rot{0.0f};
                    if (ImGui::Selectable(
                            std::format("{} to {} Bone of: {} {}", item->getName().c_str(),
                                        item->mSocket->anchorName.c_str(), item->mSocket->model->getName().c_str(),
                                        static_cast<int>(item->mSocket->type))
                                .c_str(),
                            target_socket == item)) {
                    }
                    if (ImGui::BeginPopupContextItem()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.18f, 0.22f, 1.0f));
                        if (ImGui::MenuItem("Modify")) {
                            target_socket = item;
                            euler_rot = glm::eulerAngles(item->mSocket->rotationOffset);
                        };
                        ImGui::PopStyleColor();
                        ImGui::EndPopup();
                    }

                    if (target_socket == item) {
                        float speed = ImGui::IsKeyPressed(ImGuiKey_LeftShift) ? 0.001 : 0.5;
                        ImGui::DragFloat3("Pos offset", glm::value_ptr(item->mSocket->positionOffset), speed);
                        modified |= ImGui::DragFloat3("Rotation offset", glm::value_ptr(euler_rot),
                                                      ImGui::IsKeyPressed(ImGuiKey_LeftShift) ? 0.1 : 5.0);
                        ImGui::DragFloat3("scale offset", glm::value_ptr(item->mSocket->scaleOffset), speed);
                        ImGui::NewLine();
                        if (modified) {
                            item->mSocket->rotationOffset = glm::quat(glm::vec3{
                                glm::radians(euler_rot.x), glm::radians(euler_rot.y), glm::radians(euler_rot.z)});
                        }

                        switch (static_cast<int>(item->mSocket->type)) {
                            case 0:
                                item->mSocket->anchorName = "";
                                break;

                            case 1:
                                break;

                            case 2: {
                                auto* base_model = reinterpret_cast<Model*>(item->mSocket->model);

                                auto& selected_name = item->mSocket->anchorName;
                                if (ImGui::BeginCombo("Bone", selected_name.c_str())) {
                                    for (auto& [key, value] : base_model->anim->activeAction->Bonemap) {
                                        bool selected = (key == selected_name);
                                        if (ImGui::Selectable(key.c_str(), selected)) {
                                            selected_name = key;
                                        }

                                        if (selected) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }

                                break;
                            }
                        }
                    }

                    ImGui::PopID();  // Pop the unique ID for this item
                }
            }
            ImGui::Separator();

            static BaseModel* child = nullptr;
            static BaseModel* base = nullptr;
            if (mSelectedModel != nullptr) {
                ImGui::TextColored(ImVec4{1.0, 0.0, 0.0, 1.0}, "%s is selected as child.",
                                   mSelectedModel->getName().c_str());
                ImGui::SameLine();
                if (ImGui::Button("Ok##chid")) {
                    child = mSelectedModel;
                }
                static int current_item = 0;
                static std::string anchor_name = "";

                if (child != nullptr) {
                    ImGui::Text("Select %s as base.", mSelectedModel->getName().c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("Ok##base")) {
                        base = mSelectedModel;
                    }

                    if (child != nullptr && base != nullptr) {
                        const char* items[] = {"Model", "Mesh", "Bone"};

                        if (ImGui::Combo("Type", &current_item, items, IM_ARRAYSIZE(items))) {
                            // This runs when selection changes
                        }
                        ImGui::Text("child=%s and base=%s for Bone: %s", child->getName().c_str(),
                                    base->getName().c_str(), anchor_name.c_str());
                        switch (current_item) {
                            case 0:
                                anchor_name = "";
                                break;

                            case 1:
                                break;

                            case 2: {
                                auto* base_model = reinterpret_cast<Model*>(base);
                                if (base_model->anim && base_model->anim->activeAction) {
                                    static std::string selectedKey;

                                    if (ImGui::BeginCombo("Bone", selectedKey.c_str())) {
                                        for (auto& [key, value] : base_model->anim->activeAction->Bonemap) {
                                            bool selected = (key == selectedKey);
                                            if (ImGui::Selectable(key.c_str(), selected)) {
                                                anchor_name = key;
                                            }
                                            selectedKey = key;

                                            if (selected) ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                                }
                                break;
                            }
                        }
                    }

                    if (ImGui::Button("Create Socket")) {
                        // auto diff_pos = base->mTransform.getPosition() - child->mTransform.getPosition();
                        child->mSocket = new BoneSocket{reinterpret_cast<Model*>(mSelectedModel),
                                                        anchor_name,
                                                        glm::vec3{0.0},
                                                        glm::vec3{0.02},
                                                        glm::vec3{0.0},
                                                        static_cast<AnchorType>(current_item)};
                        child = nullptr;
                        base = nullptr;
                    }
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Physics")) {
            ImGui::Checkbox("Run Physics", &runPhysics);

            ImGui::NewLine();
            ImGui::Separator();
            if (ImGui::DragFloat3("Center", glm::value_ptr(debugbox.center), 0.01) ||
                ImGui::DragFloat3("Half extent", glm::value_ptr(debugbox.halfExtent), 0.01)) {
                if (debugbox.debuglines != nullptr && debugbox.debuglines->isInitialized()) {
                    debugbox.update();
                    mEditor->mSelectedObject = &debugbox;
                    mEditor->gizmo.moveTo(debugbox.center);
                }
            }
            static bool is_static = false;
            static bool is_sensor = false;
            ImGui::Checkbox("is Dynamic?", &is_static);
            ImGui::Checkbox("is Sensor?", &is_sensor);
            if (ImGui::Button("Create box")) {
                physics::PhysicSystem::createCollider(this, "new collider", debugbox.center, debugbox.halfExtent,
                                                      is_static, is_sensor, nullptr);
            }

            {
                static glm::vec3 scale{0.0};
                static glm::vec3 translation{0.0};
                if (ImGui::DragFloat3("sphere center", glm::value_ptr(translation), 0.01) ||
                    ImGui::DragFloat3("sphere scale ", glm::value_ptr(scale), 0.01)) {
                    spheredebuglines.updateTransformation(glm::translate(glm::mat4{1.0}, translation) *
                                                          glm::scale(glm::mat4{1.0}, scale));
                }
            }
            if (ImGui::Button("Create sphere")) {
                spheredebuglines.updateVisibility(true);
            }

            ImGui::NewLine();
            ImGui::Separator();
            if (ImGui::Checkbox("show physics objects", &show_physic_objects)) {
                if (selectedPhysicModel != nullptr) {
                    debuglinegroup.updateVisibility(show_physic_objects);
                }
            }

            if (ImGui::BeginCombo("Physic objects", selectedPhysicModel == nullptr
                                                        ? "Choose one##1"
                                                        : selectedPhysicModel->getName().c_str())) {
                for (const auto& item :
                     ModelRegistry::instance().getLoadedModel(Visibility_User) /*mWorld->rootContainer*/) {
                    if (item->mPhysicComponent != nullptr) {
                        ImGui::PushID((void*)item);
                        // Create a unique ID for each selectable item based on its unique item.id
                        if (ImGui::Button(item->getName().c_str())) {
                            selectedPhysicModel = item;
                        }
                        ImGui::PopID();  // Pop the unique ID for this item
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::NewLine();
            ImGui::Separator();
            if (ImGui::Checkbox("Show Colliders Debug lines", &show_physic_debugs)) {
                for (auto& collider : physics::PhysicSystem::mColliders) {
                    collider.getDebugLines().updateVisibility(show_physic_debugs);
                }
            }
            static LineGroup* selectedGroup = nullptr;
            if (ImGui::BeginCombo("Colliders", "Choose one##2")) {
                for (auto& collider : physics::PhysicSystem::mColliders) {
                    ImGui::PushID((void*)&collider);

                    if (ImGui::Button(
                            std::format("{} #{}", collider.mName.c_str(), collider.getDebugLines().mId).c_str())) {
                        if (selectedGroup != nullptr) {
                            selectedGroup->updateColor({1.0, 0.0, 0.0});
                        }
                        selectedGroup = &collider.getDebugLines();
                        selectedGroup->updateColor({0.1, 0.0, 0.9});
                    }

                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Remove")) {
                            collider.getDebugLines().remove();
                            physics::PhysicSystem::removeCollider(collider);
                        };
                        if (ImGui::MenuItem("Modify")) {
                            auto cld = collider;
                            collider.getDebugLines().remove();
                            physics::PhysicSystem::removeCollider(collider);

                            debugbox.center = cld.mCenter;
                            debugbox.halfExtent = cld.mHalfExtent;

                            if (debugbox.debuglines != nullptr && debugbox.debuglines->isInitialized()) {
                                debugbox.update();
                                mEditor->mSelectedObject = &debugbox;
                                mEditor->gizmo.moveTo(debugbox.center);
                            }
                        };
                        if (ImGui::MenuItem("Duplicate")) {
                            debugbox.center = collider.mCenter;
                            debugbox.halfExtent = collider.mHalfExtent;

                            if (debugbox.debuglines != nullptr && debugbox.debuglines->isInitialized()) {
                                debugbox.update();
                                mEditor->mSelectedObject = &debugbox;
                                mEditor->gizmo.moveTo(debugbox.center);
                            }
                        };
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();  // Pop the unique ID for this item
                }
                ImGui::EndCombo();
            }
            ImGui::NewLine();
            ImGui::Separator();

            if (ImGui::Button("export colliders to json")) {
                for (auto& collider : physics::PhysicSystem::mColliders) {
                    std::cout << "{\n";
                    std::cout << "\"active\": " << "true" << ",\n";
                    std::cout << "\"name\": \"" << collider.mName << "\",\n";
                    std::cout << "\"type\": \"static\",\n";
                    std::cout << "\"shape\": \"box\",\n";
                    std::cout << "\"center\":"
                              << std::format("[{},{},{}]", collider.mCenter.x, collider.mCenter.y, collider.mCenter.z)
                              << ",\n";
                    std::cout << "\"half_extent\":"
                              << std::format("[{},{},{}]", collider.mHalfExtent.x, collider.mHalfExtent.y,
                                             collider.mHalfExtent.z)
                              << "\n";
                    std::cout << "},\n";
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Shadows")) {
            if (ImGui::Button("Save scene")) {
                mWorld->exportScene();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // Draw the UI
    ImGui::EndFrame();
    // Convert the UI defined above into low-level drawing commands
    ImGui::Render();
    // Execute the low-level drawing commands on the WebGPU backend
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

Camera& Application::getCamera() { return mCamera; }

RendererResource& Application::getRendererResource() { return *mRendererResource; }

BindingGroup& Application::getBindingGroup() { return mBindingGroup; }

Buffer& Application::getUniformBuffer() { return mUniformBuffer; }

CameraInfo& Application::getUniformData() { return mUniforms; }

const WGPUBindGroupLayout& Application::getObjectBindGroupLayout() const { return mBindGroupLayouts[1]; }
const WGPUBindGroupLayout* Application::getBindGroupLayouts() const { return mBindGroupLayouts.data(); }

std::pair<size_t, size_t> Application::getWindowSize() { return {mWindow->mWindowSize.x, mWindow->mWindowSize.y}; }

void Application::setWindowSize(size_t width, size_t height) { mWindow->mWindowSize = {width, height}; }

const std::vector<WGPUBindGroupEntry> Application::getDefaultTextureBindingData() const {
    return mDefaultTextureBindingData;
};

WGPUTextureView Application::getDepthStencilTarget() { return mDepthTextureView; }
WGPUTextureView Application::getColorTarget() { return mCurrentTargetView; }
