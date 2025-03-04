
#include "frustum_culling.h"

#include <format>

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "shapes.h"

bool Frustum::AABBTest(const glm::vec3& min, const glm::vec3& max) {
    /*glm::vec3 plane_normal = glm::vec3{1.0, 0.0, 0.0};*/
    std::array<frustum::Plane, 2> planes = {frustum::Plane{-1.0, 0.0, 0.0, 5.0}, {1.0, 0.0, 0.0, 5.0}};
    /*std::array<frustum::Plane, 1> planes = {frustum::Plane{-1.0, 0.0, 0.0, 10.0}};*/

    int test = 0;
    (void)test;
    /*std::cout << mFrustumPlanes[1].a << " " << mFrustumPlanes[1].b << " " << mFrustumPlanes[1].c << "\n";*/
    for (const auto& p : planes) {
        auto dis_min = glm::dot(glm::vec3{p.a, p.b, p.c}, min) + p.d;
        auto dis_max = glm::dot(glm::vec3{p.a, p.b, p.c}, max) + p.d;
        /*std::cout << "dis min: " << dis_min << " dis max : " << dis_max << '\n';*/
        if ((dis_min >= 0.0 && dis_max >= 0.0)) {
            test += 1;
        } else {
        }
    }

    if (test == 2) {
        /*std::cout << "someone is here" << std::format("({},{},{} for {})", min.x, min.y, min.z, test) << std::endl;*/
        return true;
    }
    test = 0;
    return false;
}
void Frustum::extractPlanes(glm::mat4x4 projectionMatrix) {
    // extract each plane from the projcetion matrix
    mFrustumPlanes[0].a = projectionMatrix[3][0] + projectionMatrix[0][0];
    mFrustumPlanes[0].b = projectionMatrix[3][1] + projectionMatrix[0][1];
    mFrustumPlanes[0].c = projectionMatrix[3][2] + projectionMatrix[0][2];
    mFrustumPlanes[0].d = projectionMatrix[3][3] + projectionMatrix[0][3];
    // Right clipping plane
    mFrustumPlanes[1].a = projectionMatrix[3][0] - projectionMatrix[0][0];
    mFrustumPlanes[1].b = projectionMatrix[3][1] - projectionMatrix[0][1];
    mFrustumPlanes[1].c = projectionMatrix[3][2] - projectionMatrix[0][2];
    mFrustumPlanes[1].d = projectionMatrix[3][3] - projectionMatrix[0][3];
    // Top clipping plane
    mFrustumPlanes[2].a = projectionMatrix[3][0] - projectionMatrix[1][0];
    mFrustumPlanes[2].b = projectionMatrix[3][1] - projectionMatrix[1][1];
    mFrustumPlanes[2].c = projectionMatrix[3][2] - projectionMatrix[1][2];
    mFrustumPlanes[2].d = projectionMatrix[3][3] - projectionMatrix[1][3];
    // Bottom clipping plane
    mFrustumPlanes[3].a = projectionMatrix[3][0] + projectionMatrix[1][0];
    mFrustumPlanes[3].b = projectionMatrix[3][1] + projectionMatrix[1][1];
    mFrustumPlanes[3].c = projectionMatrix[3][2] + projectionMatrix[1][2];
    mFrustumPlanes[3].d = projectionMatrix[3][3] + projectionMatrix[1][3];
    // Near clipping plane
    mFrustumPlanes[4].a = projectionMatrix[3][0] + projectionMatrix[2][0];
    mFrustumPlanes[4].b = projectionMatrix[3][1] + projectionMatrix[2][1];
    mFrustumPlanes[4].c = projectionMatrix[3][2] + projectionMatrix[2][2];
    mFrustumPlanes[4].d = projectionMatrix[3][3] + projectionMatrix[2][3];
    // Far clipping plane
    mFrustumPlanes[5].a = projectionMatrix[3][0] - projectionMatrix[2][0];
    mFrustumPlanes[5].b = projectionMatrix[3][1] - projectionMatrix[2][1];
    mFrustumPlanes[5].c = projectionMatrix[3][2] - projectionMatrix[2][2];
    mFrustumPlanes[5].d = projectionMatrix[3][3] - projectionMatrix[2][3];
    // normalize them
}
