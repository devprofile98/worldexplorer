
#include "particle_system.h"

#include <cstdlib>
#include <glm/fwd.hpp>
#include <random>
#include <string>
#include <vector>

#include "animation.h"
#include "application.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "linegroup.h"
#include "shapes.h"
#include "webgpu/webgpu.h"

namespace {

constexpr const int NR_PARTICLES = 500;
bool which_tile = true;

static const std::array<float, 30> mLineInstance = {-1.0f, -1.0f, 0.0, /* bottom left*/ 0.0f,    0.0f,    //
                                                    1.0f,  -1.0f, 0.0, /* bottom right*/ 0.333f, 0.0f,    //
                                                    1.0f,  1.0f,  0.0, /* top right*/ 0.333f,    0.333f,  //
                                                    -1.0f, -1.0f, 0.0, /*bottom left*/ 0.0f,     0.0f,    //
                                                    1.0f,  1.0f,  0.0, /* top rigth*/ 0.333f,    0.333f,  //
                                                    -1.0f, 1.0f,  0.0, /*top left*/ 0.0f,        0.333f};

static const std::array<float, 30> mLineInstance2 = {-1.0f, -1.0f, 0.0, /* bottom left*/ 0.333f,  0.0f,    //
                                                     1.0f,  -1.0f, 0.0, /* bottom right*/ 0.666f, 0.0f,    //
                                                     1.0f,  1.0f,  0.0, /* top right*/ 0.666f,    0.333f,  //
                                                     -1.0f, -1.0f, 0.0, /*bottom left*/ 0.333f,   0.0f,    //
                                                     1.0f,  1.0f,  0.0, /* top rigth*/ 0.666f,    0.333f,  //
                                                     -1.0f, 1.0f,  0.0, /*top left*/ 0.333f,      0.333f};

std::vector<uint8_t> GenerateSoftCircleTexture(int size) {
    std::vector<uint8_t> pixels(size * size * 4);
    glm::vec2 center(size / 2.0f);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dist = glm::length(glm::vec2(x, y) - center) / (size / 2.0f);
            float alpha = glm::clamp(1.0f - dist, 0.0f, 1.0f);
            alpha = glm::pow(alpha, 1.5f);  // sharpen falloff slightly

            int idx = (y * size + x) * 4;
            pixels[idx + 0] = 255;
            pixels[idx + 1] = 255;
            pixels[idx + 2] = 255;
            pixels[idx + 3] = (uint8_t)(alpha * 255);
        }
    }
    return pixels;
}
}  // namespace

