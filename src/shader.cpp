#include "shader.h"

#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

static std::map<std::string, std::string> sShaderStorage;

std::string readFile(const fs::path& path) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open shader at " << path << std::endl;
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();

    std::string shader_code(file_size, ' ');

    file.seekg(0);
    file.read(shader_code.data(), file_size);
    return {shader_code};
}

void preprocessShader(std::string& shaderCode, std::filesystem::path basePath) {
    size_t find_index = shaderCode.find("#include");
    while (find_index != std::string::npos) {
        auto start_idx = shaderCode.find('"', find_index + 1);
        auto end_idx = shaderCode.find('"', start_idx + 1);
        std::string shader_name = shaderCode.substr(start_idx + 1, end_idx - start_idx - 1);

        std::cout << std::format("loading {} in preprocessing from {}\n", shader_name,
                                 basePath.string() + "/" + shader_name);
        std::string file_content = readFile(basePath.string() + "/" + shader_name);

        shaderCode.replace(find_index, end_idx - find_index + 1, file_content);
        find_index = shaderCode.find("#include", find_index + 1);
    }
}

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device) {
    auto base_path = path.string().substr(0, path.string().find_last_of("/"));
    std::string shader_code = readFile(path);
    preprocessShader(shader_code, base_path);
    WGPUShaderSourceWGSL module_descriptor = {};
    module_descriptor.chain.next = nullptr;
    module_descriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    module_descriptor.code = {shader_code.c_str(), shader_code.size()};

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = nullptr;
    shader_descriptor.nextInChain = &module_descriptor.chain;
    return wgpuDeviceCreateShaderModule(device, &shader_descriptor);
}
