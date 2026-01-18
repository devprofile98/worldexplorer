#include "point_light.h"

#include <cstddef>
#include <cstdint>
#include <format>

#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "imgui.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "application.h"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "rendererResource.h"
#include "shapes.h"
#include "utils.h"

LightManager::LightManager(Application* app) {
    mApp = app;
    mLightCountBuffer.setLabel("light count buffer")
        .setSize(sizeof(uint32_t))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(mApp);
}

LightManager* LightManager::init(Application* app) {
    if (mLightInstance == nullptr) {
        mLightInstance = new LightManager{app};
        mLightInstance->mLights.reserve(10);
    }
    return mLightInstance;
}

LightManager* LightManager::getInstance() { return mLightInstance; }

Buffer& LightManager::getCountBuffer() { return mLightCountBuffer; }

void LightManager::createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin,
                                    float quad, const char* name) {
    Light light;
    light.mPosition = pos;
    light.mAmbient = amb;
    light.mDiffuse = diff;
    light.mSpecular = spec;
    light.mConstant = cons;
    light.mLinear = lin;
    light.mQuadratic = quad;
    light.type = POINT;

    mLights.emplace_back(std::move(light));
    mLightsNames.push_back(name);

    updateCount();
}

void LightManager::createSpotLight(glm::vec4 pos, glm::vec4 direction, glm::vec4 diff, float cutoff, float outerCutoff,
                                   float linear, float quadratic, const char* name) {
    (void)cutoff;
    (void)outerCutoff;
    Light light;
    light.mPosition = pos;
    light.mDirection = direction;
    light.mDiffuse = diff;
    light.mAmbient = diff;
    light.mInnerCutoff = cutoff;
    light.mOuterCutoff = outerCutoff;
    light.type = SPOT;
    light.mConstant = 1.0;
    light.mLinear = linear;
    light.mQuadratic = quadratic;

    mLights.push_back(light);
    mLightsNames.push_back(name);
    updateCount();
}

void LightManager::updateCount() {
    mLightCount = mLights.size();
    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mLightCountBuffer.getBuffer(), 0, &mLightCount,
                         sizeof(uint32_t));
}

Light* LightManager::get(size_t index) { return &mLights[index]; }

void LightManager::uploadToGpu(Application* app, WGPUBuffer buffer) {
    updateCount();
    wgpuQueueWriteBuffer(app->getRendererResource().queue, buffer, 0, mLights.data(), sizeof(Light) * mLights.size());
}

void LightManager::update() {
    auto* light = get(mSelectedLightInGui);

    glm::mat4 t{1.0};
    t = glm::translate(t, glm::vec3(light->mPosition));
    if (light->type == LightType::SPOT) {
        glm::quat rot = rotationBetweenVectors(glm::vec3{0.0, 0.0, 1.0}, light->mDirection);
        t = t * glm::toMat4(rot);
    }
    if (boxId < 1024) {
        mApp->mLineEngine->updateLineTransformation(boxId, t);
    } else {
        boxId = mApp->mLineEngine->addLines(generateCone(), t);
    }

    wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->mLightBuffer.getBuffer(),
                         sizeof(Light) * mSelectedLightInGui, light, sizeof(Light));
}

void LightManager::renderGUI() {
    auto* light = get(mSelectedLightInGui);
    bool changed = false;

    changed |= ImGui::ColorEdit3(
        std::format("{} color # {}", light->type == SPOT ? "Spot" : "Point", mSelectedLightInGui).c_str(),
        glm::value_ptr(light->mAmbient));

    changed |= ImGui::SliderFloat3("Position", glm::value_ptr(light->mPosition), -20.0f, 20.0f);

    if (light->type == SPOT) {
        changed |= ImGui::SliderFloat3("sDirection", glm::value_ptr(light->mDirection), -1.0, 1.0f);
        changed |= ImGui::SliderFloat("Inner Cutoff", &light->mInnerCutoff, 0, 2.0);
        changed |= ImGui::SliderFloat("outer Cutoff", &light->mOuterCutoff, 0, 2.0);
    }
    changed |= ImGui::SliderFloat("Linear", &light->mLinear, -10.0f, 10.0f);
    changed |= ImGui::SliderFloat("Quadratic", &light->mQuadratic, -10.0f, 10.0f);
    if (changed) {
        if (boxId < 1024) {
            glm::mat4 t{1.0};
            t = glm::translate(t, glm::vec3(light->mPosition));
            glm::quat rot = rotationBetweenVectors(glm::vec3{0.0, 0.0, 1.0}, light->mDirection);

            mApp->mLineEngine->updateLineTransformation(boxId, t * glm::toMat4(rot));
        } else {
            boxId = mApp->mLineEngine->addLines(generateCone());
        }
        wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->mLightBuffer.getBuffer(),
                             sizeof(Light) * mSelectedLightInGui, light, sizeof(Light));
    }

    ImGui::BeginChild("Light manager list", ImVec2(0, 200),
                      ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

    for (size_t i = 0; i < getLights().size(); i++) {
        auto* light = get(i);
        ImGui::PushID(i);
        if (ImGui::Selectable(mLightsNames[i].c_str(), light == &getLights()[mSelectedLightInGui])) {
            ImGui::Text("%s", glm::to_string(light->mPosition).c_str());
            mSelectedLightInGui = i;
            update();
        }
        ImGui::PopID();  // Pop the unique ID for this item
    }
    ImGui::EndChild();  // End the scrollable list

    static char new_light_name[32]{0};
    static bool is_spot = false;
    static bool is_point = false;
    ImGui::Checkbox("new spot light? ##1", &is_spot);
    ImGui::Checkbox("new point light? ##1", &is_point);
    if (is_spot || is_point) {
        ImGui::InputText("light name", new_light_name, 31);
        if (ImGui::Button("Create New Light")) {
            if (is_spot) {
                createSpotLight(glm::vec4{0.0, 0.0, 0.0, 1.0}, glm::vec4{0.0, 0.0, -1.0, 1.0},
                                glm::vec4{1.0, 0.0, 0.0, 1.0}, 0.837, 0.709, 0.08, 0.4, new_light_name);
            } else if (is_point) {
                createPointLight(glm::vec4{0.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0},
                                 glm::vec4{1.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0}, 1.0, -1.0, 1.8,
                                 new_light_name);
            }
        }
    }
}

void LightManager::nextLight() {
    mSelectedLightInGui = mSelectedLightInGui < mLights.size() - 1 ? mSelectedLightInGui + 1 : 0;
}

std::vector<Light>& LightManager::getLights() { return mLights; }
