
#include "frustum_culling.h"

#include "glm/fwd.hpp"

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"

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
