
#ifndef WEBGPUTEST_TERRAIN_PASS_H
#define WEBGPUTEST_TERRAIN_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "shadow_pass.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"
#include "renderpass.h"

class Application;



class TerrainPass : public RenderPass {
        void createRenderPass(WGPUTextureFormat textureFormat) override;

    public:
        explicit TerrainPass(Application* app);

        Pipeline* create(WGPUTextureFormat textureFormat);

    private:
        Application* mApp;
};



class OutlinePass : public RenderPass {
        void createRenderPass(WGPUTextureFormat textureFormat) override;

    public:
        explicit OutlinePass(Application* app);

        Pipeline* create(WGPUTextureFormat textureFormat);

    private:
        Application* mApp;
};

#endif  // !WEBGPUTEST_TERRAIN_PASS_H
