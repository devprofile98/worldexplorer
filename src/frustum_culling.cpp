
#include "frustum_culling.h"

#include <format>

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "shapes.h"

static size_t counter = 0;

std::ostream& operator<<(std::ostream& os, const frustum::Plane& plane) {
    os << "Plane(normal: [" << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z
       << "], d: " << plane.distance << ")";
    return os;
}

frustum::Plane makePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    glm::vec3 normal = normalize(cross(b - a, c - a));
    float d = -dot(normal, a);
    return frustum::Plane{d, normal};  // plane: normal.x * x + normal.y * y + normal.z * z + d = 0
}

void Frustum::createFrustumPlanesFromCorner(const std::vector<glm::vec4>& corners) {
    // calculate,
    auto nlb = corners[0];
    auto nlt = corners[2];
    auto nrb = corners[4];
    auto nrt = corners[6];

    auto flb = corners[1];
    auto flt = corners[3];
    auto frb = corners[5];
    auto frt = corners[7];

    frustum::Plane left = makePlane(nlt, nlb, flb);
    frustum::Plane right = makePlane(nrb, nrt, frt);
    frustum::Plane top = makePlane(nlt, flt, frt);
    frustum::Plane bottom = makePlane(nlb, frb, flb);
    frustum::Plane near = makePlane(nlt, nrt, nrb);
    frustum::Plane far = makePlane(frb, frt, flt);
    (void)left;
    (void)right;
    (void)top;
    (void)bottom;
    (void)near;
    (void)far;

    /*std::cout << "left: " << left << "\n";*/
    /*std::cout << "right: " << right << "\n";*/
    /*std::cout << "top: " << top << "\n";*/
    /*std::cout << "bottom: " << bottom << "\n";*/
    /*std::cout << "near: " << near << "\n";*/
    /*std::cout << "far: " << far << "\n";*/
};

bool isBoxOutsidePlane(const frustum::Plane& plane, const glm::vec3& min, const glm::vec3& max) {
    /*for (int i = 0; i < 8; ++i) {*/
    (void)max;
    if (dot(plane.normal, min) + plane.distance > 0) {
        return false;  // at least one corner is inside this plane
    }
    /*}*/
    return true;  // all corners are outside this plane
}

bool Frustum::AABBTest(const glm::vec3& min, const glm::vec3& max) {
    /*int test = 0;*/
    /*for (const auto& p : planes) {*/
    /*auto p = rightFace;*/
    /*auto dis_min = glm::dot(p.normal, min) + p.distance;*/
    /*auto dis_max = glm::dot(p.normal, max) + p.distance;*/
    /*std::cout << "dis min: " << dis_min << " dis max : " << dis_max << '\n';*/
    /*if ((dis_min >= 0.0 && dis_max >= 0.0)) {*/
    /*    test += 1;*/
    /*} else {*/
    /*}*/
    /*}*/
    /**/
    /*if (test == 1) {*/
    /*std::cout << "someone is here" << std::format("({},{},{} for {})", min.x, min.y, min.z, test) << std::endl;*/
    /*    return true;*/
    /*}*/
    /*test = 0;*/
    /*return true;*/
    for (size_t i = 0; i < 6; i++) {
        if (isBoxOutsidePlane(faces[i], min, max)) {
            return false;  // Culled
        }
    }
    return true;  // Visible or intersecting
}

void Frustum::extractPlanes(glm::mat4x4 projectionMatrix) {
    // extract each plane from the projcetion matrix
    (void)projectionMatrix;
}

void Frustum::createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar) {
    (void)aspect;
    (void)fovY;
    (void)zNear;
    (void)zFar;
    if (counter++ == 10) {
        auto near_center = cam.mCameraPos + cam.mCameraFront * zNear;
        auto far_center = cam.mCameraPos + cam.mCameraFront * zFar;
        auto fov_tan = glm::tan(glm::radians(fovY / 2.0f));
        auto near_height = zNear * fov_tan * aspect * 2.0f;
        auto near_top_right = (near_center + cam.mCameraUp * (near_height / 2.0f)) +
                              (near_center + cam.mRight * (near_height * aspect / 2.0f));
        auto far_height = zFar * fov_tan * aspect * 2.0f;
        auto far_top_right = (far_center + cam.mCameraUp * (far_height / 2.0f)) +
                             (far_center + cam.mRight * (far_height * aspect / 2.0f));

        auto another = far_top_right - cam.mCameraUp * 5.0f;
        auto right_normal = glm::normalize(glm::cross((far_top_right - near_top_right), another));

        std::cout << std::format("pos: {}, {}, {} ||| ", cam.mCameraPos.x, cam.mCameraPos.y, cam.mCameraPos.z);
        std::cout << std::format("near: {}, {}, {} |||", near_center.x, near_center.y, near_center.z);
        std::cout << std::format("far: {}, {}, {} ||| {} {}\n", far_center.x, far_center.y, far_center.z, near_height,
                                 far_height);
        std::cout << std::format("near top right: {}, {}, {} ::: {}, {}, {} :: {},{},{} \n-----------------",
                                 near_top_right.x, near_top_right.y, near_top_right.z, cam.mCameraUp.x, cam.mCameraUp.y,
                                 cam.mCameraUp.z, right_normal.x, right_normal.y, right_normal.z);
        std::cout << 1080 * aspect << std::endl;
        counter = 0;
    }
    /*const float halfVSide = zFar * tanf(fovY * .5f);*/
    /*const float halfHSide = halfVSide * aspect;*/
    /*const glm::vec3 frontMultFar = zFar * cam.mCameraFront;*/

    /*nearFace = {cam.mCameraPos + zNear * cam.mCameraFront, cam.mCameraFront};*/
    /*farFace = {cam.mCameraPos + frontMultFar, -cam.mCameraFront};*/
    /*rightFace = {cam.mCameraPos, glm::cross(frontMultFar - cam.mRight * halfHSide, cam.mCameraUp)};*/
    /*leftFace = {cam.mCameraPos, glm::cross(cam.mCameraUp, frontMultFar + cam.mRight * halfHSide)};*/
    /*topFace = {cam.mCameraPos, glm::cross(cam.mRight, frontMultFar - cam.mCameraUp * halfVSide)};*/
    /*bottomFace = {cam.mCameraPos, glm::cross(frontMultFar + cam.mCameraUp * halfVSide, cam.mRight)};*/

    /*return frustum;*/
}

namespace frustum {

Plane::Plane(const glm::vec3& p1, const glm::vec3& norm) : normal(glm::normalize(norm)), distance(glm::dot(norm, p1)) {}

Plane::Plane(float d, const glm::vec3& norm) : normal(norm), distance(d) {}
}  // namespace frustum
