#ifndef TEST_WGPU_POINT_LIGHT
#define TEST_WGPU_POINT_LIGHT

#include <cstdint>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "gpu_buffer.h"
#include "webgpu.h"

class Application;

enum LightType : uint32_t { DIRECTIONAL = 1, SPOT, POINT };

struct Light {
        glm::vec4 mPosition;
        glm::vec4 mDirection;
        glm::vec4 mAmbient;
        glm::vec4 mDiffuse;
        glm::vec4 mSpecular;

        float mConstant;
        float mLinear;
        float mQuadratic;
        LightType type;
        float mInnerCutoff;
        float mOuterCutoff;
        float padding[2];
};

class LightManager {
    public:
        static LightManager* init(Application* app);
        void createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin,
                              float quad);

        void createSpotLight(glm::vec4 pos, glm::vec4 direction, float cutoff, float outerCutoff);

        Light* get(size_t index);
        void uploadToGpu(Application* app, WGPUBuffer buffer);

        void renderGUI();
        void nextLight();
        Buffer& getCountBuffer();
        std::vector<Light>& getLights();

    private:
        Application* mApp;
        LightManager(Application* app);
        /*static inline Light* mLightInstance;*/
        Buffer mLightCountBuffer;
        std::vector<Light> mLights;
        size_t mSelectedLightInGui;
};

#endif  // TEST_WGPU_POINT_LIGHT
