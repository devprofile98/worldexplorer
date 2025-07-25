cmake_minimum_required(VERSION 3.0...3.25)


project(
	WorldExplorer
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF) #

add_subdirectory(imgui)
add_subdirectory(webgpu)

# target_link_libraries(App PRIVATE webgpu)

# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 # set(CMAKE_BUILD_TYPE Release)
 set(CMAKE_BUILD_TYPE Debug)


add_executable(
    App 

    src/main.cpp
    src/wgpu_utils.cpp
    src/application.cpp
    src/utils.cpp
    src/texture.cpp
    src/binding_group.cpp
    src/camera.cpp
    src/model.cpp
    src/point_light.cpp
    src/pipeline.cpp
    src/skybox.cpp
    src/shadow_pass.cpp
    src/gpu_buffer.cpp
    src/shapes.cpp
    src/transparency_pass.cpp
    src/composition_pass.cpp
    src/mesh.cpp
    src/instance.cpp
    src/frustum_culling.cpp
    src/terrain_pass.cpp
    src/model_registery.cpp
    src/renderpass.cpp
    src/water_pass.cpp

    src/editor.cpp
    src/input_manager.cpp

    src/tree.cpp
)

target_compile_definitions(App PRIVATE RESOURCE_DIR="./resources")
# target_compile_definitions(App PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

target_include_directories(App PUBLIC  "imgui/" "include" "${CMAKE_SOURCE_DIR}/webgpu")
# target_link_libraries(App PRIVATE wgpu_native_static)

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# target_copy_webgpu_binaries(App)
target_link_libraries(App /home/ahmad/Documents/project/cpp/wgputest/webgpu/libwgpu_native.so glfw glfw3webgpu)
target_link_libraries(App imgui assimp)


if (EMSCRIPTEN)
set_target_properties(App PROPERTIES SUFFIX ".html")
endif()

if (NOT EMSCRIPTEN)
add_subdirectory(glfw)
else()
# Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
add_library(glfw INTERFACE)
target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# target_copy_webgpu_binaries(App)

add_subdirectory(glfw3webgpu)

# target_link_libraries(App PRIVATE webgpu )
