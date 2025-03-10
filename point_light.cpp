#include "point_light.h"

#include "application.h"
#include "utils.h"

LightManager::LightManager(Application* app) { mApp = app; }

LightManager* LightManager::init(Application* app) {
    static LightManager mLightInstance{app};
    mLightInstance.mLights.reserve(10);
    return &mLightInstance;
}

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

    mLights.push_back(light);
}

Light* LightManager::get(size_t index) { return &mLights[index]; }

void LightManager::uploadToGpu(Application* app, WGPUBuffer buffer) {
    std::cout << "+++++++++++++ light length is " << mLights.size() << std::endl;
    wgpuQueueWriteBuffer(app->getRendererResource().queue, buffer, 0, mLights.data(), sizeof(Light) * mLights.size());
}

void LightManager::renderGUI() {
    auto* mPointlight = get(mSelectedLightInGui);
    glm::vec4 tmp_ambient = mPointlight->mAmbient;
    glm::vec4 tmp_pos = mPointlight->mPosition;
    /*static glm::vec3 tmp_pos = glm::vec3{0.0f};  // arrow_model.getPosition();*/
    float tmp_linear = mPointlight->mLinear;
    float tmp_quadratic = mPointlight->mQuadratic;
    ImGui::ColorEdit3("Point Light Color #0", glm::value_ptr(mPointlight->mAmbient));
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
