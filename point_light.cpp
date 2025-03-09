#include "point_light.h"

#include "application.h"

LightManager::LightManager() {}

LightManager* LightManager::init() {
    static LightManager mLightInstance{};
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

Light* LightManager::get() { return &mLights[0]; }

void LightManager::uploadToGpu(Application* app, WGPUBuffer buffer) {
    wgpuQueueWriteBuffer(app->getRendererResource().queue, buffer, 0, mLights.data(), sizeof(Light) * mLights.size());
}
