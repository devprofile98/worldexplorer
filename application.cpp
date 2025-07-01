#include "application.h"

#include <array>
#include <cstdint>
#include <format>
#include <future>
#include <iostream>
#include <random>
#include <vector>

#include "GLFW/glfw3.h"
#include "composition_pass.h"
#include "frustum_culling.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_wgpu.h"
#include "imgui/imgui.h"
#include "instance.h"
#include "model_registery.h"
#include "pipeline.h"
#include "point_light.h"
#include "shadow_pass.h"
#include "shapes.h"
#include "terrain_pass.h"
#include "texture.h"
#include "transparency_pass.h"
#include "utils.h"
#include "webgpu.h"
#include "wgpu.h"
#include "wgpu_utils.h"

// #define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

static bool look_as_light = false;
static float fov = 60.0f;
static float znear = 0.01f;
static float zfar = 200.0f;
static size_t which_frustum = 0;
static float middle_plane_length = 15.0f;
static float far_plane_length = 50.0f;
static float ddistance = 2.0f;
static float dd = 5.0f;
bool should_update_csm = true;
extern float sunlength;

double lastClickTime = 0.0;
double lastClickX = 0.0;
double lastClickY = 0.0;

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view) {
    const auto inv = glm::inverse(proj * view);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x) {
        for (unsigned int y = 0; y < 2; ++y) {
            for (unsigned int z = 0; z < 2; ++z) {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

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

    mLightViewSceneTexture = new Texture{
        mRendererResource.device,
        1920,
        1022,
        TextureDimension::TEX_2D,
        WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
    };
    mLightViewSceneTexture->createView();

    Texture grass_texture = Texture{mRendererResource.device, RESOURCE_DIR "/forrest_ground_diff.jpg"};
    grass_texture.createView();
    grass_texture.uploadToGPU(mRendererResource.queue);

    Texture rock_texture = Texture{mRendererResource.device, RESOURCE_DIR "/rock.jpg"};
    rock_texture.createView();
    rock_texture.uploadToGPU(mRendererResource.queue);

    Texture sand_texture = Texture{mRendererResource.device, RESOURCE_DIR "/gravelly_sand_diff.jpg"};
    sand_texture.createView();
    sand_texture.uploadToGPU(mRendererResource.queue);

    Texture grass_normal_texture = Texture{mRendererResource.device, RESOURCE_DIR "/gravelly_sand_diff.jpg"};
    grass_normal_texture.createView();
    grass_normal_texture.uploadToGPU(mRendererResource.queue);

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
    texture_data = {255, 120, 10, 255};  // White color for Default specular texture
    mDefaultMetallicRoughness->setBufferData(texture_data);
    mDefaultMetallicRoughness->uploadToGPU(mRendererResource.queue);

    mDefaultNormalMap = new Texture{mRendererResource.device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_normal_map_view = mDefaultNormalMap->createView();
    texture_data = {0, 255, 0, 255};  // White color for Default specular texture
    mDefaultNormalMap->setBufferData(texture_data);
    mDefaultNormalMap->uploadToGPU(mRendererResource.queue);

    mBindingGroup.addBuffer(0,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(MyUniform));

    mBindingGroup.addBuffer(1,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t));

    mBindingGroup.addSampler(2,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    mBindingGroup.addBuffer(3,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(LightingUniforms));

    mBindingGroup.addBuffer(4,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(Light) * 10);

    mBindingGroup.addTexture(5,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::DEPTH,
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
                             TextureViewDimension::ARRAY_2D);
    mBindingGroup.addBuffer(11,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(Scene) * 5);
    mBindingGroup.addSampler(12,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Compare);

    mBindingGroup.addBuffer(13,  //
                            BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                            mInstanceManager->mBufferSize);

    mBindingGroup.addBuffer(14,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(float));

    mBindingGroup.addTexture(15,  //
                             BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::VIEW_2D);

    mDefaultTextureBindingGroup.addTexture(0,  //
                                           BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);

    mDefaultTextureBindingGroup.addTexture(1,  //
                                           BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);
    mDefaultTextureBindingGroup.addTexture(2,  //
                                           BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);

    WGPUBindGroupLayout bind_group_layout = mBindingGroup.createLayout(this, "binding group layout");
    WGPUBindGroupLayout texture_bind_group_layout =
        mDefaultTextureBindingGroup.createLayout(this, "default texture bindgroup layout");

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
    mBindGroupLayouts[2] = texture_bind_group_layout;

    mPipeline =
        new Pipeline{this, {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2]}, "standard pipeline"};

    mPipeline->defaultConfiguration(this, mSurfaceFormat);
    setDefaultActiveStencil(mPipeline->getDepthStencilState());
    mPipeline->setDepthStencilState(mPipeline->getDepthStencilState());
    mPipeline->createPipeline(this);

    mStenctilEnabledPipeline =
        new Pipeline{this, {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2]}, "Draw outline pipe"};
    mStenctilEnabledPipeline->defaultConfiguration(this, mSurfaceFormat, WGPUTextureFormat_Depth24PlusStencil8)
        .createPipeline(this);

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

    mShadowPass = new ShadowPass{this};
    mShadowPass->createRenderPass(WGPUTextureFormat_RGBA8Unorm, 3);

    mTerrainPass = new TerrainPass{this};
    mTerrainPass->create(mSurfaceFormat);

    mOutlinePass = new OutlinePass{this};
    mOutlinePass->create(mSurfaceFormat, mDepthTextureViewDepthOnly);

    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mUniformBuffer;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(MyUniform);

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = mLightManager->getCountBuffer().getBuffer();
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = sizeof(uint32_t);

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
    mBindingData[4].size = sizeof(Light) * 10;

    /*mDepthTextureViewDepthOnly = mDepthTexture->createViewDepthOnly();*/

    mBindingData[5] = {};
    mBindingData[5].nextInChain = nullptr;
    mBindingData[5].binding = 5;
    // mBindingData[5].textureView = nullptr;
    mBindingData[5].textureView = mDepthTextureViewDepthOnly;

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

    /*mTransparencyPass = new TransparencyPass{this};*/
    /*mTransparencyPass->initializePass();*/
    /**/
    /*mCompositionPass = new CompositionPass{this};*/
    /*mCompositionPass->initializePass();*/

    /*static auto texture_array =*/
    /*    std::vector<WGPUTextureView>{mShadowPass->mNearFrustum->mShadowDepthTexture->getTextureViewArray(),*/
    /*                                 mShadowPass->mFarFrustum->mShadowDepthTexture->getTextureViewArray()};*/
    mBindingData[10] = {};
    mBindingData[10].nextInChain = nullptr;
    mBindingData[10].binding = 10;
    mBindingData[10].textureView = mShadowPass->getShadowMapView();

    mBindingData[11].nextInChain = nullptr;
    mBindingData[11].binding = 11;
    mBindingData[11].buffer = mLightSpaceTransformation.getBuffer();
    mBindingData[11].offset = 0;
    mBindingData[11].size = sizeof(Scene) * 5;

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

    mBindingData[13] = {};
    mBindingData[13].nextInChain = nullptr;
    mBindingData[13].buffer = mInstanceManager->getInstancingBuffer().getBuffer();
    mBindingData[13].binding = 13;
    mBindingData[13].offset = 0;
    mBindingData[13].size = mInstanceManager->mBufferSize;

    mTimeBuffer.setLabel("number of cascades buffer")
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion(false)
        .create(this);

    mBindingData[14] = {};
    mBindingData[14].nextInChain = nullptr;
    mBindingData[14].buffer = mTimeBuffer.getBuffer();
    mBindingData[14].binding = 14;
    mBindingData[14].offset = 0;
    mBindingData[14].size = sizeof(uint32_t);

    mBindingData[15] = {};
    mBindingData[15].nextInChain = nullptr;
    mBindingData[15].binding = 15;
    mBindingData[15].textureView = grass_normal_texture.getTextureView();

    mBindingGroup.create(this, mBindingData);
    mDefaultTextureBindingGroup.create(this, mDefaultTextureBindingData);

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
    mLightManager = LightManager::init(this);
    mInstanceManager = new InstanceManager{this, sizeof(glm::mat4) * 100000 * 15, 100000};

    /*water.load("water", this, RESOURCE_DIR "/bluecube.obj", mBindGroupLayouts[1])*/
    /*    .moveTo(glm::vec3{-3.725, -7.640, -3.425})*/
    /*    .scale(glm::vec3{100.0, 100.0, 1.0});*/
    /*water.uploadToGPU(this);*/
    /*water.setTransparent(false);*/
    /*water.useTexture(false);*/

    terrain.generate(200, 8, terrainData).uploadToGpu(this);
    std::cout << "Generate is " << terrainData.size() << '\n';

    shapes = new Cube{this};
    shapes->moveTo(glm::vec3{10.0f, 1.0f, 4.0f});

    plane = new Plane{this};
    plane->mName = "Plane";
    plane->moveTo(glm::vec3{3.0f, 1.0f, 4.0f}).scale(glm::vec3{0.01, 1.0, 1.0});
    plane->setTransparent(false);

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
        .setSize(sizeof(Scene) * 5)
        .setMappedAtCraetion()
        .create(this);

    mLightingUniforms.directions = {glm::vec4{0.5, 0.5, 0.4, 1.0}, glm::vec4{0.2, 0.4, 0.3, 1.0}};
    mLightingUniforms.colors = {glm::vec4{0.99, 1.0, 0.88, 1.0}, glm::vec4{0.6, 0.9, 1.0, 1.0}};
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                         lighting_buffer_descriptor.size);

    WGPUBufferDescriptor pointligth_buffer_descriptor = {};
    pointligth_buffer_descriptor.nextInChain = nullptr;
    pointligth_buffer_descriptor.label = ":::::";
    pointligth_buffer_descriptor.size = sizeof(Light) * 10;
    pointligth_buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    pointligth_buffer_descriptor.mappedAtCreation = false;
    mBuffer1 = wgpuDeviceCreateBuffer(mRendererResource.device, &pointligth_buffer_descriptor);

    glm::vec4 red = {1.0, 0.0, 0.0, 1.0};
    glm::vec4 blue = {0.0, 0.0, 1.0, 1.0};

    mLightManager->createPointLight({-2.5, -5.833, 0.184, 1.0}, red, red, red, 1.0, -3.0, 1.8);
    mLightManager->createPointLight({-1.0, -0.833, 1.0, 1.0}, blue, blue, blue, 1.0, -0.922, 1.8);
    mLightManager->createPointLight({-5.0, -3.0, 1.0, 1.0}, blue, blue, blue, 1.0, 0.7, 1.8);
    mLightManager->createPointLight({2.0, 2.0, 1.0, 1.0}, blue, blue, blue, 1.0, 0.7, 1.8);
    mLightManager->createSpotLight({1.0, 2.0, 1.0, 1.0}, {0.0, 0.0, -1.0f, 1.0f}, glm::cos(glm::radians(12.5f)),
                                   glm::cos(glm::radians(17.5f)));

    mLightManager->uploadToGpu(this, mBuffer1);
}

