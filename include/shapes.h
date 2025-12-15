#ifndef WEBGPUTEST_SHAPE_H
#define WEBGPUTEST_SHAPE_H

#include <cstdint>
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
class NewRenderPass;

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
        void executePass();

        std::array<float, 12> mLineInstance = {0, -0.5, 1, -0.5, 1, 0.5, 0, -0.5, 1, 0.5, 0, 0.5};
        WGPURenderPipeline mRenderPipeline;

        BindingGroup mBindGroup{};
        BindingGroup mCameraBindGroup{};
        Pipeline* mPipeline;
        Pipeline* mCirclePipeline;
        VertexBufferLayout mVertexBufferLayout;
        VertexBufferLayout mCircleBufferLayout;
        Buffer mVertexBuffer = {};

        Buffer mCircleVertexBuffer = {};
        Buffer mCircleIndexBuffer = {};

        // std::vector<glm::vec4> mLineList;

        std::vector<uint16_t> mCircleIndexData;
        std::vector<glm::vec3> mCircleVertexData;

        std::vector<WGPUBindGroupEntry> mBindingData;
        std::vector<WGPUBindGroupEntry> mCameraBindingData;
        Buffer mOffsetBuffer;
        Buffer mTransformationBuffer;
        NewRenderPass* mLineRenderingPass;

        struct alignas(16) LineSegment {
                glm::vec4 point;
                glm::vec3 color;
                uint32_t transformationId;
        };

        struct LineGroup {
                glm::mat4 transformation;
                glm::vec3 groupColor;
                std::vector<LineSegment> segment;
                uint32_t buffer_offset = 0;  // Starting index in the storage buffer
                bool dirty = true;           // Needs write to buffer?
        };

        uint32_t addLines(const std::vector<glm::vec4>& points, const glm::mat4 transformation = glm::mat4{1.0},
                          const glm::vec3& color = {0.0, 1.0, 0.0});
        void removeLines(uint32_t id);
        void updateLines(uint32_t id, const std::vector<glm::vec4>& newPoints);
        void updateLineTransformation(uint32_t id, const glm::mat4& trans);

        std::unordered_map<uint32_t, LineGroup> mLineGroups;
        uint32_t mNextGroupId = 0;   // For assigning handles
        bool mGlobalDirty = false;   // Needs offset recalculation and potential buffer resize?
        uint32_t mMaxPoints = 1024;  // Initial capacity; grow as needed

    private:
        Application* mApp;
        void initCirclePipeline();
};

#endif  // WEBGPUTEST_SHAPE_H