ParticleSystem::ParticleSystem(Application* app, const std::string& name, const ParticleSystemSetting& settings)
    : mApp(app), mName(name), mSettings(settings) {
    auto& resource = mApp->getRendererResource();

    mInstancingBuffer.setSize(sizeof(ParticleData) * NR_PARTICLES)
        .setLabel("Particle system instancing buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage)
        .setMappedAtCraetion()
        .create(&resource);

    mRenderPass = new NewRenderPass{"Particle system pass"};
    mRenderPass->setColorAttachment(
        {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
    mRenderPass->setDepthStencilAttachment(
        {mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false, StoreOp::Undefined, LoadOp::Undefined, false});
    mRenderPass->init();

    WGPUBindGroupLayout bind_group_layout =
        mBindGroup
            .addBuffer(0, BindGroupEntryVisibility::VERTEX_FRAGMENT, BufferBindingType::UNIFORM,
                       sizeof(CameraInfo) * 10)
            .addBuffer(1, BindGroupEntryVisibility::VERTEX, BufferBindingType::STORAGE_READONLY,
                       mInstancingBuffer.getBufferSize())
            .addSampler(2, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering)
            .addTexture(3, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::VIEW_2D)
            .createLayout(resource, "particle system pass");

    mPipeline = new Pipeline{app, {bind_group_layout}, "Particle system pipeline"};

    mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
        .addAttribute(sizeof(glm::vec3), 1, WGPUVertexFormat_Float32x2)
        .configure(sizeof(glm::vec3) + sizeof(glm::vec2), VertexStepMode::VERTEX);

    WGPUBlendState blend_state = {};
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_One;  // changed
    blend_state.color.operation = WGPUBlendOperation_Add;
    blend_state.alpha.srcFactor = WGPUBlendFactor_SrcAlpha;  // changed
    blend_state.alpha.dstFactor = WGPUBlendFactor_One;       // changed
    blend_state.alpha.operation = WGPUBlendOperation_Add;

    mPipeline->setVertexBufferLayout(mVertexBufferLayout.getLayout())
        .setShader(mApp->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / "shaders/particle_system.wgsl", resource)
        .setVertexState()
        .setBlendState(blend_state)
        .setPrimitiveState()
        .setColorTargetState(WGPUTextureFormat_BGRA8UnormSrgb)
        // .setColorTargetState(WGPUTextureFormat_RGBA16Float)
        .setDepthStencilState(false, 0xFF, 0xFF, WGPUTextureFormat_Depth24PlusStencil8)
        .setFragmentState()
        .createPipeline(resource);

    mVertexBuffer.setSize(sizeof(mLineInstance))
        .setLabel("A Simple Reactangle")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(&resource);

    mVertexBuffer.queueWrite(0, mLineInstance.data(), sizeof(mLineInstance));

    mVertexBuffer2.setSize(sizeof(mLineInstance))
        .setLabel("A Simple Reactangle2")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setMappedAtCraetion()
        .create(&resource);

    mVertexBuffer2.queueWrite(0, mLineInstance2.data(), sizeof(mLineInstance2));

    mBindingData.push_back({});
    mBindingData.push_back({});
    mBindingData.reserve(4);
    mBindingData.resize(4);
    mBindingData[0] = {};
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].buffer = mApp->getUniformBuffer().getBuffer();
    mBindingData[0].binding = 0;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(CameraInfo) * 10;

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].buffer = mInstancingBuffer.getBuffer();
    mBindingData[1].binding = 1;
    mBindingData[1].offset = 0;
    mBindingData[1].size = mInstancingBuffer.getBufferSize();

    mBindingData[2] = {};
    mBindingData[2].binding = 2;
    mBindingData[2].sampler = mApp->mDefaultSampler;

    // creating circle fire texture
    auto& rc = mApp->getRendererResource();
    // auto fire_texture = std::make_shared<Texture>(rc.device, 200, 200, TextureDimension::TEX_2D);
    // WGPUTextureView _ = fire_texture->createView();
    //
    // fire_texture->getBuffer(0) = GenerateSoftCircleTexture(200);
    // fire_texture->uploadToGPU(rc.queue);
    // mApp->mTextureRegistery->addToRegistery("fire texture", fire_texture);

    std::shared_ptr<Texture> fire_texture = nullptr;
    std::string path = "rc://explosion_atlas.png";
    fire_texture =
        Texture::asyncLoadTexture(app->mTextureRegistery, rc, normalizePath(mApp, path).string(), "fire-atlas");

    mBindingData[3] = {};
    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].textureView = fire_texture->getTextureView();

    mBindGroup.create(resource, mBindingData);

    for (size_t i = 0; i < NR_PARTICLES; ++i) {
        mParticles.push_back({});
    }
    //
    mParticlesData.reserve(NR_PARTICLES);
    mParticlesData.resize(NR_PARTICLES);
    //
    glm::vec3 colorlist[]{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {0.5, 0.5, 0.0}};

    for (size_t i = 0; i < NR_PARTICLES; ++i) {
        auto translation = glm::translate(glm::mat4{1.0f}, mParticles[i].position);
        auto rotation = glm::normalize(glm::quat(mParticles[i].rotate));
        auto scale = glm::scale(glm::mat4{1.0}, glm::vec3{0.05});
        mParticlesData[i].transformation = translation * glm::mat4_cast(rotation) * scale;

        auto color = ((rand() % 4));

        mParticlesData[i].props = glm::vec4{colorlist[color], 0.0};
    }
    mInstancingBuffer.queueWrite(0, mParticlesData.data(), mParticlesData.size() * sizeof(ParticleData));
}

