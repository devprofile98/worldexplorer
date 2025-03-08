#ifndef TEST_WGPU_POINT_LIGHT
#define TEST_WGPU_POINT_LIGHT

#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"

class Application;

enum LightType { DIRECTIONAL = 1, SPOT, POINT };

struct Light {
        glm::vec4 mPosition;
        glm::vec4 mAmbient;
        glm::vec4 mDiffuse;
        glm::vec4 mSpecular;

        float mConstant;
        float mLinear;
        float mQuadratic;
        float padding;
        LightType type;
};

class LightManager {
    public:
        static LightManager* init();
        void createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin,
                              float quad);

	Light* get();
        void uploadToGpu(Application* app);

    private:
        LightManager();
        /*static inline Light* mLightInstance;*/
        std::vector<Light> mLights;
};

#endif  // TEST_WGPU_POINT_LIGHT
