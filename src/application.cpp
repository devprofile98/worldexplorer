#include "application.h"

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "binding_group.h"
#include "glm/matrix.hpp"
#include "renderpass.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

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
#include "shadow_pass.h"
#include "shapes.h"
#include "terrain_pass.h"
#include "texture.h"
#include "transparency_pass.h"
#include "utils.h"
#include "water_pass.h"
#include "webgpu/webgpu.h"
#include "wgpu_utils.h"

// #define IMGUI_IMPL_WEBGPU_BACKEND_WGPU

static bool look_as_light = false;
static bool cull_frustum = true;

static float fov = 60.0f;
static float znear = 0.01f;
static float zfar = 200.0f;
static size_t which_frustum = 0;
static float middle_plane_length = 15.0f;
static float far_plane_length = 50.0f;
static float ddistance = 2.0f;
static float dd = 5.0f;
bool should_update_csm = true;

double lastClickTime = 0.0;
double lastClickX = 0.0;
double lastClickY = 0.0;
Frustum frustumm{};

float rotrot = 0.f;

void beginPass(NewRenderPass* renderPass, WGPUCommandEncoder encoder,
               std::function<void(WGPURenderPassEncoder encoder)> drawFunc) {
    WGPURenderPassEncoder terrain_pass_encoder =
        wgpuCommandEncoderBeginRenderPass(encoder, &renderPass->mRenderPassDesc);

    drawFunc(terrain_pass_encoder);

    wgpuRenderPassEncoderRelease(terrain_pass_encoder);
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

    mLineEngine = new LineEngine{};

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

    // Creating default normal-map texture
    mDefaultNormalMap = new Texture{mRendererResource.device, 1, 1, TextureDimension::TEX_2D};
    WGPUTextureView default_normal_map_view = mDefaultNormalMap->createView();
    texture_data = {0, 255, 0, 255};  // White color for Default specular texture
    mDefaultNormalMap->setBufferData(texture_data);
    mDefaultNormalMap->uploadToGPU(mRendererResource.queue);

    // Initializing Default bindgroups
    mBindingGroup.addBuffer(0,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                            sizeof(MyUniform) * 10);

    mBindingGroup.addBuffer(1,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(uint32_t));

    mBindingGroup.addSampler(2,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    mBindingGroup.addBuffer(3,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(LightingUniforms));

    mBindingGroup.addBuffer(4,  //
                            BindGroupEntryVisibility::FRAGMENT, BufferBindingType::UNIFORM, sizeof(Light) * 10);

    mBindingGroup.addBuffer(5,  //
                            BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM, sizeof(float));

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

    /* Default textures for the render pass, if a model doenst have textures, these will be used */
    mDefaultTextureBindingGroup.addTexture(0,  //
                                           BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);

    mDefaultTextureBindingGroup.addTexture(1,  //
                                           BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);
    mDefaultTextureBindingGroup.addTexture(2,  //
                                           BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::FLAOT,
                                           TextureViewDimension::VIEW_2D);

    /* Default camera index buffer
     * Each render pass could have its own camear index to use a different camera in shader
     * */
    mDefaultCameraIndexBindgroup.addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                           sizeof(uint32_t));

    mDefaultClipPlaneBG.addBuffer(0,  //
                                  BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                  sizeof(glm::vec4));

    mDefaultVisibleBuffer.addBuffer(0,  //
                                    BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                                    sizeof(uint32_t) * 13000);

    /* Default Object Detail bindgroup*/
    BindingGroup object_information;
    object_information.addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                                 sizeof(ObjectInfo));
    object_information.addBuffer(1, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM,
                                 100 * sizeof(glm::mat4));

    /* Creating bindgroup layouts */
    /* ************************** */
    WGPUBindGroupLayout bind_group_layout = mBindingGroup.createLayout(this, "binding group layout");
    WGPUBindGroupLayout texture_bind_group_layout =
        mDefaultTextureBindingGroup.createLayout(this, "default texture bindgroup layout");

    WGPUBindGroupLayout camera_bind_group_layout =
        mDefaultCameraIndexBindgroup.createLayout(this, "default camera index bindgroup layout");

    WGPUBindGroupLayout clipplane_bind_group_layout =
        mDefaultClipPlaneBG.createLayout(this, "default clip plane bindgroup layout");

    WGPUBindGroupLayout visible_bind_group_layout =
        mDefaultVisibleBuffer.createLayout(this, "default visible index layout");

    WGPUBindGroupLayout obj_transform_layout =
        object_information.createLayout(this, "Object Tranformation Matrix uniform");

    mBindGroupLayouts[0] = bind_group_layout;
    mBindGroupLayouts[1] = obj_transform_layout;
    mBindGroupLayouts[2] = texture_bind_group_layout;
    mBindGroupLayouts[3] = camera_bind_group_layout;
    mBindGroupLayouts[4] = clipplane_bind_group_layout;
    mBindGroupLayouts[5] = visible_bind_group_layout;

    mPipeline = new Pipeline{this,
                             {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2], mBindGroupLayouts[3],
                              mBindGroupLayouts[4], mBindGroupLayouts[5]},
                             "standard pipeline"};

    mPipeline->defaultConfiguration(this, mSurfaceFormat);
    setDefaultActiveStencil(mPipeline->getDepthStencilState());
    mPipeline->setDepthStencilState(mPipeline->getDepthStencilState());
    mPipeline->createPipeline(this);

    mStenctilEnabledPipeline = new Pipeline{this,
                                            {bind_group_layout, mBindGroupLayouts[1], mBindGroupLayouts[2],
                                             mBindGroupLayouts[3], mBindGroupLayouts[4], mBindGroupLayouts[5]},
                                            "Draw outline pipe"};
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

    mDefaultCameraIndex.setLabel("defualt camera index buffer")
        .setMappedAtCraetion()
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(this);

    static uint32_t cidx = 0;
    wgpuQueueWriteBuffer(mRendererResource.queue, mDefaultCameraIndex.getBuffer(), 0, &cidx, sizeof(uint32_t));

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
        .create(this);

    // glm::vec4 default_clip_plane{0.0, 0.0, 1.0, 100};
    wgpuQueueWriteBuffer(mRendererResource.queue, mDefaultClipPlaneBuf.getBuffer(), 0, glm::value_ptr(mDefaultPlane),
                         sizeof(glm::vec4));

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

    mDefaultClipPlaneBGData[0].nextInChain = nullptr;
    mDefaultClipPlaneBGData[0].binding = 0;
    mDefaultClipPlaneBGData[0].buffer = mDefaultClipPlaneBuf.getBuffer();
    mDefaultClipPlaneBGData[0].offset = 0;
    mDefaultClipPlaneBGData[0].size = sizeof(glm::vec4);

    /* Initializing Render Passes */
    mShadowPass = new ShadowPass{this, "Shadow pass"};
    mShadowPass->createRenderPass(WGPUTextureFormat_RGBA8Unorm, 3);

    // mDepthPrePass = new DepthPrePass{this, "Depth PrePass", mDepthTextureView};
    // mDepthPrePass->createRenderPass(WGPUTextureFormat_RGBA8Unorm);
    //
    mDepthPrePass = new DepthPrePass{this, "Depth PrePass", mDepthTextureView};
    mDepthPrePass->createRenderPass(WGPUTextureFormat_RGBA8Unorm);

    mTerrainPass = new TerrainPass{this, "Terrain Render Pass"};
    mTerrainPass->create(mSurfaceFormat);

    m3DviewportPass = new ViewPort3DPass{this, "ViewPort 3D Render Pass"};

    mLineRenderingPass = new NewRenderPass{"Line Rendering render pass"};
    mLineRenderingPass->setColorAttachment(
        {this->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mLineRenderingPass->setDepthStencilAttachment(
        {this->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    mLineRenderingPass->init();

    initDepthBuffer();

    m3DviewportPass->create(mSurfaceFormat);

    mOutlinePass = new OutlinePass{this, "Outline Render Pass"};
    mOutlinePass->create(mSurfaceFormat, mDepthTextureViewDepthOnly);

    mWaterPass = new WaterReflectionPass{this, "Water Reflection Pass"};
    mWaterPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

    mWaterRefractionPass = new WaterRefractionPass{this, "Water Refraction Pass"};
    mWaterRefractionPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

    mTerrainForRefraction = new NewRenderPass{"Terrain for refraction"};

    mTerrainForRefraction->setColorAttachment({mWaterRefractionPass->mRenderTargetView, nullptr,
                                               WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mTerrainForRefraction->setDepthStencilAttachment({mWaterRefractionPass->mDepthTextureView, StoreOp::Store,
                                                      LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined,
                                                      false});
    mTerrainForRefraction->init();

    //
    mTerrainForReflection = new NewRenderPass{"Terrain for reflection"};
    mTerrainForReflection->setColorAttachment(
        {mWaterPass->mRenderTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mTerrainForReflection->setDepthStencilAttachment({mWaterPass->mDepthTextureView, StoreOp::Store, LoadOp::Load,
                                                      false, StoreOp::Undefined, LoadOp::Undefined, false});
    mTerrainForReflection->init();
    //
    //
    //

    mWaterRenderPass =
        new WaterPass{this, mWaterPass->mRenderTarget, mWaterRefractionPass->mRenderTarget, "Water pass"};
    mWaterRenderPass->createRenderPass(WGPUTextureFormat_BGRA8UnormSrgb);

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
    mBindingData[0].size = sizeof(MyUniform) * 10;

    mLineEngine->initialize(this);

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
        .create(this);

    mBindingData[5] = {};
    mBindingData[5].nextInChain = nullptr;
    mBindingData[5].buffer = mTimeBuffer.getBuffer();
    mBindingData[5].binding = 5;
    mBindingData[5].offset = 0;
    mBindingData[5].size = sizeof(uint32_t);

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

    mDefaultBoneFinalTransformData.setLabel("default bone final transform")
        .setSize(100 * sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
        .setMappedAtCraetion(false)
        .create(this);

    static std::vector<glm::mat4> bones;
    bones.reserve(100);
    for (int i = 0; i < 100; i++) {
        bones.emplace_back(glm::mat4{1.0});
    }
    wgpuQueueWriteBuffer(getRendererResource().queue, mDefaultBoneFinalTransformData.getBuffer(), 0, bones.data(),
                         sizeof(glm::mat4) * bones.size());

    mDefaultVisibleBGData[0] = {};
    mDefaultVisibleBGData[0].nextInChain = nullptr;
    mDefaultVisibleBGData[0].buffer = mVisibleIndexBuffer.getBuffer();
    mDefaultVisibleBGData[0].binding = 0;
    mDefaultVisibleBGData[0].offset = 0;
    mDefaultVisibleBGData[0].size = sizeof(uint32_t) * 100'000 * 5;

    /* Creating actual bindgroups */
    mBindingGroup.create(this, mBindingData);
    mDefaultTextureBindingGroup.create(this, mDefaultTextureBindingData);
    mDefaultCameraIndexBindgroup.create(this, mDefaultCameraIndexBindingData);
    mDefaultClipPlaneBG.create(this, mDefaultClipPlaneBGData);
    mDefaultVisibleBuffer.create(this, mDefaultVisibleBGData);

    mSkybox = new SkyBox{this, RESOURCE_DIR "/skybox"};
}

// Initializing Vertex Buffers
void Application::initializeBuffers() {
    // initialize The instancing buffer
    mLightManager = LightManager::init(this);
    mInstanceManager = new InstanceManager{this, sizeof(InstanceData) * 100000 * 10, 100000};

    terrain.generate(200, 8, terrainData).uploadToGpu(this);
    std::cout << "Generate is " << terrainData.size() << '\n';

    shapes = new Cube{this};
    shapes->mTransform.moveTo(glm::vec3{10.0f, 1.0f, 4.0f});

    plane = new Plane{this};
    plane->mName = "Plane";
    plane->mTransform.moveTo(glm::vec3{3.0f, 1.0f, 4.0f}).scale(glm::vec3{0.01, 1.0, 1.0});
    plane->setTransparent(false);

    mUniformBuffer.setLabel("MVP matrices matrix")
        .setSize(sizeof(MyUniform) * 10)
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(this);

    mUniforms.time = 1.0f;
    mUniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};
    // setupCamera(mUniforms);
    mCamera = Camera{{-0.0f, -0.0f, 0.0f}, glm::vec3{0.8f}, {1.0, 0.0, 0.0}, 0.0};
    mUniforms.setCamera(mCamera);
    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer.getBuffer(), 0, &mUniforms, sizeof(MyUniform) * 10);

    mDirectionalLightBuffer.setLabel("Directional light buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(LightingUniforms))
        .setMappedAtCraetion()
        .create(this);

    std::cout << "Generate is " << terrainData.size() << '\n';
    mLightSpaceTransformation.setLabel("Light space transform buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Scene) * 5)
        .setMappedAtCraetion()
        .create(this);

    mLightingUniforms.directions = {glm::vec4{0.7, 0.4, 1.0, 1.0}, glm::vec4{0.2, 0.4, 0.3, 1.0}};
    mLightingUniforms.colors = {glm::vec4{0.99, 1.0, 0.88, 1.0}, glm::vec4{0.6, 0.9, 1.0, 1.0}};
    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer.getBuffer(), 0, &mLightingUniforms,
                         sizeof(LightingUniforms));

    mLightBuffer.setLabel("Lights Buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Light) * 10)
        .setMappedAtCraetion(false)
        .create(this);
    glm::vec4 red = {1.0, 0.0, 0.0, 1.0};
    glm::vec4 blue = {0.0, 0.0, 1.0, 1.0};

    mLightManager->createPointLight({-2.5, -5.833, 0.184, 1.0}, red, red, red, 1.0, -3.0, 1.8, "red light 1");
    mLightManager->createPointLight({-1.0, -0.833, 1.0, 1.0}, blue, blue, blue, 1.0, -0.922, 1.8, "blue light 1");
    mLightManager->createPointLight({-5.0, -3.0, 1.0, 1.0}, blue, blue, blue, 1.0, 0.7, 1.8, "blue light 2");
    mLightManager->createPointLight({2.0, 2.0, 1.0, 1.0}, blue, blue, blue, 1.0, 0.7, 1.8, "blue light 3");
    mLightManager->createSpotLight({1.0, 2.0, 1.0, 1.0}, {0.0, 0.0, -1.0f, 1.0f}, glm::cos(glm::radians(12.5f)),
                                   glm::cos(glm::radians(17.5f)), "spotlight ");

    mLightManager->uploadToGpu(this, mLightBuffer.getBuffer());

    setupComputePass(this, mInstanceManager->getInstancingBuffer().getBuffer());
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

    mOutlinePass->mTextureView = mDepthTexture->createViewDepthOnly();
    mOutlinePass->createSomeBinding();
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
    glfwMakeContextCurrent(provided_window);  // window: your GLFWwindow*
    glfwSwapInterval(0);
    // Set up Callbacks
    glfwSetWindowUserPointer(provided_window, this);  // set user pointer to be used in the callback function
    glfwSetFramebufferSizeCallback(provided_window, onWindowResize);
    glfwSetCursorPosCallback(provided_window, InputManager::handleMouseMove);
    glfwSetMouseButtonCallback(provided_window, InputManager::handleButton);
    glfwSetScrollCallback(provided_window, InputManager::handleScroll);
    glfwSetKeyCallback(provided_window, InputManager::handleKeyboard);

    Screen::instance().initialize(this);
    InputManager::instance().mMouseMoveListeners.push_back(&Screen::instance());
    InputManager::instance().mMouseButtonListeners.push_back(&Screen::instance());
    InputManager::instance().mMouseMoveListeners.push_back(&mEditor.gizmo);
    InputManager::instance().mMouseButtonListeners.push_back(&mEditor.gizmo);
    InputManager::instance().mMouseScrollListeners.push_back(&Screen::instance());
    InputManager::instance().mKeyListener.push_back(&Screen::instance());

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

    {
        int width, height;
        // Use glfwGetFramebufferSize to get the dimensions of the window's framebuffer.
        glfwGetFramebufferSize(provided_window, &width, &height);
        std::cout << std::format("width: {} heigth: {}\n", width, height);
        setWindowSize(width, height);
    }

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

    initGui();

    initializeBuffers();
    initializePipeline();

    return true;
}

void Application::mainLoop() {
    glfwPollEvents();

    double time = glfwGetTime();
    mCurrentTargetView = getNextSurfaceTextureView();
    if (mCurrentTargetView == nullptr) {
        return;
    }

    {
        // PerfTimer timer{"tick"};
        for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
            model->mAnimationSecond = std::fmod(time, model->mAnimationDuration) * 1000.0f;
            model->ExtractBonePositions();
        }
    }

    // create a commnad encoder
    WGPUCommandEncoderDescriptor encoder_descriptor = {};
    encoder_descriptor.nextInChain = nullptr;
    encoder_descriptor.label = createStringViewC("command encoder descriptor");
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mRendererResource.device, &encoder_descriptor);
    mRendererResource.commandEncoder = encoder;

    auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());

    // Dispaching Compute shaders to cull everything that is outside the frustum
    // -------------------------------------------------------------------------
    auto fp = create2FrustumPlanes(corners);
    wgpuQueueWriteBuffer(mRendererResource.queue, getFrustumPlaneBuffer().getBuffer(), 0, fp.data(),
                         sizeof(FrustumPlanesUniform));

    if (cull_frustum) {
        runFrustumCullingTask(this, encoder);
    }

    // -------------------------------------------------------------------------

    // ---------------- 1 - Preparing for shadow pass ---------------
    // The first pass is the shadow pass, only based on the opaque objects
    if (!look_as_light && should_update_csm) {
        auto all_scenes = mShadowPass->createFrustumSplits(
            corners,
            {
                {0.0, middle_plane_length},
                {middle_plane_length, middle_plane_length + far_plane_length},
                {middle_plane_length + far_plane_length, middle_plane_length + far_plane_length + 100} /*{0, 60},*/
                                                                                                       /*{0, 160}*/
            });

        uint32_t new_value = all_scenes.size();
        wgpuQueueWriteBuffer(mRendererResource.queue, mTimeBuffer.getBuffer(), 0, &new_value, sizeof(uint32_t));

        frustumm.createFrustumPlanesFromCorner(corners);
        wgpuQueueWriteBuffer(mRendererResource.queue, mLightSpaceTransformation.getBuffer(), 0, all_scenes.data(),
                             sizeof(Scene) * all_scenes.size());
    }

    {
        mShadowPass->renderAllCascades(encoder);
    }
    //-------------- End of shadow pass

    mUniforms.setCamera(mCamera);
    mUniforms.cameraWorldPosition = getCamera().getPos();

    wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer.getBuffer(), 0, &mUniforms, sizeof(MyUniform));

    // ---------------- 2 - begining of the opaque object color pass ---------------

    // inverting pitch and reflect camera based on the water plane
    // auto water_plane = ModelRegistry::instance().getLoadedModel(Visibility_User).find("water");
    for (auto* model : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
        if (model->mName == "water") {
            // if (model != ModelRegistry::instance().getLoadedModel(Visibility_User).end()) {
            static Camera camera = getCamera();
            camera = getCamera();
            static MyUniform muniform = mUniforms;
            float diff = 2 * (camera.getPos().z - model->mTransform.getPosition().z);
            auto new_pos = camera.getPos();
            new_pos.z -= diff;
            camera.setPosition(new_pos);
            camera.mPitch *= -1.0;
            camera.updateCamera();
            muniform.setCamera(camera);
            muniform.cameraWorldPosition = camera.getPos();
            wgpuQueueWriteBuffer(mRendererResource.queue, mUniformBuffer.getBuffer(), sizeof(MyUniform), &muniform,
                                 sizeof(MyUniform));
            glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(muniform.viewMatrix));
            glm::mat4 mvp = muniform.projectMatrix * viewNoTranslation;
            auto reflected_camera = mvp;
            wgpuQueueWriteBuffer(mRendererResource.queue, mSkybox->mReflectedCameraMatrix.getBuffer(), 0,
                                 &reflected_camera, sizeof(glm::mat4));
        }
    }

    {
        WGPURenderPassDescriptor render_pass_descriptor = {};

        {
            render_pass_descriptor.nextInChain = nullptr;

            static WGPURenderPassColorAttachment color_attachment = {};
            color_attachment.view = mWaterPass->mRenderTargetView;
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
            depth_stencil_attachment.view = mWaterPass->mDepthTextureView;
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

        WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(mUniforms.viewMatrix));
        glm::mat4 mvp = mUniforms.projectMatrix * viewNoTranslation;
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mSkybox->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mSkybox->mReflectedBindingGroup.getBindGroup(), 0,
                                          nullptr);
        mSkybox->draw(this, render_pass_encoder, mvp);

        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }

    mWaterPass->execute(encoder);

    // Test for scene hirarchy
    // static bool reparenting = true;
    // {
    // auto& iter = ModelRegistry::instance().getLoadedModel(Visibility_User);
    // auto boat = iter.find("tower");
    // auto arrow = iter.find("arrow");
    // auto desk = iter.find("boat");
    // if (boat != iter.end() && arrow != iter.end() && desk != iter.end()) {
    //     if (reparenting) {
    //         reparenting = false;
    //         // boat->second->mTransform.mTransformMatrix = new_transform;
    //
    //         arrow->second->addChildren(static_cast<BaseModel*>(boat->second));
    //         boat->second->addChildren(static_cast<BaseModel*>(desk->second));
    //     }
    //
    //     arrow->second->update();
    // }
    // auto human = iter.find("human");
    // auto sphere = iter.find("sphere");
    // if (human != iter.end() && sphere != iter.end()) {
    //     if (reparenting) {
    //         reparenting = false;
    //         // loadSphereAtHumanBones(this, human->second, sphere->second);
    //     }
    // }
    // }

    // ---------- Terrain Render Pass for Water Reflection
    beginPass(mTerrainForReflection, encoder, [&](WGPURenderPassEncoder pass_encoder) {
        wgpuRenderPassEncoderSetPipeline(pass_encoder, mTerrainPass->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mWaterPass->mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                          nullptr);

        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mWaterPass->mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

        terrain.draw(this, pass_encoder, mBindingData);

        wgpuRenderPassEncoderEnd(pass_encoder);
    });

    //----------------------------  Water Refraction Pass
    mWaterRefractionPass->execute(encoder);
    // ----------------------------------------------

    // ---------- Terrain Render Pass for Water Refraction
    beginPass(mTerrainForRefraction, encoder, [&](WGPURenderPassEncoder pass_encoder) {
        wgpuRenderPassEncoderSetPipeline(pass_encoder, mTerrainPass->getPipeline()->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 4, mWaterRefractionPass->mDefaultClipPlaneBG.getBindGroup(), 0,
                                          nullptr);
        wgpuRenderPassEncoderSetBindGroup(pass_encoder, 5, mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

        terrain.draw(this, pass_encoder, mBindingData);

        wgpuRenderPassEncoderEnd(pass_encoder);
    });

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
                if (model->mName != "water") {
                    // continue;
                    model->draw(this, render_pass_encoder);
                }
                // if (!model->isTransparent()) {
                // }
            }
        }

        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }
    // ----------------------------------------------

    // ---- Skybox and PBR render pass

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
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mPipeline->getPipeline());
        for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
            if (model->mName != "water") {
                // continue;

                // std::cout << mPipeline->getPipelineLayout  .depthStencil->depthCompare;
                if (!model->isTransparent()) {
                    model->draw(this, render_pass_encoder);
                }
            }
        }

        // for (const auto& model : mLines) {
        //     model->draw(this, render_pass_encoder);
        // }
    }

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);
    // ---------------------------------------------------------------------
    {
        mLineRenderingPass->setColorAttachment(
            {this->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
        mLineRenderingPass->setDepthStencilAttachment({this->mDepthTextureView, StoreOp::Store, LoadOp::Load, false,
                                                       StoreOp::Undefined, LoadOp::Undefined, false});
        mLineRenderingPass->init();

        WGPURenderPassEncoder render_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(encoder, &mLineRenderingPass->mRenderPassDesc);
        wgpuRenderPassEncoderSetPipeline(render_pass_encoder, mLineEngine->mPipeline->getPipeline());
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, mLineEngine->mBindGroup.getBindGroup(), 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 1, mLineEngine->mCameraBindGroup.getBindGroup(), 0,
                                          nullptr);
        mLineEngine->draw(this, render_pass_encoder);
        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }
    // ---------------------------------------------------------------------
    {
        mWaterRenderPass->setColorAttachment(
            {mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
        mWaterRenderPass->setDepthStencilAttachment(
            {mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Store, LoadOp::Load, false, 1.0});
        mWaterRenderPass->init();

        WGPURenderPassEncoder water_render_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(encoder, mWaterRenderPass->getRenderPassDescriptor());
        for (const auto& model : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
            if (model->mName == "water") {
                // continue;
                wgpuRenderPassEncoderSetPipeline(water_render_pass_encoder,
                                                 mWaterRenderPass->getPipeline()->getPipeline());
                wgpuRenderPassEncoderSetBindGroup(water_render_pass_encoder, 3,
                                                  mDefaultCameraIndexBindgroup.getBindGroup(), 0, nullptr);

                wgpuRenderPassEncoderSetBindGroup(water_render_pass_encoder, 4,
                                                  mWaterRenderPass->mWaterTextureBindGroup.getBindGroup(), 0, nullptr);
                if (!model->isTransparent()) {
                    model->draw(this, water_render_pass_encoder);
                }
            }
        }
        wgpuRenderPassEncoderEnd(water_render_pass_encoder);
        wgpuRenderPassEncoderRelease(water_render_pass_encoder);
    }

    // terrain pass
    {
        mTerrainPass->setColorAttachment(
            {mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
        mTerrainPass->setDepthStencilAttachment(
            {mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
        mTerrainPass->init();

        WGPURenderPassEncoder terrain_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(encoder, mTerrainPass->getRenderPassDescriptor());
        wgpuRenderPassEncoderSetPipeline(terrain_pass_encoder, mTerrainPass->getPipeline()->getPipeline());

        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 3, mDefaultCameraIndexBindgroup.getBindGroup(), 0,
                                          nullptr);

        wgpuRenderPassEncoderSetBindGroup(terrain_pass_encoder, 4, mDefaultClipPlaneBG.getBindGroup(), 0, nullptr);
        terrain.draw(this, terrain_pass_encoder, mBindingData);

        updateGui(terrain_pass_encoder, time);

        wgpuRenderPassEncoderEnd(terrain_pass_encoder);
        wgpuRenderPassEncoderRelease(terrain_pass_encoder);
    }

    // ---------------------------------------------------------------------

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
    // wgpuRenderPassEncoderSetBindGroup(outline_pass_encoder, 3, mOutlinePass->mDepthTextureBindgroup.getBindGroup(),
    // 0,
    //                                   nullptr);
    // wgpuRenderPassEncoderSetBindGroup(outline_pass_encoder, 4, mDefaultCameraIndexBindgroup.getBindGroup(), 0,
    // nullptr);
    //
    // for (const auto& [name, model] : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
    //     if (model->isSelected()) {
    //         wgpuRenderPassEncoderSetPipeline(outline_pass_encoder, mOutlinePass->getPipeline()->getPipeline());
    //         model->draw(this, outline_pass_encoder, mBindingData);
    //     }
    // }
    //
    // wgpuRenderPassEncoderEnd(outline_pass_encoder);
    // wgpuRenderPassEncoderRelease(outline_pass_encoder);

    // 3D editor elements pass
    m3DviewportPass->execute(encoder);

    // polling if any model loading process is done and append it to loaded model list
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
    /*mCompositionPass->mRenderPassColorAttachment.view = mCurrentTargetView;*/
    /*auto composition_pass_desc = mCompositionPass->getRenderPassDescriptor();*/
    /*WGPURenderPassEncoder composition_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder,
     * composition_pass_desc);*/
    /*wgpuRenderPassEncoderSetPipeline(composition_pass_encoder, mCompositionPass->getPipeline()->getPipeline());*/

    /*mCompositionPass->render(mLoadedModel, composition_pass_encoder, &render_pass_color_attachment);*/

    /*wgpuRenderPassEncoderEnd(composition_pass_encoder);*/
    /*wgpuRenderPassEncoderRelease(composition_pass_encoder);*/

    {
        // PerfTimer timer{"test"};

        WGPUCommandBufferDescriptor command_buffer_descriptor = {};
        command_buffer_descriptor.nextInChain = nullptr;
        command_buffer_descriptor.label = createStringView("command buffer");
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &command_buffer_descriptor);

        wgpuDevicePoll(mRendererResource.device, false, nullptr);  // This is good!
        wgpuQueueSubmit(mRendererResource.queue, 1, &command);
        // static WGPUQueueWorkDoneCallbackInfo cbinfo{};
        // cbinfo.nextInChain = nullptr;
        // cbinfo.callback = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
        //     Application* app = (Application*)userdata1;
        //     if (status == WGPUQueueWorkDoneStatus_Success) {
        //         app->ccounter++;
        //     }
        // };
        //
        // cbinfo.userdata1 = this;

        // wgpuQueueOnSubmittedWorkDone(mRendererResource.queue, cbinfo);
        wgpuCommandBufferRelease(command);
        wgpuCommandEncoderRelease(encoder);

        wgpuTextureViewRelease(mCurrentTargetView);
    }

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
    wgpuBufferRelease(mLightBuffer.getBuffer());
    wgpuBufferRelease(mUniformBuffer.getBuffer());
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

WGPULimits Application::GetRequiredLimits(WGPUAdapter adapter) const {
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

void Application::updateGui(WGPURenderPassEncoder renderPass, double time) {
    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginTabBar("ObjectTabs")) {
        if (ImGui::BeginTabItem("Scene")) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            if (ImGui::CollapsingHeader("Camera",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
                                                                            //
                if (ImGui::SliderFloat("z-near", &znear, 0.0, 180.0f) ||
                    ImGui::SliderFloat("z-far", &zfar, 0.0, 1000.0f) ||
                    ImGui::DragFloat("FOV", &fov, 0.1, 0.0, 1000.0f)) {
                    updateProjectionMatrix();
                }

                if (ImGui::Button("Reset to default Params")) {
                    fov = 60.0f;
                    znear = 0.01f;
                    zfar = 200.0f;
                    updateProjectionMatrix();
                }

                ImGui::Checkbox("cull frustum", &cull_frustum);
            }

            // static glm::vec3 pointlightshadow = glm::vec3{5.6f, -2.1f, 6.0f};
            if (ImGui::CollapsingHeader("Lights",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
                                                                            // ImGui::Begin("Lighting");
                ImGui::Text("Directional Light");
                if (ImGui::ColorEdit3("Color", glm::value_ptr(mLightingUniforms.colors[0])) ||
                    ImGui::DragFloat3("Direction", glm::value_ptr(mLightingUniforms.directions[0]), 0.1, -1.0, 1.0)) {
                    wgpuQueueWriteBuffer(mRendererResource.queue, mDirectionalLightBuffer.getBuffer(), 0,
                                         &mLightingUniforms, sizeof(LightingUniforms));
                }
                static glm::vec3 new_ligth_position = {};
                ImGui::InputFloat3("create new light at:", glm::value_ptr(new_ligth_position));
                ImGui::SliderFloat("frustum split factor", &middle_plane_length, 1.0, 100);
                ImGui::SliderFloat("far split factor", &far_plane_length, 1.0, 200);
                ImGui::SliderFloat("visualizer", &mUniforms.time, -1.0, 1.0);
                if (ImGui::InputInt("near far frstum", (int*)&which_frustum)) {
                    /*auto corners = getFrustumCornersWorldSpace(mCamera.getProjection(), mCamera.getView());*/
                }
                ImGui::Text("Cascade number #%zu", which_frustum);

                ImGui::Checkbox("shoud update csm", &should_update_csm);

                mShadowPass->lightPos = mLightingUniforms.directions[0];

                ImGui::Text("    ");
                ImGui::Text("Point Lights");

                mLightManager->renderGUI();
            }
            ImGui::Text("\nClip Plane");
            if (ImGui::DragFloat4("clip plane:", glm::value_ptr(mDefaultPlane))) {
                wgpuQueueWriteBuffer(mRendererResource.queue, mDefaultClipPlaneBuf.getBuffer(), 0,
                                     glm::value_ptr(mDefaultPlane), sizeof(glm::vec4));
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Objects")) {
            ImGui::BeginChild("ScrollableList", ImVec2(0, 200),
                              ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

            for (const auto& item : ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User)) {
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
                }

                ImGui::PopID();  // Pop the unique ID for this item
            }
            ImGui::EndChild();  // End the scrollable list

            if (mSelectedModel) {
                ImGui::Text("Model: %s, Meshes: %zu", mSelectedModel->getName().c_str(),
                            mSelectedModel->mMeshes.size());

#ifdef DEVELOPMENT_BUILD
                mSelectedModel->userInterface();
#endif  // DEVELOPMENT_BUILD
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Begin("Add Line");

    ImGui::Image((ImTextureID)(intptr_t)mWaterRefractionPass->mRenderTargetView, ImVec2(1920 / 4.0, 1022 / 4.0));
    ImGui::Image((ImTextureID)(intptr_t)mWaterPass->mRenderTargetView, ImVec2(1920 / 4.0, 1022 / 4.0));

    static glm::vec3 start = glm::vec3{0.0f};
    static glm::vec3 end = glm::vec3{0.0f};
    static glm::vec3 color = glm::vec3{0.0f};

    ImGui::InputFloat3("Start", glm::value_ptr(start));
    ImGui::InputFloat3("End", glm::value_ptr(end));

    ImGui::ColorPicker3("lines color", glm::value_ptr(color));
    if (ImGui::Button("Create", ImVec2(100, 30))) {
        // auto& item = ModelRegistry::instance().getLoadedModel(Visibility_User);
        // auto human = iter.find("human");
        // auto sheep = iter.find("sheep");
        // // auto sphere = iter.find("sphere");
        // if (human != iter.end()) {
        //     human->second->mAnimationSecond = std::fmod(time, human->second->mAnimationDuration) * 1000.0f;
        //     human->second->ExtractBonePositions();
        //     // loadSphereAtHumanBones(this, human->second, sphere->second);
        //     // wgpuQueueWriteBuffer(mRendererResource.queue, mDefaultBoneFinalTransformData.getBuffer(), 0 *
        //     // sizeof(glm::mat4),
        //     //                      human->second->mFinalTransformations.data(), 100 * sizeof(glm::mat4));
        // }
        // if (sheep != iter.end()) {
        //     sheep->second->mAnimationSecond = std::fmod(time, sheep->second->mAnimationDuration) * 1000.0f;
        //     sheep->second->ExtractBonePositions();
        //     // loadSphereAtHumanBones(this, human->second, sphere->second);
        //     // wgpuQueueWriteBuffer(mRendererResource.queue, mDefaultBoneFinalTransformData.getBuffer(), 0 *
        //     // sizeof(glm::mat4),
        //     //                      human->second->mFinalTransformations.data(), 100 * sizeof(glm::mat4));
        std::cout << "Line got created !";
        // mLines.emplace_back(new Line(this, start, end, 100.0, color));
    }
    if (ImGui::Button("remove frustum", ImVec2(100, 30))) {
        // for (int i = 0; i < 24; i++) {
        //     mLoadedModel.pop_back();
        // };
    }
    ImGui::SliderFloat("distance", &ddistance, 0.0, 30.0);
    ImGui::SliderFloat("dd", &dd, 0.0, 120.0);
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

Buffer& Application::getUniformBuffer() { return mUniformBuffer; }

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

WGPUTextureView Application::getDepthStencilTarget() { return mDepthTextureView; }
WGPUTextureView Application::getColorTarget() { return mCurrentTargetView; }
