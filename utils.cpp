#include "utils.h"

#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION  // add this to exactly 1 of your C++ files
#include "tinyobjloader/tiny_obj_loader.h"

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData,
                  size_t dimensions) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open file at " << path.c_str() << std::endl;
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section {
        None,
        Points,
        Indices,
    };

    Section current_section = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof()) {
        getline(file, line);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "[points]") {
            current_section = Section::Points;
        } else if (line == "[indices]") {
            current_section = Section::Indices;
        } else if (line[0] == '#' || line.empty()) {
            // doing nothing
        } else if (current_section == Section::Points) {
            std::istringstream iss{line};
            for (size_t i = 0; i < dimensions + 3; i++) {
                iss >> value;
                pointData.push_back(value);
            }
        } else if (current_section == Section::Indices) {
            std::istringstream iss{line};
            for (int i = 0; i < 3; i++) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

WGPUShaderModule loadShader(const fs::path& path, WGPUDevice device) {
    std::ifstream file{path};
    if (!file.is_open()) {
        std::cout << "Failed to open shader at " << path.c_str() << std::endl;
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    std::string shader_code(file_size, ' ');  // what the fuck!
    file.seekg(0);
    file.read(shader_code.data(), file_size);

    WGPUShaderModuleWGSLDescriptor module_descriptor = {};
    module_descriptor.chain.next = nullptr;
    module_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    module_descriptor.code = shader_code.c_str();

    WGPUShaderModuleDescriptor shader_descriptor = {};
    shader_descriptor.nextInChain = nullptr;
    shader_descriptor.hintCount = 0;
    shader_descriptor.hints = nullptr;
    shader_descriptor.nextInChain = &module_descriptor.chain;
    return wgpuDeviceCreateShaderModule(device, &shader_descriptor);
}

bool loadGeometryFromObj(const fs::path& path, std::vector<VertexAttributes>& vertexData) {
    // tinyobj::attrib_t attrib = {};
    // std::vector<tinyobj::shape_t> shapes;
    // std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = "/home/ahmad/Documents/project/cpp/wgputest/resources";

    if (!reader.ParseFromFile(path.string().c_str(), reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    // auto& materials = reader.GetMaterials();

    // bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());
    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    // if (!ret) {
    //     return false;
    // }

    // Fill in vertexData here
    const auto& shape = shapes[0];
    vertexData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        vertexData[i].position = {attrib.vertices[3 * idx.vertex_index + 0], -attrib.vertices[3 * idx.vertex_index + 2],
                                  attrib.vertices[3 * idx.vertex_index + 1]};

        vertexData[i].normal = {attrib.normals[3 * idx.normal_index + 0], -attrib.normals[3 * idx.normal_index + 2],
                                attrib.normals[3 * idx.normal_index + 1]};

        vertexData[i].color = {attrib.colors[3 * idx.vertex_index + 0], attrib.colors[3 * idx.vertex_index + 1],
                               attrib.colors[3 * idx.vertex_index + 2]};

        vertexData[i].uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                            1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
    }

    return true;
}