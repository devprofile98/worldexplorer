#ifndef WEBGPUTEST_SHAPE_H
#define WEBGPUTEST_SHAPE_H

#include <vector>

#include "../webgpu/webgpu.h"
#include "binding_group.h"
#include "glm/fwd.hpp"
#include "gpu_buffer.h"
#include "mesh.h"
#include "model.h"
#include "pipeline.h"
#include "utils.h"

class Application;

class Cube : public BaseModel {
    public:
        Cube(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder) override;
        void userInterface() override;
        size_t getVertexCount() const override;

    private:
        // vertex indices
        Buffer mIndexDataBuffer = {};
        Buffer offset_buffer = {};
        // ? material
        Application* mApp;
};

class Plane : public Cube {
    public:
        Plane(Application* app);

    private:
        std::map<int, Mesh> mMeshes;
};

// class Line : public BaseModel {
//     public:
//         Line(Application* app, glm::vec3 start, glm::vec3 end, float width, glm::vec3 color);
//
//         virtual void draw(Application* app, WGPURenderPassEncoder encoder) override;
//
//     private:
//         Application* mApp;
//         Buffer mIndexDataBuffer = {};
//         /*float triangleVertexData[33];*/
//         uint16_t mIndexData[6] = {0, 1, 2, 1, 2, 3};
//         float triangleVertexData[112] = {};
//         //
//         //     1.0, 0.5, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  //
//         //     6.0, 3.0, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,                                      //
//         //     6.1, 2.8, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,                                      //
//         // };
//         /*    std::map<int, Mesh> mMeshes;*/
// };

struct alignas(16) Line {
        glm::vec4 p1;
        glm::vec4 p2;
};

struct LineEngine {
        void initialize(Application* app);
        void draw(Application* app, WGPURenderPassEncoder encoder);

        std::array<float, 12> mLineInstance = {0, -0.5, 1, -0.5, 1, 0.5, 0, -0.5, 1, 0.5, 0, 0.5};
        WGPURenderPipeline mRenderPipeline;

        BindingGroup mBindGroup{};
        BindingGroup mCameraBindGroup{};
        Pipeline* mPipeline;
        VertexBufferLayout mVertexBufferLayout;
        Buffer mVertexBuffer = {};
        std::vector<Line> mLineList;

        std::vector<WGPUBindGroupEntry> mBindingData;
        std::vector<WGPUBindGroupEntry> mCameraBindingData;
        Buffer mOffsetBuffer;
};

#endif  // WEBGPUTEST_SHAPE_H
