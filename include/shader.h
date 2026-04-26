
#ifndef WORLD_EXPLORER_SHADER_H
#define WORLD_EXPLORER_SHADER_H

#include <filesystem>

#include "webgpu/webgpu.h"

namespace fs = std::filesystem;

std::string readFile(const fs::path& path);
WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device);

#endif  // !WORLD_EXPLORER_SHADER_H
