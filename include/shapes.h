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

// class Cube : public BaseModel {
//     public:
//         Cube(Application* app);
//         virtual void draw(Application* app, WGPURenderPassEncoder encoder) override;
//         void userInterface() override;
//         size_t getVertexCount() const override;
//
//     private:
//         // vertex indices
//         Buffer mIndexDataBuffer = {};
//         Buffer offset_buffer = {};
//         // ? material
//         Application* mApp;
// };
//
// class Plane : public Cube {
//     public:
//         Plane(Application* app);
//
//     private:
//         std::map<int, Mesh> mMeshes;
// };

// struct alignas(16) Line {
//         glm::vec4 p1;
//         glm::vec4 p2;
// };

struct LineEngine {
        struct alignas(16) LineSegment {
                glm::vec4 point;
                glm::vec3 color;
                uint32_t transformationId;
                uint32_t isActive;
        };

        struct LineGroup {
                glm::mat4 transformation;
                glm::vec3 groupColor;
                std::vector<LineSegment> segment;
                uint32_t buffer_offset = 0;  // Starting index in the storage buffer
                bool dirty = true;           // Needs write to buffer?
        };
        void initialize(Application* app);
        void draw(Application* app, WGPURenderPassEncoder encoder);
        void executePass();
        uint32_t addLines(const std::vector<glm::vec4>& points, const glm::mat4& transformation = glm::mat4{1.0},
                          const glm::vec3& color = {0.0, 1.0, 0.0});
        ::LineGroup create(const std::vector<glm::vec4>& points, const glm::mat4& transformation = glm::mat4{1.0},
                           const glm::vec3& color = {0.0, 1.0, 0.0});
        void removeLines(uint32_t id);
        void setVisibility(uint32_t id, bool visibility);
        void updateLines(uint32_t id, const std::vector<glm::vec4>& newPoints);
        void updateLineTransformation(uint32_t id, const glm::mat4& trans);
        void updateLineColor(uint32_t id, const glm::vec3& color);

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

        std::vector<uint16_t> mCircleIndexData;
        std::vector<glm::vec3> mCircleVertexData;

        std::vector<WGPUBindGroupEntry> mBindingData;
        std::vector<WGPUBindGroupEntry> mCameraBindingData;
        Buffer mOffsetBuffer;
        Buffer mTransformationBuffer;
        NewRenderPass* mLineRenderingPass;

        std::unordered_map<uint32_t, LineGroup> mLineGroups;
        uint32_t mNextGroupId = 0;        // For assigning handles
        bool mGlobalDirty = false;        // Needs offset recalculation and potential buffer resize?
        uint32_t mMaxPoints = 10 * 1024;  // Initial capacity; grow as needed

    private:
        Application* mApp;
        void initCirclePipeline();
};

#endif  // WEBGPUTEST_SHAPE_H
