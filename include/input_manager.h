
#ifndef WORLD_EXPLORER_INPUT_MANAGER_H
#define WORLD_EXPLORER_INPUT_MANAGER_H

#include <unordered_map>
#include <variant>
#include <vector>

#include "GLFW/glfw3.h"

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

struct Scroll {
        GLFWwindow* window;
        double xOffset;
        double yOffset;
};

using MouseEvent = std::variant<Click, Move, Scroll>;

struct Keyboard {
        GLFWwindow* window;
        int key;
        int scancode;
        int action;
        int mods;
};

using KeyEvent = std::variant<Keyboard>;

struct MouseMoveListener {
        virtual void onMouseMove(MouseEvent event) { (void)event; };
};

struct MouseButtonListener {
        virtual void onMouseClick(MouseEvent event) { (void)event; };
};

struct MouseScrollListener {
        virtual void onMouseScroll(MouseEvent event) { (void)event; };
};

struct KeyboardListener {
        virtual void onKey(KeyEvent event) { (void)event; };
};

class InputManager {
    public:
        static InputManager& instance();
        static void handleMouseMove(GLFWwindow* window, double xPos, double yPos);
        static void handleButton(GLFWwindow* window, int click, int action, int mods);
        static void handleScroll(GLFWwindow* window, double xOffset, double yOffset);
        static void handleKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

        void setCursorPosition(GLFWwindow* window, double xPos, double yPos);
        std::vector<MouseMoveListener*> mMouseMoveListeners = {};
        std::vector<MouseButtonListener*> mMouseButtonListeners = {};
        std::vector<MouseScrollListener*> mMouseScrollListeners = {};
        std::vector<KeyboardListener*> mKeyListener = {};

    private:
        explicit InputManager();
};

#endif  //! WORLD_EXPLORER_INPUT_MANAGER_H