// We define a function that hides implementation-specific variants of device polling:
void wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser) {
    wgpuDevicePoll(device, false, nullptr);
}

void Application::onResize() {
    // Terminate in reverse order
    std::cout << "On rsize called -------------------------\n";
    terminateDepthBuffer();
    terminateSwapChain();

    // Re-init

    initSwapChain();
    initDepthBuffer();
    updateProjectionMatrix();
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

/*static int WINDOW_WIDTH = 1920;*/
/*static int WINDOW_HEIGHT = 1080;*/

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

    auto [window_width, window_height] = getWindowSize();
    GLFWwindow* provided_window = glfwCreateWindow(window_width, window_height, "World Explorer", nullptr, nullptr);
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

        auto [window_width, window_height] = that->getWindowSize();
        if (ypos <= 0) {
            glfwSetCursorPos(window, xpos, window_height - 1);
            if (that != nullptr) that->getCamera().updateCursor(xpos, window_height - 1);
        } else if (ypos > window_height - 20) {
            glfwSetCursorPos(window, xpos, 1);
            if (that != nullptr) that->getCamera().updateCursor(xpos, 1);
        }

        if (xpos <= 0) {
            glfwSetCursorPos(window, window_width - 1, ypos);
            if (that != nullptr) that->getCamera().updateCursor(window_width - 1, ypos);
        } else if (xpos >= window_width - 1) {
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
            that->getCamera().lookAtAABB(model);
        } else if (GLFW_KEY_KP_1 == key && action == GLFW_PRESS) {
            look_as_light = !look_as_light;

        } else if (GLFW_KEY_KP_2 == key && action == GLFW_PRESS) {
            that->mLightManager->nextLight();
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
    auto [w_width, w_height] = getWindowSize();
    surface_configuration.width = w_width;
    surface_configuration.height = w_height;
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

    if (!look_as_light) {
        auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());
        /*mShadowPass->setupScene(corners, which_frustum == true ? 1 : 0);*/
        /*mShadowPass->createFrustumSplits(corners, 10.0f, 100.0f);*/
    }
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, nullptr);
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(mRendererResource.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    mLoadedModel = {

        /*&water,*/
    };

    for (auto& model : mLoadedModel) {
        static_cast<Model*>(model)->createSomeBinding(this, mDefaultTextureBindingData);
    }

    std::cout << "On rsize called +++++++++++++++++++++\n";
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

    /*float time = glfwGetTime();*/
    /*std::cout << "value is " << new_value << std::endl;*/

    /*auto first_corner = getFrustumCornersWorldSpace(frustum_projection, mUniforms.viewMatrix);*/
    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = "command encoder descriptor";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, &encoder_descriptor);
    updateDragInertia();

    // preprocessing
    // doing frustum culling

    Frustum frustum{};
    /*frustum.extractPlanes(mCamera.getProjection() * mCamera.getView());*/

    // ---------------- 1 - Preparing for shadow pass ---------------
    // The first pass is the shadow pass, only based on the opaque objects

    if (!look_as_light && should_update_csm) {
        auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());
        auto all_scenes = mShadowPass->createFrustumSplits(
            corners, {
                         {0.0, middle_plane_length},
                         {0.0, middle_plane_length + far_plane_length},
                         {0.0, middle_plane_length + far_plane_length + 100} /*{0, 60},*/
                                                                             /*{0, 160}*/
                     });

        uint32_t new_value = all_scenes.size();
        wgpuQueueWriteBuffer(mRendererResource.queue, mTimeBuffer.getBuffer(), 0, &new_value, sizeof(uint32_t));

        frustum.createFrustumPlanesFromCorner(corners);
        wgpuQueueWriteBuffer(mRendererResource.queue, mLightSpaceTransformation.getBuffer(), 0, all_scenes.data(),
                             sizeof(Scene) * all_scenes.size());
    }

    // draw near
    WGPURenderPassEncoder shadow_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, mShadowPass->getRenderPassDescriptor(0));
    wgpuRenderPassEncoderSetPipeline(shadow_pass_encoder, mShadowPass->getPipeline()->getPipeline());
    mShadowPass->render(mLoadedModel, shadow_pass_encoder, 0);

    wgpuRenderPassEncoderEnd(shadow_pass_encoder);
    wgpuRenderPassEncoderRelease(shadow_pass_encoder);

    // draw far
    WGPURenderPassEncoder shadow_pass_encoder2 =
        wgpuCommandEncoderBeginRenderPass(encoder, mShadowPass->getRenderPassDescriptor(1));
    wgpuRenderPassEncoderSetPipeline(shadow_pass_encoder2, mShadowPass->getPipeline()->getPipeline());
    mShadowPass->render(mLoadedModel, shadow_pass_encoder2, 1);

    wgpuRenderPassEncoderEnd(shadow_pass_encoder2);
    wgpuRenderPassEncoderRelease(shadow_pass_encoder2);

    // draw very far
    WGPURenderPassEncoder shadow_pass_encoder3 =
        wgpuCommandEncoderBeginRenderPass(encoder, mShadowPass->getRenderPassDescriptor(2));
    wgpuRenderPassEncoderSetPipeline(shadow_pass_encoder3, mShadowPass->getPipeline()->getPipeline());
    mShadowPass->render(mLoadedModel, shadow_pass_encoder3, 2);

    wgpuRenderPassEncoderEnd(shadow_pass_encoder3);
    wgpuRenderPassEncoderRelease(shadow_pass_encoder3);

    /*mUniforms.time = static_cast<float>(glfwGetTime());*/
    mUniforms.setCamera(mCamera);
    /*std::cout << "camera position is " << glm::to_string(getCamera().getPos()) << std::endl;*/
    mUniforms.cameraWorldPosition = getCamera().getPos();
    auto all_scenes = mShadowPass->getScene();
    if (look_as_light) {
        auto all_scene = all_scenes[which_frustum];
        mUniforms.projectMatrix = all_scene.projection;
        mUniforms.viewMatrix = all_scene.view;
        mUniforms.modelMatrix = all_scene.model;
    }

    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, 0, &mUniforms, sizeof(MyUniform));

    //-------------- End of shadow pass
    // ---------------- 2 - begining of the opaque object color pass ---------------

    /*auto light_pass_descriptor =*/
    /*    createRenderPassDescriptor(mLightViewSceneTexture->getTextureView(), mDepthTextureView);*/
    /*WGPURenderPassEncoder light_space_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &light_pass_descriptor);*/
    /**/
    /*wgpuRenderPassEncoderSetPipeline(light_space_encoder, mPipeline2->getPipeline());*/
    /*for (const auto& model : mLoadedModel) {*/
    /*    if (!model->isTransparent()) {*/
    /*        model->draw(this, light_space_encoder, mBindingData);*/
    /*    }*/
    /*}*/
    /**/
    /*terrain.draw(this, light_space_encoder, mBindingData);*/
    /*wgpuRenderPassEncoderEnd(light_space_encoder);*/
    /*wgpuRenderPassEncoderRelease(light_space_encoder);*/

    // ------------------------------------
    /*auto render_pass_descriptor = createRenderPassDescriptor(target_view, mDepthTextureView);*/
    WGPURenderPassDescriptor render_pass_descriptor = {};

    {
        render_pass_descriptor.nextInChain = nullptr;

        static WGPURenderPassColorAttachment color_attachment = {};
        color_attachment.view = target_view;
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
        depth_stencil_attachment.depthLoadOp = WGPULoadOp_Clear;
        depth_stencil_attachment.depthStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.depthReadOnly = false;
        depth_stencil_attachment.stencilClearValue = 0;
        depth_stencil_attachment.stencilLoadOp = WGPULoadOp_Clear;
        depth_stencil_attachment.stencilStoreOp = WGPUStoreOp_Store;
        depth_stencil_attachment.stencilReadOnly = false;
        render_pass_descriptor.depthStencilAttachment = &depth_stencil_attachment;
        render_pass_descriptor.timestampWrites = nullptr;
    }

    // skybox and pbr render pass
    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(mUniforms.viewMatrix));
    glm::mat4 mvp = mUniforms.projectMatrix * viewNoTranslation;
    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mSkybox->getPipeline()->getPipeline());
    mSkybox->draw(this, render_pass_encoder, mvp);
    int32_t stencilReferenceValue = 240;
    wgpuRenderPassEncoderSetStencilReference(render_pass_encoder, stencilReferenceValue);

    for (const auto& model : mLoadedModel) {
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder,
                                         (model->isSelected() ? mPipeline : mStenctilEnabledPipeline)->getPipeline());

        if (!model->isTransparent()) {
            model->draw(this, render_pass_encoder, mBindingData);
        }
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    // end of color render pass
    // ---------------------------------------------------------------------

    // terrain pass
    mTerrainPass->setColorAttachment(
        {target_view, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mTerrainPass->setDepthStencilAttachment(
        {mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Store, LoadOp::Load, false});
    mTerrainPass->init();

    WGPURenderPassEncoder terrain_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, mTerrainPass->getRenderPassDescriptor());
    wgpuRenderPassEncoderSetPipeline(terrain_pass_encoder, mTerrainPass->getPipeline()->getPipeline());

    terrain.draw(this, terrain_pass_encoder, mBindingData);

    updateGui(terrain_pass_encoder);

    wgpuRenderPassEncoderEnd(terrain_pass_encoder);
    wgpuRenderPassEncoderRelease(terrain_pass_encoder);

    // ---------------------------------------------------------------------

    // outline pass
    mOutlinePass->setColorAttachment(
        {target_view, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mOutlinePass->setDepthStencilAttachment(
        {mDepthTextureView, StoreOp::Discard, LoadOp::Load, false, StoreOp::Discard, LoadOp::Load, false, 0.0});
    mOutlinePass->init();

    std::cout << "Debug for Render Pass Descriptor:" << std::endl;
    std::cout << "  depthLoadOp: " << mOutlinePass->mDepthStencilAttachment.get()->depthLoadOp
              << std::endl;  // Print the enum value
    std::cout << "  depthStoreOp: " << mOutlinePass->mDepthStencilAttachment.get()->depthStoreOp << std::endl;
    std::cout << "  depthReadOnly: " << mOutlinePass->mDepthStencilAttachment.get()->depthReadOnly << std::endl;
    std::cout << "  stencilLoadOp: " << mOutlinePass->mDepthStencilAttachment.get()->stencilLoadOp
              << std::endl;  // <-- Focus on this!
    std::cout << "  stencilStoreOp: " << mOutlinePass->mDepthStencilAttachment.get()->stencilStoreOp << std::endl;
    std::cout << "  stencilReadOnly: " << mOutlinePass->mDepthStencilAttachment.get()->stencilReadOnly << std::endl;

    WGPURenderPassEncoder outline_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, mOutlinePass->getRenderPassDescriptor());
    wgpuRenderPassEncoderSetStencilReference(outline_pass_encoder, stencilReferenceValue);

    mOutlinePass->mTextureView = mDepthTexture->createViewDepthOnly();
    mOutlinePass->createSomeBinding();

    wgpuRenderPassEncoderSetBindGroup(outline_pass_encoder, 3, mOutlinePass->mDepthTextureBindgroup.getBindGroup(), 0,
                                      nullptr);

    for (const auto& model : mLoadedModel) {
        if (model->isSelected()) {
            wgpuRenderPassEncoderSetPipeline(outline_pass_encoder, mOutlinePass->getPipeline()->getPipeline());
            model->draw(this, outline_pass_encoder, mBindingData);
        }
    }

    wgpuRenderPassEncoderEnd(outline_pass_encoder);
    wgpuRenderPassEncoderRelease(outline_pass_encoder);

    ModelRegistry::instance().tick(this);
    // ------------ 3- Transparent pass
    // Calculate the Accumulation Buffer from the transparent object, this pass does not draw
    // on the render Target
    /*auto transparency_pass_desc = mTransparencyPass->getRenderPassDescriptor();*/
    /*mTransparencyPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;*/
    /*WGPURenderPassEncoder transparency_pass_encoder =*/
    /*    wgpuCommandEncoderBeginRenderPass(encoder, transparency_pass_desc);*/
    /*wgpuRenderPassEncoderSetPipeline(transparency_pass_encoder, mTransparencyPass->getPipeline()->getPipeline());*/

    /*mShadowPass->setupScene({1.0f, 1.0f, 4.0f});*/
    /*mTransparencyPass->render(mLoadedModel, transparency_pass_encoder, mDepthTextureView);*/

    /*wgpuRenderPassEncoderEnd(transparency_pass_encoder);*/
    /*wgpuRenderPassEncoderRelease(transparency_pass_encoder);*/

    // ------------ 4- Composition pass
    // In this pass we will compose the result from opaque pass and the transparent pass
    /*auto ssbo_buffers = mTransparencyPass->getSSBOBuffers();*/
    /*mCompositionPass->setSSBOBuffers(ssbo_buffers.first, ssbo_buffers.second);*/
    /*mCompositionPass->mRenderPassDepthStencilAttachment.view = mDepthTextureView;*/
    /*mCompositionPass->mRenderPassColorAttachment.view = target_view;*/
    /*auto composition_pass_desc = mCompositionPass->getRenderPassDescriptor();*/
    /*WGPURenderPassEncoder composition_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder,
     * composition_pass_desc);*/
    /*wgpuRenderPassEncoderSetPipeline(composition_pass_encoder, mCompositionPass->getPipeline()->getPipeline());*/

    /*mCompositionPass->render(mLoadedModel, composition_pass_encoder, &render_pass_color_attachment);*/

    /*wgpuRenderPassEncoderEnd(composition_pass_encoder);*/
    /*wgpuRenderPassEncoderRelease(composition_pass_encoder);*/

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

    required_limits.limits.maxVertexAttributes = 6;
    required_limits.limits.maxVertexBuffers = 1;
    required_limits.limits.maxBufferSize = 134217728;  // 1000000 * sizeof(VertexAttributes);
    required_limits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    required_limits.limits.maxSampledTexturesPerShaderStage = 8;
    required_limits.limits.maxInterStageShaderComponents = 116;

    // Binding groups
    required_limits.limits.maxBindGroups = 4;
    required_limits.limits.maxUniformBuffersPerShaderStage = 6;
    required_limits.limits.maxUniformBufferBindingSize = 2048;  // 16 * 4 * sizeof(float);

    required_limits.limits.maxTextureDimension1D = 4096;
    required_limits.limits.maxTextureDimension2D = 4096;
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

    WGPUTextureFormat depth_texture_format = WGPUTextureFormat_Depth24PlusStencil8;
    mDepthTexture = new Texture{this->getRendererResource().device,
                                static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height),
                                TextureDimension::TEX_2D,
                                WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                depth_texture_format,
                                2,
                                "Standard depth texture"};

    // Create the view of the depth texture manipulated by the rasterizer
    mDepthTextureView = mDepthTexture->createViewDepthStencil();
    // 2. Create a WGPUTextureView for the DEPTH aspect only
    WGPUTextureViewDescriptor depthViewDesc = {};
    depthViewDesc.format = WGPUTextureFormat_Depth24Plus;  // Must match the base texture format
    depthViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthViewDesc.baseMipLevel = 0;
    depthViewDesc.mipLevelCount = 1;
    depthViewDesc.baseArrayLayer = 0;
    depthViewDesc.arrayLayerCount = 1;
    depthViewDesc.aspect = WGPUTextureAspect_DepthOnly;  // <-- CRITICAL: Specify Depth Only
    depthViewDesc.label = "Depth-Only View";             // Label for debugging

    mDepthTextureViewDepthOnly = wgpuTextureCreateView(mDepthTexture->getTexture(), &depthViewDesc);

    /*mDepthTextureViewDepthOnly = mDepthTexture->createViewDepthOnly();*/
    return mDepthTextureView != nullptr;
}

