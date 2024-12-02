#include "camera.h"

Camera::Camera() : mRotationMatrix({}), mScaleMatrix({}), mTranslationMatrix({}) {
    mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
}

Camera::Camera(glm::vec3 translate, glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle) {
    mRotationMatrix = glm::rotate(glm::mat4{1.0}, rotationAngle, rotationAxis);
    mScaleMatrix = glm::scale(glm::mat4{1.0}, scale);
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, translate);
    mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;

    // View Matrix ------------------
    glm::vec3 focal_point{1.0, 1.0, 0.0};
    glm::mat4 T2 = glm::translate(glm::mat4{1.0f}, -focal_point);

    // Rotate the view point
    float angle2 = 3.0 * PI / 4.0;
    glm::mat4 R2 = glm::rotate(glm::mat4{1.0f}, -angle2, glm::vec3{1.0, 0.0, 0.0});

    mViewMatrix = T2 * R2;
    mViewMatrix = glm::lookAt({1.0, 1.0, 1.0}, glm::vec3(0.0f), glm::vec3(0, 0, 1));

    // Projection Matrix ------------------
    float ratio = 640.0f / 480.0f;
    float focal_length = 2.0;
    float near = 0.01f;
    float far = 200.0f;
    // float divider = 1.0f / (focal_length * (far - near));
    float fov = 2 * glm::atan(1 / focal_length);
    mProjectionMatrix = glm::perspective(fov, ratio, near, far);
}

Camera& Camera::setViewMatrix(const glm::mat4 viewMatrix) {
    mViewMatrix = viewMatrix;
    return *this;
}

glm::mat4 Camera::getProjection() const { return mProjectionMatrix; };

glm::mat4 Camera::getView() const { return mViewMatrix; };

glm::mat4 Camera::getModel() const { return mModelMatrix; };

glm::mat4 Camera::getScale() const { return mScaleMatrix; };

glm::mat4 Camera::getTranslate() const { return mTranslationMatrix; };

glm::mat4 Camera::getRotation() const { return mRotationMatrix; };

DragState& Camera::getDrag() { return mDragState; }

CameraState& Camera::getSate() { return mCameraState; }