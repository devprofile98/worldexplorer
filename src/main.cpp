
#include <GLFW/glfw3.h>

#include <cassert>
#include <chrono>
#include <format>
#include <iostream>
#include <thread>
#include <vector>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"
#include "application.h"
#include "wgpu_utils.h"

int main(int, char**) {
    // At the end of the program, destroy the window

    Application app;
    if (app.initialize()) {
        while (app.isRunning()) {
            std::cout << "What the fuck!" << std::endl;
            app.mainLoop();
        }

        app.terminate();
        return 0;
    } else {
    }
    return 1;
}
