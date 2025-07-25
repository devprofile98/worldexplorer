#ifndef TEST_WGPU_SKYBOX_H
#define TEST_WGPU_SKYBOX_H

#include <filesystem>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "binding_group.h"
#include "gpu_buffer.h"
#include "pipeline.h"

class Application;

class SkyBox {
    public:
        SkyBox(Application* app, const std::filesystem::path& cubeTexturePath);

        Pipeline* getPipeline();
        void setReflectedCameraMatrix();
        void draw(Application* app, WGPURenderPassEncoder encoder, const glm::mat4& proj);

        BindingGroup mBindingGroup;
        BindingGroup mReflectedBindingGroup;
        Buffer mReflectedCameraMatrix;

    private:
        Pipeline* mRenderPipeline;

        std::vector<WGPUBindGroupEntry> mBindingData{5};
        std::vector<WGPUBindGroupEntry> mReflectedBindingData{5};
        WGPUBuffer mMatrixBuffer;

        glm::mat4 mMatrix;

        WGPUBuffer mCubeVertexDataBuffer;
        WGPUBuffer mCubeIndexDataBuffer;

        // application instance
        Application* app = nullptr;

        void CreateBuffer();
};

#endif  //! TEST_WGPU_SKYBOX_H
