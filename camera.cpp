#include "camera.h"

#include <iostream>

Camera::Camera() : mRotationMatrix({}), mScaleMatrix({}), mTranslationMatrix({}) {
    // mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
}

Camera::Camera(glm::vec3 translate, glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle) {
    mRotationMatrix = glm::rotate(glm::mat4{1.0}, rotationAngle, rotationAxis);
    mScaleMatrix = glm::scale(glm::mat4{1.0}, scale);
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, translate);

    mCameraFront = glm::vec3{0.0f, 0.0f, 0.0f};
    mCameraPos = glm::vec3{0.0f, -1.0f, 1.0f};
    mCameraUp = glm::vec3{0.0f, 0.0f, 1.0f};
    mWorldUp = mCameraUp;

    mViewMatrix = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);

    // Projection Matrix ------------------
    float ratio = 640.0f / 480.0f;
    float focal_length = 2.0;
    float near = 0.01f;
    float far = 200.0f;
    // float divider = 1.0f / (focal_length * (far - near));
    float fov = 2 * glm::atan(1 / focal_length);
    mProjectionMatrix = glm::perspective(fov, ratio, near, far);
}

void Camera::processInput(int key, int scancode, int action, int mod) {
    (void)action;
    (void)scancode;
    (void)mod;
    (void)key;
    const float cameraSpeed = 0.4f;  // adjust accordingly
    if (GLFW_KEY_W == key) {
        mCameraPos += cameraSpeed * glm::normalize(mCameraFront);
    }
    if (GLFW_KEY_S == key) {
        mCameraPos -= cameraSpeed * glm::normalize(mCameraFront);
    }
    if (GLFW_KEY_A == key) {
        mCameraPos -= glm::normalize(glm::cross(mCameraFront, mCameraUp)) * cameraSpeed;
    }
    if (GLFW_KEY_D == key) {
        mCameraPos += glm::normalize(glm::cross(mCameraFront, mCameraUp)) * cameraSpeed;
    }

    // mViewMatrix = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);
}

void Camera::processMouse(int x, int y) {
    float xoffset = x - mLastX;
    float yoffset = mLastY - y;
    mLastX = x;
    mLastY = y;

    float sensitivity = 0.2f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    mYaw += xoffset;
    mPitch += yoffset;

    if (mPitch > 89.0f) mPitch = 89.0f;
    if (mPitch < -89.0f) mPitch = -89.0f;

    // glm::vec3 front;
    // front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    // front.y = sin(glm::radians(mPitch));
    // front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    // mCameraFront = glm::normalize(front);
    mCameraFront.x = cos(glm::radians(mYaw));
    mCameraFront.y = -sin(glm::radians(mYaw));
    mCameraFront.z = sin(glm::radians(mPitch));

    // mCameraFront.z += yoffset;
    // std::cout << mCameraFront.x << " " << mCameraFront.y << " " << mCameraFront.z << std::endl;
    // mRight = glm::vec3{std::cos(mPitch), 0.f, std::sin(mPitch)};
    // mRight = glm::normalize(
    //     glm::cross(mCameraFront, mWorldUp));  // normalize the vectors, because their length gets closer to 0 the
    // mCameraUp = glm::normalize(glm::cross(mRight, mCameraFront));
}

void Camera::updateCursor(int x, int y) {
    mLastX = x;
    mLastY = y;
}

Camera& Camera::setViewMatrix(const glm::mat4 viewMatrix) {
    mViewMatrix = viewMatrix;
    return *this;
}

glm::mat4 Camera::getProjection() const { return mProjectionMatrix; };

glm::mat4 Camera::getView() {
    mViewMatrix = glm::lookAt(mCameraPos, mCameraPos + mCameraFront, mCameraUp);

    return mViewMatrix;
};

glm::mat4 Camera::getModel() const { return mModelMatrix; };

glm::mat4 Camera::getScale() const { return mScaleMatrix; };

glm::mat4 Camera::getTranslate() const { return mTranslationMatrix; };

glm::mat4 Camera::getRotation() const { return mRotationMatrix; };

DragState& Camera::getDrag() { return mDragState; }

CameraState& Camera::getSate() { return mCameraState; }
