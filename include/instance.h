
#ifndef WEBGPUTEST_INSTANCE_H
#define WEBGPUTEST_INSTANCE_H

#include <cstdint>
#include <vector>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include "gpu_buffer.h"

class Application;

struct alignas(16) InstanceData {
        glm::mat4 modelMatrix;
        glm::vec4 minAABB;
        glm::vec4 maxAABB;
};

class InstanceManager {
    public:
        explicit InstanceManager(Application* app, size_t bufferSize, size_t maxInstancePerModel);

        size_t mBufferSize = 0;
        // Getter
        Buffer& getInstancingBuffer();

    private:
        Buffer mOffsetBuffer;
};

class Instance {
    public:
        // explicit Instance(std::vector<glm::mat4>& instanceBuffer);
        explicit Instance(std::vector<glm::vec3> positions, glm::vec3 rotationAxis, std::vector<float> degree,
                          std::vector<glm::vec3> scales, const glm::vec4&& minAABB, const glm::vec4&& maxAABB);

        // Getter
        size_t getInstanceCount();
        uint16_t getInstanceID();
        // uint16_t addNewInstance(const glm::vec3& positoin, const glm::vec3& scale, const glm::quat& rot);
        uint16_t duplicateLastInstance(const glm::vec3& posOffset, const glm::vec3& min, const glm::vec3& max);
        void dumpJson();
        std::vector<glm::vec3> mPositions;
        std::vector<glm::vec3> mRotation;
        std::vector<glm::vec3> mScale;

        std::vector<InstanceData> mInstanceBuffer;
        uint16_t mOffsetID = 0;
};

#endif  // WEBGPUTEST_INSTANCE_H