const std::string_view ParticleSystem::getName() const { return mName; }

void ParticleSystem::setActive(bool active) { mActive = active; }

bool ParticleSystem::isActive() const { return mActive; }

size_t ParticleSystem::lastAddedParticle() {
    lastAddedIdx = lastAddedIdx % NR_PARTICLES;

    if (lastAddedIdx < NR_PARTICLES && mParticles[lastAddedIdx].life < 0.0f) {
        return lastAddedIdx;
    }
    if ((lastAddedIdx + 1 < NR_PARTICLES) && mParticles[lastAddedIdx + 1].life < 0.0f) {
        return lastAddedIdx + 1;
    }
    for (size_t i = 0; i < NR_PARTICLES; ++i) {
        if (mParticles[i].life < 0.0) {
            lastAddedIdx = i;
            return lastAddedIdx;
        }
    }

    return 0;
}

void ParticleSystem::updateParticleSystem(float dt, bool simulate) {
    mEmitterSocket->calculateTransform();

    auto trans = mEmitterSocket->update();
    auto [t, s, r] = decomposeTransformation(trans);

    getSettings().setSpeedDiretcion(glm::vec3{glm::toMat4(r) * glm::vec4{0.0, 0.0, 1.0, 1.0}});

    if (simulate) {
        lastAddedIdx = lastAddedParticle();
        if (isActive()) {
            for (size_t i = 0; i < getSettings().ParticlePerFrame; i++) {
                lastAddedIdx++;
                lastAddedIdx %= NR_PARTICLES;
                mParticles[lastAddedIdx] = mSettings.spawnCallback(this, t);
            }
        }
        simulateParticles(dt);
    }
}

void ParticleSystem::executePass() {
    {
        mRenderPass->setColorAttachment(
            {mApp->mCurrentTargetView, nullptr, WGPUColor{0.52, 0.80, 0.92, 1.0}, StoreOp::Store, LoadOp::Load});
        mRenderPass->setDepthStencilAttachment({mApp->mDepthTextureView, StoreOp::Store, LoadOp::Load, false,
                                                StoreOp::Undefined, LoadOp::Undefined, false});
        mRenderPass->init();

        WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(
            mApp->getRendererResource().commandEncoder, &mRenderPass->mRenderPassDesc);
        draw(render_pass_encoder);
        wgpuRenderPassEncoderEnd(render_pass_encoder);
        wgpuRenderPassEncoderRelease(render_pass_encoder);
    }
}

void ParticleSystem::simulateParticles(float dt) {
    mSettings.simulationCallback(this, dt, mParticles, mParticlesData);
    mInstancingBuffer.queueWrite(0, mParticlesData.data(), mParticlesData.size() * sizeof(ParticleData));
}

void ParticleSystem::draw(WGPURenderPassEncoder encoder) {
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindGroup.getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetPipeline(encoder, mPipeline->getPipeline());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mVertexBuffer.getBuffer(), 0, sizeof(mLineInstance));
    wgpuRenderPassEncoderDraw(encoder, 6, NR_PARTICLES, 0, 0);
}

void ParticleSystem::setEmitterSocket(BoneSocket* emitterSocket) { mEmitterSocket = emitterSocket; }

BoneSocket* ParticleSystem::getEmitterSocket() { return mEmitterSocket; }

ParticleSystemSetting::ParticleSystemSetting(const glm::vec3& speedDirVector, EmitterCallback emitterCb,
                                             SimulationCallback simCb, const glm::vec2& speedRange,
                                             const glm::vec2& scaleRange, float lifeTime, float particleSize, int ppf)
    : mExitVector(speedDirVector),
      speedRange(speedRange),
      scaleRange(scaleRange),
      mLifeTime(lifeTime),
      mSize(particleSize),
      ParticlePerFrame(ppf),
      spawnCallback(emitterCb),
      simulationCallback(simCb)

{}

