
#include <GLFW/glfw3.h>

#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>

#include "application.h"

#define expose
expose bool no_texture = false;

int main(int argc, char** argv) {
    // At the end of the program, destroy the window
    std::cout << argc << argv[0] << "\n" << std::filesystem::current_path() << std::endl;
    std::string world_file;
    if (argc < 2) {
        std::cout << "Default to loading rc://world.json\n";
        world_file = "world.json";
    } else {
        world_file = argv[1];
    }

    if (argc > 2 && strcmp(argv[2], "--no-texture") == 0) {
        no_texture = true;
    }

    Application app{argv[0], world_file};

    if (app.initialize("World Explorer", 1920 * 4 / 4, 1080 * 4 / 4)) {
        while (app.isRunning()) {
            app.mainLoop();
        }
        std::cout << "Engine Closed!" << std::endl;

        app.terminate();
        return 0;
    } else {
    }
    return 1;
}
