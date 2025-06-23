#include "shadow_pass.h"

#include <cstdint>
#include <cstring>
#include <format>

#include "application.h"
#include "glm/ext.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "gpu_buffer.h"
#include "model.h"
#include "renderpass.h"
#include "webgpu.h"

bool should = false;

ShadowPass::ShadowPass(Application* app) { mApp = app; }

ShadowFrustum::ShadowFrustum(Application* app, size_t width, size_t height, Texture* renderTarget)
    : mRenderTarget(renderTarget), mApp(app), mWidth(width), mHeight(height) {
    if (mRenderTarget == nullptr) {
        mRenderTarget = new Texture{mApp->getRendererResource().device, static_cast<uint32_t>(mWidth),
                                    static_cast<uint32_t>(mHeight), TextureDimension::TEX_2D,
                                    WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment};
        mRenderTarget->createView();
    }

    mShadowDepthTexture = new Texture{mApp->getRendererResource().device,
                                      static_cast<uint32_t>(mWidth),
                                      static_cast<uint32_t>(mHeight),
                                      TextureDimension::TEX_2D,
                                      WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding,
                                      WGPUTextureFormat_Depth24Plus,
                                      2};
    mShadowDepthTexture->createViewArray(0, 2);
    mShadowDepthTexture->createViewDepthOnly(0, 1);

    mColorAttachment = ColorAttachment{mRenderTarget->getTextureView(), nullptr, WGPUColor{0.02, 0.80, 0.92, 1.0},
                                       StoreOp::Discard, LoadOp::Load};

    mRenderPassDesc = {};
    mRenderPassDesc.nextInChain = nullptr;
    mRenderPassDesc.colorAttachmentCount = 1;
    mRenderPassDesc.colorAttachments = mColorAttachment.get();

    mDepthStencilAttachment = DepthStencilAttachment{mShadowDepthTexture->getTextureView(),
                                                     StoreOp::Store,
                                                     LoadOp::Clear,
                                                     false,
                                                     StoreOp::Discard,
                                                     LoadOp::Clear,
                                                     true};

    mRenderPassDesc.depthStencilAttachment = mDepthStencilAttachment.get();
    mRenderPassDesc.timestampWrites = nullptr;
}

void ShadowPass::createRenderPass(WGPUTextureFormat textureFormat) {
    // creating pipeline
    /*createRenderPassDescriptor();*/
    /*createRenderPassDescriptor2();*/
    mNearFrustum = new ShadowFrustum{mApp, 2048, 2048};
    mFarFrustum = new ShadowFrustum{mApp, 2048, 2048};
    mFarFrustum->mDepthStencilAttachment =
        DepthStencilAttachment{mNearFrustum->mShadowDepthTexture->createViewDepthOnly2(1, 1),
                               StoreOp::Store,
                               LoadOp::Clear,
                               false,
                               StoreOp::Discard,
                               LoadOp::Clear,
                               true};

    /*mNearFrustum->mShadowDepthTexture->createViewDepthOnly();*/
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

    auto bind_group_layout = mBindingGroup.createLayout(mApp, "shadow pass pipeline");
    auto texture_bind_group_layout = mTextureBindingGroup.createLayout(mApp, "shadow pass pipeline");

    mRenderPipeline = new Pipeline{mApp, {bind_group_layout, texture_bind_group_layout}};
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
    mFragmentState.entryPoint = nullptr;
    mFragmentState.constants = nullptr;
    mFragmentState.constantCount = 0;
    mFragmentState.targetCount = 0;
    mFragmentState.targets = nullptr;
    (void)mFragmentState;

    // fill bindgroup
    mBindingData.reserve(5);

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

    mTextureBindingData[0] = {};
    mTextureBindingData[0].nextInChain = nullptr;
    mTextureBindingData[0].binding = 0;
    mTextureBindingData[0].textureView = nullptr;

    mTextureBindingData[1] = {};
    mTextureBindingData[1].nextInChain = nullptr;
    mTextureBindingData[1].binding = 1;
    mTextureBindingData[1].textureView = nullptr;

    mRenderPipeline->setShader(RESOURCE_DIR "/shadow.wgsl")
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
    maxZ += 20.0f;
    minZ -= 20.0f;
    /*if (std::strcmp(name, "Near") == 0) {*/
    /*    std::cout << name << " minZ: " << minZ << "  maxZ: " << maxZ << std::endl;*/
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

Scene ShadowPass::calculateFrustumScene(const std::vector<glm::vec4> frustum, float farZ) {
    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : frustum) {
        center += glm::vec3(v);
    }
    center /= frustum.size();

    glm::vec3 lightDirection = glm::normalize(-this->lightPos);
    glm::vec3 lightPosition =
        center - lightDirection * (glm::length(frustum[0] - frustum[7]) / 5.0f);  // Push light back

    auto view = glm::lookAt(lightPosition, center, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 projection = createProjectionFromFrustumCorner(frustum, view, &MinZ, "frustum", 0.0);
    return Scene{projection, glm::mat4{1.0}, view, farZ};
}

std::vector<Scene> ShadowPass::createFrustumSplits(std::vector<glm::vec4>& corners, std::vector<FrustumParams> params) {
    mScenes.clear();
    for (const auto& param : params) {
        mScenes.emplace_back(calculateFrustumScene(calculateSplit(corners, param.begin, param.end), param.end));
    }

    return mScenes;
}

Pipeline* ShadowPass::getPipeline() { return mRenderPipeline; }

WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor() { return mNearFrustum->getRenderPassDescriptor(); }
WGPURenderPassDescriptor* ShadowPass::getRenderPassDescriptor2() { return mFarFrustum->getRenderPassDescriptor(); }

WGPURenderPassDescriptor* ShadowFrustum::getRenderPassDescriptor() { return &mRenderPassDesc; }

void ShadowPass::render(std::vector<BaseModel*> models, WGPURenderPassEncoder encoder, size_t which) {
    /*auto& render_resource = mApp->getRendererResource();*/
    for (auto* model : models) {
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

            wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindgroup, 0, nullptr);
            wgpuRenderPassEncoderSetBindGroup(encoder, 1,
                                              mesh.mTextureBindGroup == nullptr
                                                  ? mApp->mDefaultTextureBindingGroup.getBindGroup()
                                                  : mesh.mTextureBindGroup,
                                              0, nullptr);

            wgpuRenderPassEncoderDraw(encoder, mesh.mVertexData.size(),
                                      model->instance == nullptr ? 1 : model->instance->getInstanceCount(), 0, 0);

            wgpuBufferRelease(modelUniformBuffer.getBuffer());
            wgpuBindGroupRelease(bindgroup);
        }
    }
}

std::vector<Scene>& ShadowPass::getScene() { return mScenes; }

/*WGPUTextureView ShadowFrustum::getShadowMapView() { return mShadowDepthTexture->getTextureView(); }*/
/*WGPUTextureView ShadowFrustum::getShadowMapViewArray() { return mShadowDepthTexture->getTextureViewArray(); }*/

WGPUTextureView ShadowPass::getShadowMapView() { return mNearFrustum->mShadowDepthTexture->getTextureView(); }
WGPUTextureView ShadowPass::getShadowMapView2() { return mFarFrustum->mShadowDepthTexture->getTextureView(); }

void printMatrix(const glm::mat4& matrix) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            std::cout << matrix[row][col] << " ";
        }
        std::cout << std::endl;
    }
}
