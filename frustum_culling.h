#ifndef WEBGPUTEST_FRUSTUM_CULLING_H
#define WEBGPUTEST_FRUSTUM_CULLING_H

#include "camera.h"
#include "glm/fwd.hpp"

namespace frustum {

class Plane {
    public:
        Plane() {}
        Plane(const glm::vec3& p1, const glm::vec3& norm);
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
        void extractPlanes(glm::mat4x4 projectionMatrix);
        bool AABBTest(const glm::vec3& min, const glm::vec3& max);
        void createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar);
};

#endif  // WEBGPUTEST_FRUSTUM_CULLING_H