void Application::terminateDepthBuffer() {
    // wgpuTextureViewRelease(mDepthTextureView);
    // wgpuTextureDestroy(mDepthTexture->getTexture());
    // delete mDepthTexture;
}

void Application::updateProjectionMatrix() {
    int width, height;
    glfwGetFramebufferSize(mRendererResource.window, &width, &height);
    float ratio = width / (float)height;
    mUniforms.projectMatrix = glm::perspective(fov * Camera::PI / 180, ratio, znear, zfar);
    mCamera.setProjection(mUniforms.projectMatrix);
    /*frustum_projection = mUniforms.projectMatrix;*/

    /*wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer, offsetof(MyUniform, projectMatrix),*/
    /*                     &mUniforms.projectMatrix, sizeof(MyUniform::projectMatrix));*/
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

const double DOUBLE_CLICK_TIME_THRESHOLD = 0.3;               // Time in seconds (e.g., 300 milliseconds)
const double DOUBLE_CLICK_DISTANCE_THRESHOLD_SQ = 5.0 * 5.0;  // Max squared distance in pixels (e.g., 5x5 pixel area)

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
            double currentX, currentY;
            glfwGetCursorPos(mRendererResource.window, &currentX, &currentY);  // Get current mouse position

            double currentTime = glfwGetTime();  // Get current time
            double deltaTime = currentTime - lastClickTime;
            double distanceSq =
                (currentX - lastClickX) * (currentX - lastClickX) + (currentY - lastClickY) * (currentY - lastClickY);

            // Check if it's within the time and distance thresholds for a double-click

            auto [w_width, w_height] = getWindowSize();
            double xpos, ypos;
            glfwGetCursorPos(mRendererResource.window, &xpos, &ypos);
            double xndc = 2.0 * xpos / (float)w_width - 1.0;
            double yndc = 1.0 - (2.0 * ypos) / (float)w_height;
            glm::vec4 clip_coords = glm::vec4{xndc, yndc, 0.0, 1.0};
            std::cout << clip_coords.x << ":" << clip_coords.y << std::endl;
            glm::vec4 eyecoord = glm::inverse(mCamera.getProjection()) * clip_coords;
            eyecoord = glm::vec4{eyecoord.x, eyecoord.y, -1.0f, 0.0f};

            auto ray_origin = mCamera.getPos();

            glm::vec4 worldray = glm::inverse(mCamera.getView()) * eyecoord;
            auto normalized = glm::normalize(worldray);

            for (auto* obj : mLoadedModel) {
                auto [obj_in_world_min, obj_in_world_max] = obj->getWorldMin();
                bool does_intersect = intersection(ray_origin, normalized, obj_in_world_min, obj_in_world_max);
                if (does_intersect) {
                    if (mSelectedModel) {
                        mSelectedModel->selected(false);
                    }
                    mSelectedModel = obj;
                    mSelectedModel->selected(true);
                    if (deltaTime < DOUBLE_CLICK_TIME_THRESHOLD && distanceSq < DOUBLE_CLICK_DISTANCE_THRESHOLD_SQ) {
                        std::cout << "double click detected" << std::endl;
                        getCamera().lookAtAABB(obj);
                    }
                    std::cout << std::format("the ray {}intersect with {}\n", "", obj->getName());
                    break;
                }
            }

            lastClickTime = currentTime;
            lastClickX = currentX;
            lastClickY = currentY;
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
                /*if (mSelectedModel != nullptr) {*/
                /*    mSelectedModel->selected(false);*/
                /*    mSelectedModel = nullptr;*/
                /*}*/
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
    init_info.DepthStencilFormat = WGPUTextureFormat_Depth24PlusStencil8;
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
    if (ImGui::ColorEdit3("Color #0", glm::value_ptr(mLightingUniforms.colors[0])) ||
        ImGui::DragFloat3("Direction #0", glm::value_ptr(mLightingUniforms.directions[0]), 0.1, -1.0, 1.0)) {
        wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer, 0, &mLightingUniforms,
                             sizeof(LightingUniforms));
    }
    ImGui::InputFloat("sun length", &sunlength);
    // ImGui::ColorEdit3("Color #1", glm::value_ptr(mLightingUniforms.colors[1]));
    ImGui::DragFloat3("sun pos direction", glm::value_ptr(pointlightshadow), 0.1, -10.0, 10.0);
    static glm::vec3 new_ligth_position = {};
    ImGui::InputFloat3("create new light at:", glm::value_ptr(new_ligth_position));
    /*static float split_fcator = 1.0;*/
    ImGui::SliderFloat("frustum split factor", &middle_plane_length, 1.0, 100);
    ImGui::SliderFloat("far split factor", &far_plane_length, 1.0, 200);
    ImGui::SliderFloat("visualizer", &mUniforms.time, -1.0, 1.0);
    /*ImGui::SliderFloat("frustum split factor", &split_fcator, 1.0, 100);*/
    if (ImGui::InputInt("near far frstum", (int*)&which_frustum)) {
        /*auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());*/
    }
    ImGui::Text("Cascade number #%zu", which_frustum);

    ImGui::Checkbox("shoud update csm", &should_update_csm);

    static glm::vec3 water_orientation = glm::vec3{0.0f};
    ImGui::InputFloat3("Water plane orientation:", glm::value_ptr(water_orientation));
    if (ImGui::Button("Rotate", ImVec2(80, 30))) {
    }

    ImGui::End();

    ImGui::Begin("Add Line");

    static glm::vec3 start = glm::vec3{0.0f};
    static glm::vec3 end = glm::vec3{0.0f};
    static glm::vec3 color = glm::vec3{0.0f};
    /*static glm::vec3 color2 = glm::vec3{0.0f};*/

    ImGui::InputFloat3("Start", glm::value_ptr(start));
    ImGui::InputFloat3("End", glm::value_ptr(end));

    ImGui::ColorPicker3("lines color", glm::value_ptr(color));
    if (ImGui::Button("Create", ImVec2(100, 30))) {
        /*color2 = color;*/
        /*color2.x += color.z;*/
        /*color2.z += color.x;*/
        /*std::vector<glm::vec2> lines = {{0, 1}, {2, 3}, {6, 7}, {4, 5}, {0, 4}, {2, 6},*/
        /*                                {0, 2}, {4, 6}, {1, 5}, {1, 3}, {5, 7}, {3, 7}};*/
        /*for (const auto& l : lines) {*/
        /*    Line* line = new Line{this, mShadowPass->mFar[l.x], mShadowPass->mFar[l.y], 0.5, color};*/
        /*    line->setTransparent(false);*/
        /*    mLoadedModel.push_back(line);*/
        /*    Line* line2 = new Line{this, mShadowPass->mNear[l.x], mShadowPass->mNear[l.y], 0.5, color2};*/
        /*    line2->setTransparent(false);*/
        /*    mLoadedModel.push_back(line2);*/
        /*}*/
        /*should = !should;*/
        /*loadTree(this);*/
        /*for (auto [key, func] : ModelRegistry::instance().factories) {*/
        /*    futures.push_back(std::async(std::launch::async, func, this));*/
        /*}*/
    }
    if (ImGui::Button("remove frustum", ImVec2(100, 30))) {
        for (int i = 0; i < 24; i++) {
            mLoadedModel.pop_back();
        };
    }
    ImGui::SliderFloat("distance", &ddistance, 0.0, 30.0);
    ImGui::SliderFloat("dd", &dd, 0.0, 120.0);
    ImGui::End();
    mShadowPass->lightPos = pointlightshadow;
    /*mShadowPass->setupScene({1.0f, 1.0f, 4.0f});*/

    ImGui::Begin("Point Light");

    mLightManager->renderGUI();

    float tmp_znear = znear;
    ImGui::SliderFloat("z-near", &tmp_znear, 0.0, 180.0f);
    if (tmp_znear != znear) {
        znear = tmp_znear;
        updateProjectionMatrix();
    }

    float tmp_zfar = zfar;
    ImGui::SliderFloat("z-far", &tmp_zfar, 0.0, 1000.0f);
    if (tmp_zfar != zfar) {
        zfar = tmp_zfar;
        updateProjectionMatrix();
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
const WGPUBindGroupLayout* Application::getBindGroupLayouts() const { return mBindGroupLayouts.data(); }

std::pair<size_t, size_t> Application::getWindowSize() { return {mWindowWidth, mWindowHeight}; }

void Application::setWindowSize(size_t width, size_t height) {
    mWindowWidth = width;
    mWindowHeight = height;
}

const std::vector<WGPUBindGroupEntry> Application::getDefaultTextureBindingData() const {
    return mDefaultTextureBindingData;
};
