#include "shadow_pass.h"

#include <cstdint>
#include <cstring>
#include <format>
#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "application.h"
#include "glm/ext.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "gpu_buffer.h"
#include "model.h"
#include "renderpass.h"

float sunlength = 5.0;

extern const char* model_name;

ShadowPass::ShadowPass(Application* app) { mApp = app; }

ShadowFrustum::ShadowFrustum(Application* app, WGPUTextureView renderTarget, WGPUTextureView depthTexture)
    : mShadowDepthTexture(depthTexture), mRenderTarget(renderTarget), mApp(app) {
    mColorAttachment =
        ColorAttachment{mRenderTarget, nullptr, WGPUColor{0.02, 0.80, 0.92, 1.0}, StoreOp::Discard, LoadOp::Load};

    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = mColorAttachment.get();

    mDepthStencilAttachment = DepthStencilAttachment{mShadowDepthTexture, StoreOp::Store, LoadOp::Clear, false,
                                                     StoreOp::Discard,    LoadOp::Clear,  true};

    mRenderPassDesc.depthStencilAttachment = mDepthStencilAttachment.get();
    mRenderPassDesc.timestampWrites = nullptr;
}

void ShadowPass::createRenderPass(WGPUTextureFormat textureFormat) { (void)textureFormat; }
void ShadowPass::createRenderPass(WGPUTextureFormat textureFormat, size_t cascadeNumber) {
    mNumOfCascades = cascadeNumber;
    mRenderTarget =
        new Texture{mApp->getRendererResource().device, static_cast<uint32_t>(2048), static_cast<uint32_t>(2048),
                    TextureDimension::TEX_2D, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment};
    mRenderTarget->createView();

    mShadowDepthTexture = new Texture{mApp->getRendererResource().device,
                                      static_cast<uint32_t>(2048),
                                      static_cast<uint32_t>(2048),
                                      TextureDimension::TEX_2D,
                                      WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                      WGPUTextureFormat_Depth24Plus,
                                      5};

    mShadowDepthTexture->createViewArray(0, 5);
    for (size_t c = 0; c < mNumOfCascades; c++) {
        mSubFrustums.push_back(
            new ShadowFrustum{mApp, mRenderTarget->getTextureView(), mShadowDepthTexture->createViewDepthOnly2(c, 1)});
    }
    // for projection
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(Scene));
    mBindingGroup.addBuffer(1,  //
                            BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                            mApp->mInstanceManager->mBufferSize);

    mBindingGroup.addBuffer(2, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                            sizeof(ObjectInfo));

    mBindingGroup.addSampler(3,  //
                             BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);

    mTextureBindingGroup.addTexture(0,  //
                                    BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);

    mTextureBindingGroup.addTexture(1,  //
                                    BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);
    mTextureBindingGroup.addTexture(2,  //
                                    BindGroupEntryVisibility::VERTEX_FRAGMENT, TextureSampleType::FLAOT,
                                    TextureViewDimension::VIEW_2D);

    mVisibleBindingGroup.addBuffer(0,  //
                                   BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                                   sizeof(uint32_t) * 100'000 * 5);

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");
    auto texture_bind_group_layout = mTextureBindingGroup.createLayout(mApp, "shadow pass pipeline");
    auto visible_bind_group_layout = mVisibleBindingGroup.createLayout(mApp, "visible indices pipeline");

    mRenderPipeline = new Pipeline{mApp,
                                   {bind_group_layout, texture_bind_group_layout, mApp->getBindGroupLayouts()[5]},
                                   "shadow pass pipeline layout"};

    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout
                                   .addAttribute(0, 0, WGPUVertexFormat_Float32x3)  // for position
                                   .addAttribute(3 * sizeof(float), 1, WGPUVertexFormat_Float32x3)
                                   .addAttribute(6 * sizeof(float), 2, WGPUVertexFormat_Float32x3)
                                   .addAttribute(9 * sizeof(float), 3, WGPUVertexFormat_Float32x3)
                                   .addAttribute(12 * sizeof(float), 4, WGPUVertexFormat_Float32x3)
                                   .addAttribute(offsetof(VertexAttributes, uv), 5, WGPUVertexFormat_Float32x2)
                                   .configure(sizeof(VertexAttributes), VertexStepMode::VERTEX);

    static WGPUFragmentState mFragmentState = {};
    mFragmentState.nextInChain = nullptr;
    mFragmentState.module = nullptr;
    mFragmentState.entryPoint = WGPUStringView{nullptr, 0};
    mFragmentState.constants = nullptr;
    mFragmentState.constantCount = 0;
    mFragmentState.targetCount = 0;
    mFragmentState.targets = nullptr;
    (void)mFragmentState;

    // fill bindgroup
    mBindingData.reserve(5);
    mBindingData.resize(5);

    // mBindingData.push_back(entry);
    mBindingData[0] = {};
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = nullptr;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(Scene);

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = mApp->mInstanceManager->mBufferSize;

    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].buffer = nullptr;
    mBindingData[2].offset = 0;
    mBindingData[2].size = sizeof(ObjectInfo);

    mBindingData[3] = {};
    mBindingData[3].binding = 3;
    mBindingData[3].sampler = mApp->getDefaultSampler();

    mBindingData[4] = {};
    mBindingData[4].nextInChain = nullptr;
    mBindingData[4].buffer = mApp->mVisibleIndexBuffer.getBuffer();
    mBindingData[4].binding = 4;
    mBindingData[4].offset = 0;
    mBindingData[4].size = sizeof(uint32_t) * 100'000 * 5;

    mTextureBindingData[0] = {};
    mTextureBindingData[0].nextInChain = nullptr;
    mTextureBindingData[0].binding = 0;
    mTextureBindingData[0].textureView = nullptr;

    mTextureBindingData[1] = {};
    mTextureBindingData[1].nextInChain = nullptr;
    mTextureBindingData[1].binding = 1;
    mTextureBindingData[1].textureView = nullptr;

    mRenderPipeline->setShader(RESOURCE_DIR "/shaders/shadow.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState()
        .setPrimitiveState()
        .setDepthStencilState(true, 0xFF, 0xFF)
        .setBlendState()
        .setColorTargetState(textureFormat)
        .setFragmentState();

    /*mRenderPipeline->getDescriptorPtr()->fragment = nullptr;*/

    mRenderPipeline->setMultiSampleState().createPipeline(mApp);

    // creating and writing to buffer
    mSceneUniformBuffer.setLabel("shadow map pass scene buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(Scene))
        .setMappedAtCraetion()
        .create(mApp);
}

glm::mat4 createProjectionFromFrustumCorner(const std::vector<glm::vec4>& corners, const glm::mat4& lightView,
                                            float* mm, const char* name, float dis) {
    (void)name;
    (void)dis;
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners) {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    /*constexpr float zMult = 10.0f;*/
    /*if (minZ < 0) {*/
    /*    minZ *= zMult;*/
    /*} else {*/
    /*    minZ /= zMult;*/
    /*}*/
    /*if (maxZ < 0) {*/
    /*    maxZ /= zMult;*/
    /*} else {*/
    /*    maxZ *= zMult;*/
    /*}*/
    /*if (should) {*/
    maxZ += 20.0f * dis;
    minZ -= 20.0f * dis;
    /*if (std::strcmp(name, "Near") == 0) {*/
    /*}*/

    *mm = minZ;
    return glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
}

std::vector<glm::vec4> calculateSplit(std::vector<glm::vec4>& corners, float begin, float end) {
    auto near0 = corners[0] + (glm::normalize(corners[1] - corners[0]) * begin);
    auto near1 = corners[2] + (glm::normalize(corners[3] - corners[2]) * begin);
    auto near2 = corners[4] + (glm::normalize(corners[5] - corners[4]) * begin);
    auto near3 = corners[6] + (glm::normalize(corners[7] - corners[6]) * begin);

    auto far0 = corners[0] + (glm::normalize(corners[1] - corners[0]) * end);
    auto far1 = corners[2] + (glm::normalize(corners[3] - corners[2]) * end);
    auto far2 = corners[4] + (glm::normalize(corners[5] - corners[4]) * end);
    auto far3 = corners[6] + (glm::normalize(corners[7] - corners[6]) * end);

    return {near0, far0, near1, far1, near2, far2, near3, far3};
}

Scene ShadowPass::calculateFrustumScene(const std::vector<glm::vec4> frustum, float farZ, size_t cascadeIdx) {
    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : frustum) {
        center += glm::vec3(v);
    }
    center /= frustum.size();

    glm::vec3 lightDirection = glm::normalize(-this->lightPos);
    glm::vec3 lightPosition =
        center - lightDirection * (glm::length(frustum[0] - frustum[7]) / 5.0f);  // Push light back

    auto view = glm::lookAt(lightPosition, center, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 projection = createProjectionFromFrustumCorner(frustum, view, &MinZ, "frustum", cascadeIdx);
    return Scene{projection, glm::mat4{1.0}, view, farZ};
}

std::vector<Scene> ShadowPass::createFrustumSplits(std::vector<glm::vec4>& corners, std::vector<FrustumParams> params) {
    mScenes.clear();
    size_t counter = 0;
    for (const auto& param : params) {
        mScenes.emplace_back(
            calculateFrustumScene(calculateSplit(corners, param.begin, param.end), param.end, ++counter));
    }

    return mScenes;
}

Pipeline* ShadowPass::getPipeline() { return mRenderPipeline; }

WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor(size_t index) {
    return mSubFrustums[index]->getRenderPassDescriptor();
}

WGPURenderPassDescriptor* ShadowFrustum::getRenderPassDescriptor() { return &mRenderPassDesc; }

void ShadowPass::renderAllCascades(WGPUCommandEncoder encoder) {
    for (size_t c = 0; c < mNumOfCascades; ++c) {
        WGPURenderPassEncoder shadow_pass_encoder =
            wgpuCommandEncoderBeginRenderPass(encoder, getRenderPassDescriptor(c));
        wgpuRenderPassEncoderSetPipeline(shadow_pass_encoder, getPipeline()->getPipeline());
        render(ModelRegistry::instance().getLoadedModel(ModelVisibility::Visibility_User), shadow_pass_encoder, c);
        wgpuRenderPassEncoderEnd(shadow_pass_encoder);
        wgpuRenderPassEncoderRelease(shadow_pass_encoder);
    }
}

void ShadowPass::render(ModelRegistry::ModelContainer& models, WGPURenderPassEncoder encoder, size_t which) {
    /*auto& render_resource = mApp->getRendererResource();*/
    for (auto [name, model] : models) {
        if (name == "water") {
            continue;
        }
        for (auto& [mat_id, mesh] : model->mMeshes) {
            Buffer modelUniformBuffer = {};
            modelUniformBuffer.setLabel("Model Uniform Buffer")
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .setSize(sizeof(Scene))
                .setMappedAtCraetion()
                .create(mApp);

            if (which >= mScenes.size()) {
                continue;
            };

            mScenes[which].model = model->getTranformMatrix();
            mBindingData[0].buffer = modelUniformBuffer.getBuffer();
            mBindingData[1].buffer = mApp->mInstanceManager->getInstancingBuffer().getBuffer();
            mBindingData[2].buffer = model->getUniformBuffer().getBuffer();
            mBindingData[3].sampler = mApp->getDefaultSampler();

            auto bindgroup = mBindingGroup.createNew(mApp, mBindingData);
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, modelUniformBuffer.getBuffer(), 0,
                                 &mScenes.at(which), sizeof(Scene));

            wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                                 wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));

            if (model->getName() != model_name) {
                wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh.mIndexBuffer.getBuffer(), WGPUIndexFormat_Uint32, 0,
                                                    wgpuBufferGetSize(mesh.mIndexBuffer.getBuffer()));
            }
            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(encoder, 1,
                                              mesh.mTextureBindGroup == nullptr
                                                  ? mApp->mDefaultTextureBindingGroup.getBindGroup()
                                                  : mesh.mTextureBindGroup,
                                              0, nullptr);

            wgpuRenderPassEncoderSetBindGroup(encoder, 2, mApp->mDefaultVisibleBuffer.getBindGroup(), 0, nullptr);

            if (model->instance == nullptr) {
                wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(),
                                                 model->instance == nullptr ? 1 : model->instance->getInstanceCount(),
                                                 0, 0, 0);
            } else {
                wgpuRenderPassEncoderDrawIndexedIndirect(encoder, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0);
            }

            wgpuBufferRelease(modelUniformBuffer.getBuffer());
            wgpuBindGroupRelease(bindgroup);
        }
    }
}

std::vector<Scene>& ShadowPass::getScene() { return mScenes; }

WGPUTextureView ShadowPass::getShadowMapView() { return mShadowDepthTexture->getTextureViewArray(); }

void printMatrix(const glm::mat4& matrix) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            std::cout << matrix[row][col] << " ";
        }
        std::cout << std::endl;
    }
}
