
#ifndef WORLD_EXPLORER_CORE_WINDOW_H
#define WORLD_EXPLORER_CORE_WINDOW_H

#include <expected>
#include <functional>
#include <glm/fwd.hpp>
#include <iostream>

#include "GLFW/glfw3.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"

template <typename W>
class Window {
    public:
        Window() = default;

        std::expected<GLFWwindow*, bool> create(glm::vec2 windowSize) {
            mWindowSize = windowSize;
            // Placeholder for window creation logic

            if (!glfwInit()) {
                std::cerr << "Could not initialize GLFW!" << std::endl;
                return std::unexpected<bool>(false);
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // <-- extra info for glfwCreateWindow
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            // auto [window_width, window_height] = getWindowSize();
            mWindowPtr = glfwCreateWindow(mWindowSize.x, mWindowSize.y, mName, nullptr, nullptr);
            if (!mWindowPtr) {
                std::cerr << "Could not open window!" << std::endl;
                return std::unexpected<bool>(false);
            }
            glfwMakeContextCurrent(mWindowPtr);  // window: your GLFWwindow*
            glfwSwapInterval(0);

            // glfwSetWindowUserPointer(mWindowPtr, this);  // set user pointer to be used in the callback function
            // glfwSetFramebufferSizeCallback(mWindowPtr, onWindowResize);

            return mWindowPtr;
        }

        W* getWindow() { return mWindowPtr; }

        // static void onWindowResize(GLFWwindow* window, int width, int height) {
        //     auto* that = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //     that->mWindowSize = {width, height};
        //     that->mResizeCallback();
        // }
        static void onWindowResize(GLFWwindow* window, int width, int height) {
            auto* that = reinterpret_cast<Window<GLFWwindow*>*>(glfwGetWindowUserPointer(window));
            that->mWindowSize = {width, height};
            that->mResizeCallback();
        }

        using onResizeCallbackType = std::function<void()>;
        onResizeCallbackType mResizeCallback;

        const char* mName = "Window";

        // private:
        glm::vec2 mWindowSize;
        W* mWindowPtr = nullptr;
};

#endif  // WORLD_EXPLORER_CORE_WINDOW_H
