#include "input_manager.h"

#include "GLFW/glfw3.h"
#include "editor.h"

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
    (void)window;
    (void)click;
    (void)action;
    (void)mods;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    MouseEvent event = Click{window, xpos, ypos, click, action, mods};
    std::cout << "sdfsdfsdf " << instance().mMouseButtonListeners.size() << " " << click << " "
              << (std::get<Click>(event).click) << std::endl;

    for (auto* listener : instance().mMouseButtonListeners) {
        listener->onMouseClick(event);
    }
}

void InputManager::setCursorPosition(GLFWwindow* window, double xPos, double yPos) {
    glfwSetCursorPos(window, xPos, yPos);
}
