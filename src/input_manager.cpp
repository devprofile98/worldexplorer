#include "input_manager.h"

#include "GLFW/glfw3.h"
#include "editor.h"
#include "utils.h"

InputManager::InputManager() {}

InputManager& InputManager::instance() {
    static InputManager inst = InputManager{};
    return inst;
}

void InputManager::handleMouseMove(GLFWwindow* window, double xPos, double yPos) {
    mWindow = window;
    MouseEvent event = Move{window, xPos, yPos};
    for (auto* listener : instance().mMouseMoveListeners) {
        listener->onMouseMove(event);
    }
}

void InputManager::handleButton(GLFWwindow* window, int click, int action, int mods) {
    mWindow = window;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(click, action == GLFW_PRESS);

    if (!io.WantCaptureMouse) {
        MouseEvent event = Click{window, xpos, ypos, click, action, mods};

        for (auto* listener : instance().mMouseButtonListeners) {
            listener->onMouseClick(event);
        }
    }
}

void InputManager::handleScroll(GLFWwindow* window, double xOffset, double yOffset) {
    mWindow = window;
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xOffset), static_cast<float>(yOffset));
    if (!io.WantCaptureMouse) {
        MouseEvent event = Scroll{window, xOffset, yOffset};

        for (auto* listener : instance().mMouseScrollListeners) {
            listener->onMouseScroll(event);
        }
    }
}

void InputManager::setCursorPosition(GLFWwindow* window, double xPos, double yPos) {
    mWindow = window;
    glfwSetCursorPos(window, xPos, yPos);
}

bool InputManager::isKeyDown(int key) {
    // 1. Trust our array FIRST (callback-updated)
    if (key >= 0 && key <= GLFW_KEY_LAST && keys[key]) {
        return true;
    }
    // 2. FALLBACK: Query GLFW directly (bypasses ghosting!)
    int state = glfwGetKey(mWindow, key);
    return state == GLFW_PRESS;
}

void InputManager::handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    mWindow = window;
    if (key < 0 || key > GLFW_KEY_LAST) return;

    if (action == GLFW_PRESS) {
        keys[key] = true;
    } else if (action == GLFW_RELEASE) {
        keys[key] = false;
    }

    KeyEvent event = Keyboard{window, key, scancode, action, mods};

    for (auto* listener : instance().mKeyListener) {
        listener->onKey(event);
    }
}
