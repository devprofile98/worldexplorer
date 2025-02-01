cmake_minimum_required(VERSION 3.0...3.25)


project(
    LearnWebgpu
    VERSION 0.1.0
    LANGUAGES CXX C
)

add_subdirectory(imgui)
add_subdirectory(webgpu)

# include_directories()
# target_link_libraries(App PRIVATE webgpu)

# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    App 

    main.cpp
    wgpu_utils.cpp
    application.cpp
    utils.cpp
    texture.cpp
    binding_group.cpp
    camera.cpp
    model.cpp
    point_light.cpp
    pipeline.cpp
    skybox.cpp
    shadow_pass.cpp
    gpu_buffer.cpp
    shapes.cpp
)

target_compile_definitions(App PRIVATE RESOURCE_DIR="./resources")
# target_compile_definitions(App PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

target_include_directories(App PUBLIC "${CMAKE_SOURCE_DIR}/webgpu")
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
target_link_libraries(App imgui)


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
