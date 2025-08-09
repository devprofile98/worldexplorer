cmake_minimum_required(VERSION 3.10...3.25)

set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

project(
	WorldExplorer
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF) #

add_subdirectory(webgpu)

# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 # set(CMAKE_BUILD_TYPE Release)
 set(CMAKE_BUILD_TYPE Debug)



 # configuring imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui")
set(IMGUI_BACKENDS_DIR "${IMGUI_DIR}/backends") # Assuming backends are inside imgui/backends

file(GLOB IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
)

file(GLOB IMGUI_BACKEND_SOURCES
    "${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp"
    "${IMGUI_BACKENDS_DIR}/imgui_impl_wgpu.cpp" # Or imgui_impl_vulkan.cpp, imgui_impl_dx11.cpp, etc.
)


# Fetch Glm
include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)


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
    src/shader.cpp

    src/editor.cpp
    src/input_manager.cpp

    src/tree.cpp
    
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw3webgpu/glfw3webgpu.c"
)

target_compile_definitions(App PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
# Adding ImGui include directories
target_include_directories(App PRIVATE
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends" # Include backends if headers are there
)

# set(WEBGPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/webgpu")

target_compile_definitions(App PRIVATE RESOURCE_DIR="./resources")

target_include_directories(App PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}"
                                       "extern/imgui"
                                       "extern/glfw3webgpu"
                                       "include"
                                       
                                   )

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR OFF
)

if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# target_copy_webgpu_binaries(App)
target_link_libraries(App "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/libwgpu_native.so" glfw assimp glm::glm)
target_link_libraries(App )


if (EMSCRIPTEN)
set_target_properties(App PROPERTIES SUFFIX ".html")
endif()

if (NOT EMSCRIPTEN)
    add_subdirectory(extern/glfw)
else()
# Emscripten has built-in support for GLFW but requires the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3)
endif()
# add_subdirectory(extern/glfw3webgpu)
