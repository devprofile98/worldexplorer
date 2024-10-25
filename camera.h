#ifndef WEBGPUTEST_CAMERA_H
#define WEBGPUTEST_CAMERA_H

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Camera {
    public:
        static inline const float PI = 3.14159265358979323846f;

        Camera();
        Camera(glm::vec3 translate, glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle);

        Camera& setViewMatrix(const glm::mat4 viewMatrix);

        // Getter
        glm::mat4 getProjection() const;
        glm::mat4 getView() const;
        glm::mat4 getModel() const;
        glm::mat4 getScale() const;
        glm::mat4 getTranslate() const;
        glm::mat4 getRotation() const;

    private:
        glm::mat4 mRotationMatrix;
        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mModelMatrix;
        glm::mat4 mViewMatrix;
        glm::mat4 mProjectionMatrix;
};

#endif  // WEBGPUTEST_CAMERA_H
