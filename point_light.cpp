#include "point_light.h"

#include "application.h"

PointLight::PointLight() {}

PointLight::PointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin, float quad)
    : mPosition(pos), mAmbient(amb), mDiffuse(diff), mSpecular(spec), mConstant(cons), mLinear(lin), mQuadratic(quad) {}

void PointLight::uploadToGpu(Application* app) { (void)app; }