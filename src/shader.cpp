#include "shader.h"

#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

static std::map<std::string, std::string> sShaderStorage;

// const char* binding = R"(
// @group(0) @binding(0) var<uniform> uMyUniform: array<MyUniform, 10>;
// @group(0) @binding(1) var<uniform> lightCount: i32;
// @group(0) @binding(2) var textureSampler: sampler;
// @group(0) @binding(3) var<uniform> lightingInfos: LightingUniforms;
// @group(0) @binding(4) var<uniform> pointLight: array<PointLight,10>;
// @group(0) @binding(5) var near_depth_texture: texture_2d<f32>;
// @group(0) @binding(6) var grass_ground_texture: texture_2d<f32>;
// @group(0) @binding(7) var rock_mountain_texture: texture_2d<f32>;
// @group(0) @binding(8) var sand_lake_texture: texture_2d<f32>;
// @group(0) @binding(9) var snow_mountain_texture: texture_2d<f32>;
// @group(0) @binding(10) var depth_texture: texture_depth_2d_array;
// @group(0) @binding(11) var<uniform> lightSpaceTrans: array<Scene, 5>;
// @group(0) @binding(12) var shadowMapSampler: sampler_comparison;
// @group(0) @binding(13) var<storage, read> offsetInstance: array<OffsetData>;
// @group(0) @binding(14) var<uniform> numOfCascades: u32;
// )";

std::string readFile(const fs::path& path) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open shader at " << path.c_str() << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();

    std::string shader_code(file_size, ' ');

    file.seekg(0);
    file.read(shader_code.data(), file_size);
    return {shader_code};
}

void preprocessShader(std::string& shaderCode) {
    size_t find_index = shaderCode.find("#include");
    while (find_index != std::string::npos) {
        auto start_idx = shaderCode.find('"', find_index + 1);
        auto end_idx = shaderCode.find('"', start_idx + 1);
        std::string shader_name = shaderCode.substr(start_idx + 1, end_idx - start_idx - 1);

        // std::cout << ".." << find_index << " from " << start_idx << " to " << end_idx << " " << end_idx - find_index
        // - 1
        //           << " " << shaderCode.substr(start_idx + 1, end_idx - start_idx - 1) << std::endl;
        std::cout << std::format("loading {} in preprocessing from {}\n", shader_name,
                                 std::string{""} + RESOURCE_DIR + "/shaders/" + shader_name);
        std::string file_content = readFile(std::string{""} + RESOURCE_DIR + "/shaders/" + shader_name);

        shaderCode.replace(find_index, end_idx - find_index + 1, file_content);
        find_index = shaderCode.find("#include", find_index + 1);
    }
    // std::cout << "---------------------------------------\n";
    // std::cout << shaderCode << std::endl;
}

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device) {
    std::string shader_code = readFile(path);
    preprocessShader(shader_code);
    WGPUShaderSourceWGSL module_descriptor = {};
    module_descriptor.chain.next = nullptr;
    module_descriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    module_descriptor.code = {shader_code.c_str(), shader_code.size()};

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = nullptr;
    shader_descriptor.nextInChain = &module_descriptor.chain;
    return wgpuDeviceCreateShaderModule(device, &shader_descriptor);
}
