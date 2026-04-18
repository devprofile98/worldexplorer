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
        .create(&mApp->getRendererResource());
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

size_t LightManager::createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons,
                                      float lin, float quad, float intensity, const char* name) {
    Light light;
    light.mPosition = pos;
    light.mAmbient = amb;
    light.mDiffuse = diff;
    light.mSpecular = spec;
    light.mConstant = cons;
    light.mLinear = lin;
    light.mQuadratic = quad;
    light.intensity = intensity;
    light.type = POINT;

    auto ret_idx = mLights.size();
    mLights.emplace_back(std::move(light));
    mLightsNames.push_back(name);

    updateCount();
    return ret_idx;
}

size_t LightManager::createSpotLight(glm::vec4 pos, glm::vec4 direction, glm::vec4 diff, float cutoff,
                                     float outerCutoff, float linear, float quadratic, float intensity,
                                     const char* name) {
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
    light.intensity = intensity;
    light.mQuadratic = quadratic;

    auto ret_idx = mLights.size();
    mLights.emplace_back(std::move(light));
    mLightsNames.push_back(name);
    updateCount();
    return ret_idx;
}

void LightManager::updateCount() {
    mLightCount = mLights.size();
    mLightCountBuffer.queueWrite(0, &mLightCount, sizeof(uint32_t));
}

Light* LightManager::get(size_t index) { return mLights.size() > 0 ? &mLights[index] : nullptr; }

void LightManager::uploadToGpu(Application* app, WGPUBuffer buffer) {
    updateCount();
    wgpuQueueWriteBuffer(app->getRendererResource().queue, buffer, 0, mLights.data(), sizeof(Light) * mLights.size());
}

void LightManager::update(int index, bool updateDebugLines) {
    auto* light = get(index);

    glm::mat4 t{1.0};
    t = glm::translate(t, glm::vec3(light->mPosition));
    if (light->type == LightType::SPOT) {
        glm::quat rot = rotationBetweenVectors(glm::vec3{0.0, 0.0, 1.0}, light->mDirection);
        t = t * glm::toMat4(rot);
    }
    mApp->mLightBuffer.queueWrite(sizeof(Light) * mSelectedLightInGui, light, sizeof(Light));

    if (updateDebugLines) {
        if (boxId < 1024) {
            mApp->mLineEngine->updateLineTransformation(boxId, t);
        } else {
            boxId = mApp->mLineEngine->addLines(generateCone(), t);
        }
    }
}

void LightManager::renderGUI() {
    auto* light = get(mSelectedLightInGui);
    if (light) {
    }
    bool changed = false;

    // -------------------------- Start 2 Columns -----------------------------------
    // -------------------------- First Column -----------------------------------
    ImGui::Columns(2, nullptr, true);

    ImGui::BeginChild("Light manager list", ImVec2(0, 200),
                      ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

    for (size_t i = 0; i < getLights().size(); i++) {
        auto* light = get(i);
        ImGui::PushID(i);
        if (ImGui::Selectable(mLightsNames[i].c_str(), light == &getLights()[mSelectedLightInGui])) {
            mSelectedLightInGui = i;
            update(mSelectedLightInGui);
        }
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
            };
            if (ImGui::MenuItem("Duplicate")) {
            };
            ImGui::EndPopup();
        }

        ImGui::PopID();  // Pop the unique ID for this item
    }
    ImGui::EndChild();  // End the scrollable list

    // -------------------------- Second Column -----------------------------------
    ImGui::NextColumn();

    if (light != nullptr) {
        auto speed = !ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 1.0f : 0.01f;
        changed |= ImGui::ColorEdit3("Color##env", glm::value_ptr(light->mAmbient));
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragFloat3("Position##env", glm::value_ptr(light->mPosition), speed);
            if (light->type == SPOT) {
                changed |= ImGui::DragFloat3("sDirection##env", glm::value_ptr(light->mDirection), speed, -1.0, 1.0f);
            }
        }
        if (ImGui::CollapsingHeader("Attenuation", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::DragFloat("Constant##env", &light->mConstant, speed);
            changed |= ImGui::DragFloat("Linear##env", &light->mLinear, speed);
            changed |= ImGui::DragFloat("Intensity##env", &light->intensity, speed);
            if (light->type == SPOT) {
                changed |= ImGui::DragFloat("Inner Cutoff##env", &light->mInnerCutoff, speed);
                changed |= ImGui::DragFloat("outer Cutoff##env", &light->mOuterCutoff, speed);
            }
            changed |= ImGui::DragFloat("Quadratic##env", &light->mQuadratic, speed);
        }
        if (changed) {
            if (boxId < 1024) {
                glm::mat4 t{1.0};
                t = glm::translate(t, glm::vec3(light->mPosition));
                glm::quat rot = rotationBetweenVectors(glm::vec3{0.0, 0.0, 1.0}, light->mDirection);

                mApp->mLineEngine->updateLineTransformation(boxId, t * glm::toMat4(rot));
            } else {
                boxId = mApp->mLineEngine->addLines(generateCone());
            }
            mApp->mLightBuffer.queueWrite(sizeof(Light) * mSelectedLightInGui, light, sizeof(Light));
        }
        // -------------------------- Back to first Column -----------------------------------
    }
    ImGui::Columns(1);

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
                                glm::vec4{1.0, 0.0, 0.0, 1.0}, 0.837, 0.709, 0.08, 0.4, 1.0, new_light_name);
            } else if (is_point) {
                createPointLight(glm::vec4{0.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0},
                                 glm::vec4{1.0, 0.0, 0.0, 1.0}, glm::vec4{1.0, 0.0, 0.0, 1.0}, 1.0, -1.0, 1.8, 1.0,
                                 new_light_name);
            }
        }
    }
}

void LightManager::nextLight() {
    mSelectedLightInGui = mSelectedLightInGui < mLights.size() - 1 ? mSelectedLightInGui + 1 : 0;
}

std::vector<Light>& LightManager::getLights() { return mLights; }

std::vector<std::string>& LightManager::getLightsNames() { return mLightsNames; }
