#ifndef TEST_WGPU_SKYBOX_H
#define TEST_WGPU_SKYBOX_H

#include <filesystem>

#include "binding_group.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

class SkyBox {
    public:
        SkyBox(Application* app, const std::filesystem::path& cubeTexturePath);

        Pipeline* getPipeline();
        void draw(Application* app, WGPURenderPassEncoder encoder, const glm::mat4& proj);

    private:
        Pipeline* mRenderPipeline;
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData{5};
        WGPUBuffer mMatrixBuffer;

        glm::mat4 mMatrix;

        WGPUBuffer mCubeVertexDataBuffer;
        WGPUBuffer mCubeIndexDataBuffer;

        // application instance
        Application* app = nullptr;

        void CreateBuffer();
};

#endif  //! TEST_WGPU_SKYBOX_H
