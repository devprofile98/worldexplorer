# webgpu/CMakeLists.txt

# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define a library target for the static library
add_library(wgpu_native_static STATIC IMPORTED)
set_target_properties(wgpu_native_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libwgpu_native.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_compile_options(-g)

# Define a library target for the shared library
add_library(wgpu_native_shared SHARED IMPORTED)
set_target_properties(wgpu_native_shared PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libwgpu_native.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Optionally, create an alias target for easier linking in other CMakeLists
add_library(wgpu_native ALIAS wgpu_native_shared)

# Optionally: specify usage requirements
# target_include_directories(wgpu_native INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# If you only want to link the header files without the actual libraries, use the following:
# target_include_directories(webgpu_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
