#ifndef TEST_WGPU_UTILS_H
#define TEST_WGPU_UTILS_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

namespace fs = std::filesystem;
bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData,
                  size_t dimensions);

// bool loadGeometryFromObj(const fs::path& path, std::vector<VertexAttributes>& vertexData);

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device);

#endif  //  TEST_WGPU_UTILS_H