ParticleSystemSetting& ParticleSystem::getSettings() { return mSettings; }

void ParticleSystemSetting::setSpeedDiretcion(const glm::vec3& speedDir) { mExitVector = speedDir; }

glm::vec3& ParticleSystemSetting::getSpeedDirection() { return mExitVector; }

void ParticleSystemsManager::userInterface(Application* app) {
    static LineGroup particleSocketLines =
        app->mLineEngine->create(generateBox(), glm::scale(glm::mat4{1.0}, glm::vec3{0.1, 0.1, 0.0f}), {1.0, 0.0, 0.0})
            .updateVisibility(true);
    static LineGroup line = app->mLineEngine
                                ->create(generateBox({0, 0, .5}, {1.0, 1.0, 1.0}),
                                         glm::scale(glm::mat4{1.0}, glm::vec3{0.0, 0.0, 0.1f}), {0.0, 0.0, 1.0})
                                .updateVisibility(true);

    if (ImGui::BeginTabItem("Particle System")) {
        ImGui::Checkbox("which tile", &which_tile);
        for (const auto& system : mParticleSystems) {
            auto* socket = system->getEmitterSocket();
            if (socket == nullptr) {
                continue;
            }

            ImGui::PushID((void*)system);
            ImGui::Text("%s", system->getName().data());
            bool happend = false;
            if (ImGui::DragFloat3("Emitter Pos", glm::value_ptr(socket->positionOffset), 0.01)) {
                happend = true;
            }

            static glm::vec3 euler{0.0f};
            if (ImGui::DragFloat3("Emitter Rot", glm::value_ptr(euler), 0.01)) {
                happend = true;
                std::cout << glm::to_string(socket->rotationOffset) << std::endl;
                socket->rotationOffset = glm::normalize(glm::quat(glm::radians(euler)));
            }

            auto& settings = system->getSettings();
            if (happend) {
                socket->calculateTransform();

                auto trans = socket->update();
                auto [t, s, r] = decomposeTransformation(trans);
                particleSocketLines.updateTransformation(glm::translate(glm::mat4{1.0}, t) * glm::toMat4(r) *
                                                         glm::scale(glm::mat4{1.0}, glm::vec3{0.1f, 0.1f, 0.0f}));
                auto newtrans = glm::translate(glm::mat4{1.0}, t) * glm::toMat4(r) *
                                glm::scale(glm::mat4{1.0}, glm::vec3{0.0f, 0.0f, 0.1f});
                line.updateTransformation(newtrans);
                settings.setSpeedDiretcion(glm::vec3{glm::toMat4(r) * glm::vec4{0.0, 0.0, 1.0, 1.0}});
            }
            ImGui::DragFloat3("Exit Dir", glm::value_ptr(settings.getSpeedDirection()), 0.01);
            ImGui::DragFloat2("Speed", glm::value_ptr(settings.speedRange), 0.01);
            ImGui::DragFloat2("Scale", glm::value_ptr(settings.scaleRange),
                              ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 0.001 : 0.01);
            ImGui::DragFloat2("uvoffset", glm::value_ptr(settings.uvoffset),
                              ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 0.001 : 0.01);
            if (settings.speedRange.y < settings.speedRange.x) {
                settings.speedRange.y = settings.speedRange.x;
            }

            if (settings.scaleRange.y < settings.scaleRange.x) {
                settings.scaleRange.y = settings.scaleRange.x;
            }

            ImGui::DragFloat("Life Time", &settings.mLifeTime, 0.01);
            ImGui::DragFloat("Particle Size", &settings.mSize, 0.01);
            ImGui::DragInt("ppr", &settings.ParticlePerFrame, 1, 0, 50);

            ImGui::PopID();
        }
        ImGui::EndTabItem();
    }
}

bool ParticleSystemsManager::registerSystem(ParticleSystem* system) {
    mParticleSystems.push_back(system);
    return true;
}

void ParticleSystemsManager::run(float dt) {
    for (const auto& system : mParticleSystems) {
        system->updateParticleSystem(dt, true);
        system->executePass();
    }
}
