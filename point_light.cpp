#include "point_light.h"

#include <cstdint>
#include <format>

#include "application.h"
#include "glm/fwd.hpp"
#include "utils.h"
#include "webgpu.h"

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
    /*if (mPointlight->type == SPOT) {*/
    /*    return;*/
    /*}*/
    glm::vec4 tmp_ambient = mPointlight->mAmbient;
    glm::vec4 tmp_pos = mPointlight->mPosition;
    float tmp_linear = mPointlight->mLinear;
    float tmp_quadratic = mPointlight->mQuadratic;
    ImGui::ColorEdit3(
        std::format("{} color # {}", mPointlight->type == SPOT ? "Spot" : "Point", mSelectedLightInGui).c_str(),
        glm::value_ptr(mPointlight->mAmbient));
    /*ImGui::DragFloat3("Point Ligth Position #0", glm::value_ptr(mPointlight->mPosition), 0.1, -10.0, 10.0);*/
    ImGui::SliderFloat("Linear", &tmp_linear, -10.0f, 10.0f);
    ImGui::SliderFloat("Quadratic", &tmp_quadratic, -10.0f, 10.0f);

    ImGui::SliderFloat("pos x", &tmp_pos.x, -20.0f, 20.0f);
    ImGui::SliderFloat("pos y", &tmp_pos.y, -20.0f, 20.0f);
    ImGui::SliderFloat("pos z", &tmp_pos.z, -20.0f, 20.0f);

    if (tmp_ambient != mPointlight->mAmbient ||
        (tmp_pos.x != mPointlight->mPosition.x || tmp_pos.y != mPointlight->mPosition.y ||
         tmp_pos.z != mPointlight->mPosition.z || tmp_linear != mPointlight->mLinear ||
         tmp_quadratic != mPointlight->mQuadratic)) {
        mPointlight->mPosition = tmp_pos;
        mPointlight->mLinear = tmp_linear;
        mPointlight->mQuadratic = tmp_quadratic;
        wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mApp->mBuffer1, sizeof(Light) * mSelectedLightInGui,
                             mPointlight, sizeof(Light));
    }
    //
}

void LightManager::nextLight() {
    mSelectedLightInGui = mSelectedLightInGui < mLights.size() - 1 ? mSelectedLightInGui + 1 : 0;
}
