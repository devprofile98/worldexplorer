#ifndef WEBGPUTEST_FRUSTUM_CULLING_H
#define WEBGPUTEST_FRUSTUM_CULLING_H

#include <array>

#include "glm/fwd.hpp"
namespace frustum {

class Plane {
    public:
        float a, b, c, d;
};
}  // namespace frustum
class Frustum {
    public:
        Frustum() {};
        std::array<frustum::Plane, 6> mFrustumPlanes;

        void extractPlanes(glm::mat4x4 projectionMatrix);
};

#endif  // WEBGPUTEST_FRUSTUM_CULLING_H
