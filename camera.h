#ifndef WEBGPUTEST_CAMERA_H
#define WEBGPUTEST_CAMERA_H

#include <GLFW/glfw3.h>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

struct CameraState {
        // angles.x is the rotation of the camera around the global vertical axis, affected by mouse.x
        // angles.y is the rotation of the camera around its local horizontal axis, affected by mouse.y
        glm::vec2 angles = {0.0f, 0.0f};
        // zoom is the position of the camera along its local forward axis, affected by the scroll wheel
        float zoom = 0.2f;
};

struct DragState {
        // Whether a drag action is ongoing (i.e., we are between mouse press and mouse release)
        bool active = false;
        bool firstMouse = false;
        // The position of the mouse at the beginning of the drag action
        glm::vec2 startMouse;
        // The camera state at the beginning of the drag action
        CameraState startCameraState;

        // Constant settings
        float sensitivity = 0.001f;
        float scrollSensitivity = 0.1f;

        // interia
        glm::vec2 velocity = {0.0, 0.0};
        glm::vec2 previousDelta;
        float intertia = 0.9f;
};

class Camera {
    public:
        static inline const float PI = 3.14159265358979323846f;

        Camera();
        Camera(glm::vec3 translate, glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle);

        Camera& setViewMatrix(const glm::mat4 viewMatrix);
        Camera& setTarget(glm::vec3 target);
        Camera& setPosition(glm::vec3 position);

        // Getter
        glm::mat4 getProjection() const;
        glm::mat4 getView();
        glm::mat4 getModel() const;
        glm::mat4 getScale() const;
        glm::mat4 getTranslate() const;
        glm::mat4 getRotation() const;
        const glm::vec3& getPos() const;
        CameraState& getSate();
        DragState& getDrag();

        void processInput(int key, int scancode, int action, int mod);
        void processMouse(int x, int y);
        void processScroll(double value);
        void updateCursor(int x, int y);

    private:
        CameraState mCameraState;
        DragState mDragState;
        glm::mat4 mRotationMatrix;
        glm::mat4 mScaleMatrix;
        glm::mat4 mTranslationMatrix;
        glm::mat4 mModelMatrix;
        glm::mat4 mViewMatrix;
        glm::mat4 mProjectionMatrix;

        glm::vec3 mCameraUp;
        glm::vec3 mCameraPos;
        glm::vec3 mCameraFront;
        glm::vec3 mRight;
        glm::vec3 mWorldUp;

        float mYaw = -90.0f;
        float mPitch = 0;

        int mLastX = 0;
        int mLastY = 0;
};

#endif  // WEBGPUTEST_CAMERA_H
