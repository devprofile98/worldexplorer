
#include "frustum_culling.h"

#include <format>

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "shapes.h"

static size_t counter = 0;
bool Frustum::AABBTest(const glm::vec3& min, const glm::vec3& max) {
    /*glm::vec3 plane_normal = glm::vec3{1.0, 0.0, 0.0};*/
    /*std::array<frustum::Plane, 2> planes = {frustum::Plane{-1.0, 0.0, 0.0, 5.0}, {1.0, 0.0, 0.0, 5.0}};*/

    int test = 0;
    if (counter++ == 10) {
        std::cout << "right: " << rightFace.normal.x << " " << rightFace.normal.y << " " << rightFace.normal.z << " "
                  << rightFace.distance << "\n";

        std::cout << "left: " << leftFace.normal.x << " " << leftFace.normal.y << " " << leftFace.normal.z << " "
                  << leftFace.distance << "\n";
        std::cout << "top: " << topFace.normal.x << " " << topFace.normal.y << " " << topFace.normal.z << " "
                  << topFace.distance << "\n";
        std::cout << "bottom: " << bottomFace.normal.x << " " << rightFace.normal.y << " " << rightFace.normal.z << " "
                  << bottomFace.distance << "\n";
        counter = 0;
    }
    /*for (const auto& p : planes) {*/
    auto p = rightFace;
    auto dis_min = glm::dot(p.normal, min) + p.distance;
    auto dis_max = glm::dot(p.normal, max) + p.distance;
    /*std::cout << "dis min: " << dis_min << " dis max : " << dis_max << '\n';*/
    if ((dis_min >= 0.0 && dis_max >= 0.0)) {
        test += 1;
    } else {
    }
    /*}*/

    if (test == 1) {
        /*std::cout << "someone is here" << std::format("({},{},{} for {})", min.x, min.y, min.z, test) << std::endl;*/
        return true;
    }
    test = 0;
    return true;
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

}  // namespace frustum
