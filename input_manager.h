
#ifndef WORLD_EXPLORER_INPUT_MANAGER_H
#define WORLD_EXPLORER_INPUT_MANAGER_H

#include <unordered_map>
#include <variant>
#include <vector>

#include "GLFW/glfw3.h"

class Screen;

struct Move {
        GLFWwindow* window;
        double xPos;
        double yPos;
};
struct Click {
        GLFWwindow* window;
        double xPos;
        double yPos;
        int click;
        int action;
        int mods;
};
using MouseEvent = std::variant<Click, Move>;

struct MouseMoveListener {
        virtual void onMouseMove(MouseEvent event) { (void)event; };
};

struct MouseButtonListener {
        virtual void onMouseClick(MouseEvent event) { (void)event; };
};

class InputManager {
    public:
        static InputManager& instance();
        static void handleMouseMove(GLFWwindow* window, double xPos, double yPos);
        static void handleButton(GLFWwindow* window, int click, int action, int mods);
        void setCursorPosition(GLFWwindow* window, double xPos, double yPos);
        std::vector<MouseMoveListener*> mMouseMoveListeners;
        std::vector<MouseButtonListener*> mMouseButtonListeners;

    private:
        explicit InputManager();
        // Screen* screen;
};

#endif  //! WORLD_EXPLORER_INPUT_MANAGER_H
