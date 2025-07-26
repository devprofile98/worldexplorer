#include "point_light.h"

#include <cstdint>
#include <format>

#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "application.h"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
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
    static LightManager mLightInstance{app};
    mLightInstance.mLights.reserve(10);
    return &mLightInstance;
}

Buffer& LightManager::getCountBuffer() { return mLightCountBuffer; }

void LightManager::createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin,
                                    float quad) {
    Light light;
    light.mPosition = pos;
    light.mAmbient = amb;
    light.mDiffuse = diff;
    light.mSpecular = spec;
    light.mConstant = cons;
    light.mLinear = lin;
    light.mQuadratic = quad;
    light.type = POINT;

    mLights.push_back(light);
}

void LightManager::createSpotLight(glm::vec4 pos, glm::vec4 direction, float cutoff, float outerCutoff) {
    (void)cutoff;
    (void)outerCutoff;
    Light light;
    light.mPosition = pos;
    light.mDirection = direction;
    light.mDiffuse = glm::vec4(1.0, 0.0, 0.0, 1.0);
    light.mInnerCutoff = 0.97;
    light.mOuterCutoff = 0.99;
    light.type = SPOT;
    light.mConstant = 1.0;
    light.mLinear = 0.7;
    light.mQuadratic = 1.8;

    mLights.push_back(light);
}

Light* LightManager::get(size_t index) { return &mLights[index]; }

void LightManager::uploadToGpu(Application* app, WGPUBuffer buffer) {
    uint32_t size = mLights.size();
    wgpuQueueWriteBuffer(app->getRendererResource().queue, getCountBuffer().getBuffer(), 0, &size, sizeof(uint32_t));
    wgpuQueueWriteBuffer(app->getRendererResource().queue, buffer, 0, mLights.data(), sizeof(Light) * mLights.size());
}

void LightManager::renderGUI() {
    auto* mPointlight = get(mSelectedLightInGui);
    auto tmp_light = *mPointlight;

    ImGui::ColorEdit3(
        std::format("{} color # {}", mPointlight->type == SPOT ? "Spot" : "Point", mSelectedLightInGui).c_str(),
        glm::value_ptr(tmp_light.mAmbient));

    ImGui::SliderFloat3("Position", glm::value_ptr(tmp_light.mPosition), -20.0f, 20.0f);

    if (tmp_light.type == SPOT) {
        ImGui::SliderFloat3("Direction", glm::value_ptr(tmp_light.mDirection), -1.0, 1.0f);
        ImGui::SliderFloat("Inner Cutoff", &tmp_light.mInnerCutoff, 0, 360.0);
        ImGui::SliderFloat("outer Cutoff", &tmp_light.mOuterCutoff, 0, 360.0);
    }
    ImGui::SliderFloat("Linear", &tmp_light.mLinear, -10.0f, 10.0f);
    ImGui::SliderFloat("Quadratic", &tmp_light.mQuadratic, -10.0f, 10.0f);

    if (!glm::all(glm::equal(tmp_light.mAmbient, mPointlight->mAmbient)) ||
        !glm::all(glm::equal(tmp_light.mPosition, mPointlight->mPosition)) ||
        !glm::all(glm::equal(tmp_light.mDirection, mPointlight->mDirection)) ||
        tmp_light.mLinear != mPointlight->mLinear || tmp_light.mQuadratic != mPointlight->mQuadratic ||
        tmp_light.mInnerCutoff != mPointlight->mInnerCutoff || tmp_light.mOuterCutoff != mPointlight->mOuterCutoff) {
        *mPointlight = tmp_light;
        wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->mBuffer1, sizeof(Light) * mSelectedLightInGui,
                             mPointlight, sizeof(Light));
    }

    ImGui::BeginChild("Light manager list", ImVec2(0, 200),
                      ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);

    for (size_t i = 0; i < getLights().size(); i++) {
        // Create a unique ID for each selectable item based on its unique item.id
        auto* light = &getLights()[i];
        ImGui::PushID((void*)&light);

        if (ImGui::Selectable("light", light == &getLights()[mSelectedLightInGui])) {
            ImGui::Text("%s", glm::to_string(light->mPosition).c_str());
        }

        ImGui::PopID();  // Pop the unique ID for this item
    }
    ImGui::EndChild();  // End the scrollable list

    //
}

void LightManager::nextLight() {
    mSelectedLightInGui = mSelectedLightInGui < mLights.size() - 1 ? mSelectedLightInGui + 1 : 0;
}

std::vector<Light>& LightManager::getLights() { return mLights; }
