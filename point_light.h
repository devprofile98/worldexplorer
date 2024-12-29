#ifndef TEST_WGPU_POINT_LIGHT
#define TEST_WGPU_POINT_LIGHT

#include "glm/ext.hpp"
#include "glm/glm.hpp"

class Application;

class PointLight {
    public:
        PointLight();
        PointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin, float quad);

        void uploadToGpu(Application* app);

        glm::vec4 mPosition;
        glm::vec4 mAmbient;
        glm::vec4 mDiffuse;
        glm::vec4 mSpecular;

        float mConstant;
        float mLinear;
        float mQuadratic;
        float padding;

    private:
};

#endif  // TEST_WGPU_POINT_LIGHT