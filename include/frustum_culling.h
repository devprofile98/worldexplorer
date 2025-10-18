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

struct FrustumCorners {
        glm::vec4 nearBottomLeft;
        glm::vec4 farBottomLeft;
        glm::vec4 nearTopLeft;
        glm::vec4 farTopLeft;
        glm::vec4 nearBottomRight;
        glm::vec4 farBottomRight;
        glm::vec4 nearTopRight;
        glm::vec4 farTopRight;
};

void setupComputePass(Application* app, WGPUBuffer instanceDataBuffer);
WGPUBindGroup createObjectInfoBindGroupForComputePass(Application* app, WGPUBuffer objetcInfoBuffer,
                                                      WGPUBuffer indirectDrawArgsBuffer);
void runFrustumCullingTask(Application* app, WGPUCommandEncoder encoder);

Buffer& getFrustumPlaneBuffer();

FrustumCorners getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
std::vector<FrustumPlane> create2FrustumPlanes(const FrustumCorners& corners);
bool isInFrustum(const FrustumCorners& corners, BaseModel* model);

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
        bool AABBTest(const glm::vec3& min, const glm::vec3& max);
};

#endif  // WEBGPUTEST_FRUSTUM_CULLING_H
