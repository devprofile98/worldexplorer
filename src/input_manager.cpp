#include "input_manager.h"

#include "GLFW/glfw3.h"
#include "editor.h"
#include "utils.h"

InputManager::InputManager() {}

InputManager& InputManager::instance() {
    static InputManager inst;
    return inst;
}

void InputManager::handleMouseMove(GLFWwindow* window, double xPos, double yPos) {
    MouseEvent event = Move{window, xPos, yPos};
    for (auto* listener : instance().mMouseMoveListeners) {
        listener->onMouseMove(event);
    }
}

void InputManager::handleButton(GLFWwindow* window, int click, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    MouseEvent event = Click{window, xpos, ypos, click, action, mods};

    for (auto* listener : instance().mMouseButtonListeners) {
        listener->onMouseClick(event);
    }
}

void InputManager::handleScroll(GLFWwindow* window, double xOffset, double yOffset) {
    MouseEvent event = Scroll{window, xOffset, yOffset};
    for (auto* listener : instance().mMouseScrollListeners) {
        listener->onMouseScroll(event);
    }
}

void InputManager::setCursorPosition(GLFWwindow* window, double xPos, double yPos) {
    glfwSetCursorPos(window, xPos, yPos);
}

void InputManager::handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    KeyEvent event = Keyboard{window, key, scancode, action, mods};
    for (auto* listener : instance().mKeyListener) {
        listener->onKey(event);
    }
}
