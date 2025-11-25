cmake_minimum_required(VERSION 3.10...3.25)

set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

project(
	WorldExplorer
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF) #

add_subdirectory(webgpu)
add_subdirectory(extern/assmip)


# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
 set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 # set(CMAKE_BUILD_TYPE Release)
 set(CMAKE_BUILD_TYPE Debug)
 # add_definitions(-DTRACY_ENABLE)
set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiling in Debug builds" FORCE)

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

# The configurations we support
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")
set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS ON)
set(OVERRIDE_CXX_FLAGS ON)
set(CROSS_PLATFORM_DETERMINISTIC OFF)
set(INTERPROCEDURAL_OPTIMIZATION ON)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(CPP_EXCEPTIONS_ENABLED OFF)
set(CPP_RTTI_ENABLED OFF)
set(OBJECT_LAYER_BITS 16)
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)
# Include Jolt
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "v5.4.0"
		SOURCE_SUBDIR "Build"
)
FetchContent_MakeAvailable(JoltPhysics)


if (TRACY_ENABLE)
    file(GLOB TRACY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tracy_external/TracyClient.cpp"
)
endif()

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
    src/window.cpp
    src/physics.cpp

    src/editor.cpp
    src/input_manager.cpp
    src/animation.cpp
    src/world.cpp

    src/tree.cpp

    ${TRACY_SOURCES}
    
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/glfw3webgpu/glfw3webgpu.c"
)


if (UNIX AND NOT APPLE)
    # For Linux/Unix systems
    if (DEFINED ENV{WAYLAND_DISPLAY})
        # Prefer Wayland if WAYLAND_DISPLAY is set
        target_compile_definitions(App PRIVATE
            GLFW_EXPOSE_NATIVE_WAYLAND
            _GLFW_WAYLAND
        )
    else()
        # Default to X11
        target_compile_definitions(App PRIVATE
            GLFW_EXPOSE_NATIVE_X11
            _GLFW_X11
        )
    endif()
elseif (APPLE)
    # For macOS
    target_compile_definitions(App PRIVATE
        GLFW_EXPOSE_NATIVE_COCOA
        _GLFW_COCOA
    )
elseif (WIN32)
    # For Windows
    target_compile_definitions(App PRIVATE
        GLFW_EXPOSE_NATIVE_WIN32
        _GLFW_WIN32
    )
else()
    message(FATAL_ERROR "Unsupported platform for glfw3webgpu")
endif()


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND CMAKE_BUILD_TYPE MATCHES Debug)
    # Set compile flags
    # target_compile_options(App PRIVATE -fsanitize=address)
    # target_link_options(App PRIVATE -fsanitize=address)
#     target_compile_options(App PRIVATE -S -fverbose-asm)
#     # For MSVC, the flag is /Fa
#     target_compile_options(App PRIVATE /Fa)

endif()


target_compile_definitions(App PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
# Adding ImGui include directories
target_include_directories(App PRIVATE
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends" # Include backends if headers are there
)

target_compile_definitions(App PRIVATE RESOURCE_DIR="./resources")

target_include_directories(App PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}"
                                       "extern/imgui"
                                       "extern/glfw3webgpu"
                                       "include"
                                       "tracy_external"
                                   )

set_target_properties(App PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR OFF
)

add_library(wgpu_native SHARED IMPORTED)
if (MSVC)
    target_compile_options(App PRIVATE /W4)
    # target_link_libraries(App "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/wgpu_native.dll" glfw assimp glm::glm)
    set_target_properties(wgpu_native PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/wgpu_native.dll"
        IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/wgpu_native.lib"
    )

    add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
    set(OS_LIBRARIES d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)
    target_link_libraries(App PRIVATE wgpu_native glfw assimp glm::glm ${OS_LIBRARIES})

else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
    # target_link_libraries(App "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/libwgpu_native.so" glfw assimp glm::glm)
    set_target_properties(wgpu_native PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/webgpu/libwgpu_native.so"
    )

    target_include_directories(App PUBLIC ${JoltPhysics_SOURCE_DIR}/..)
    target_link_libraries(App PRIVATE wgpu_native glfw assimp glm::glm Jolt)

endif()

# target_copy_webgpu_binaries(App)
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


# Enable profiling
# if (CMAKE_BUILD_TYPE MATCHES "Debug")
#     # Adds the -pg flag for both compiler and linker
#     target_compile_options(App PRIVATE -pg)
#     target_link_options(App PRIVATE -pg)
# endif()
