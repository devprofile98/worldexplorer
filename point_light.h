#ifndef TEST_WGPU_POINT_LIGHT
#define TEST_WGPU_POINT_LIGHT

#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "gpu_buffer.h"
#include "webgpu.h"

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
        LightType type;
};

class LightManager {
    public:
        static LightManager* init(Application* app);
        void createPointLight(glm::vec4 pos, glm::vec4 amb, glm::vec4 diff, glm::vec4 spec, float cons, float lin,
                              float quad);

	Light* get(size_t index);
        void uploadToGpu(Application* app, WGPUBuffer buffer);

	void renderGUI();
	void nextLight();

    private:
	Application* mApp;
        LightManager(Application* app);
        /*static inline Light* mLightInstance;*/
        std::vector<Light> mLights;
	size_t mSelectedLightInGui;
};

#endif  // TEST_WGPU_POINT_LIGHT
