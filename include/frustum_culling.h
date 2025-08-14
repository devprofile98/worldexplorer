#ifndef WEBGPUTEST_FRUSTUM_CULLING_H
#define WEBGPUTEST_FRUSTUM_CULLING_H

#include <vector>

#include "camera.h"
#include "glm/fwd.hpp"
#include "gpu_buffer.h"

// Ensure 16-byte alignment for structs used in uniform/storage buffers
// Use alignas(16) for structs
struct alignas(16) FrustumPlane {
        glm::vec4 N_D;  // (Nx, Ny, Nz, D)
};

struct alignas(16) FrustumPlanesUniform {
        FrustumPlane planes[2];  // Left, Right, Bottom, Top, Near, Far
};

void setupComputePass(Application* app, WGPUBuffer instanceDataBuffer);
WGPUBindGroup createObjectInfoBindGroupForComputePass(Application* app, WGPUBuffer objetcInfoBuffer,
                                                      WGPUBuffer indirectDrawArgsBuffer);
void runFrustumCullingTask(Application* app, WGPUCommandEncoder encoder);

Buffer& getFrustumPlaneBuffer();

namespace frustum {

class Plane {
    public:
        Plane() {}
        Plane(const glm::vec3& p1, const glm::vec3& norm);
        Plane(float d, const glm::vec3& norm);
        glm::vec3 normal;
        float distance;

        void normalize();
};
}  // namespace frustum
class Frustum {
    public:
        Frustum() {};
        frustum::Plane topFace;
        frustum::Plane bottomFace;

        frustum::Plane rightFace;
        frustum::Plane leftFace;

        frustum::Plane farFace;
        frustum::Plane nearFace;
        frustum::Plane faces[6];
        void extractPlanes(glm::mat4x4 projectionMatrix);
        bool AABBTest(const glm::vec3& min, const glm::vec3& max);
        void createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar);
        void createFrustumPlanesFromCorner(const std::vector<glm::vec4>& corners);
};

#endif  // WEBGPUTEST_FRUSTUM_CULLING_H
