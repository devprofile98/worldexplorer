#include "model.h"

#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "application.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "webgpu.h"
#include "webgpu/webgpu.h"

const char* model_name = "cube1";

Drawable::Drawable() {}

void Drawable::configure(Application* app) {
    mUniformBuffer.setLabel("Uniform buffer for object info")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(ObjectInfo))
        .setMappedAtCraetion()
        .create(app);
}

void Drawable::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)app;
    (void)encoder;
    (void)bindingData;
}

Buffer& Drawable::getUniformBuffer() { return mUniformBuffer; }

Model::Model() : BaseModel() {
    mScaleMatrix = glm::scale(mScaleMatrix, mScale);
    mTransformMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    mObjectInfo.transformation = mTransformMatrix;
    mObjectInfo.isFlat = 0;
    mObjectInfo.useTexture = 1;
    mObjectInfo.isFoliage = 0;
}

glm::mat3x3 computeTBN(VertexAttributes* vertex, const glm::vec3& expectedN) {
    const auto& pos1 = vertex[0].position;
    const auto& pos2 = vertex[1].position;
    const auto& pos3 = vertex[2].position;

    const auto& uv1 = vertex[0].uv;
    const auto& uv2 = vertex[1].uv;
    const auto& uv3 = vertex[2].uv;

    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent{};
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    glm::vec3 bitangent{};
    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    glm::vec3 cnormal = glm::cross(tangent, bitangent);
    /*const auto& expectedN = mMeshes[materialId].mVertexData[i].normal;*/

    if (glm::dot(expectedN, cnormal) < 0.0) {
        tangent = -tangent;
        bitangent = -bitangent;
        cnormal = -cnormal;
    }

    /*cnormal = expectedN;*/
    /*tangent = normalize(tangent - dot(tangent, cnormal) * cnormal);*/
    /*bitangent = cross(cnormal, tangent);*/

    return glm::mat3x3(tangent, bitangent, cnormal);
}

void Model::processNode(Application* app, aiNode* node, const aiScene* scene) {
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        // if (getName() == model_name) {
        //     std::cout << "Number of meshes for Steampunk is " << node->mNumMeshes << " " << mesh->mMaterialIndex << "
        //     "
        //               << node->mName.C_Str() << std::endl;
        // }
        processMesh(app, mesh, scene);
    }
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        processNode(app, node->mChildren[i], scene);
    }
}

void Model::processMesh(Application* app, aiMesh* mesh, const aiScene* scene) {
    (void)scene;

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    size_t index_offset = mMeshes[mesh->mMaterialIndex].mVertexData.size();
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        VertexAttributes vertex;
        glm::vec3 vector;

        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].z;
        vector.z = mesh->mVertices[i].y;

        min.x = std::min(min.x, vector.x);
        min.y = std::min(min.y, vector.y);
        min.z = std::min(min.z, vector.z);
        /**/
        max.x = std::max(max.x, vector.x);
        max.y = std::max(max.y, vector.y);
        max.z = std::max(max.z, vector.z);

        vertex.position = vector;

        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].z;
            vector.z = mesh->mNormals[i].y;
            vertex.normal = vector;
        }

        aiColor4D baseColor(1.0f, 0.0f, 1.0f, 1.0f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        vertex.color = glm::vec3(baseColor.r, baseColor.g, baseColor.b);

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.uv = vec;
            if (mesh->mTangents && mesh->mBitangents) {
                // // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].z;
                vector.z = mesh->mTangents[i].y;
                vertex.tangent = vector;

                // // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].z;
                vector.z = mesh->mBitangents[i].y;
                vertex.biTangent = vector;
            }
        } else {
            vertex.uv = glm::vec2{0.0f, 0.0f};
        }

        /*vertices.push_back(vertex);*/
        mMeshes[mesh->mMaterialIndex].mVertexData.push_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            /*indices.push_back(face.mIndices[j]);*/
            mMeshes[mesh->mMaterialIndex].mIndexData.push_back((uint32_t)face.mIndices[j] + index_offset);
        }
    }
    // if (getName() == model_name) {
    //     std::cout << std::format(" -------------- Assimp - Succesfully loaded mesh at {}\n", mesh->mMaterialIndex);
    // }

    mObjectInfo.setFlag(MaterialProps::HasNormalMap, false);
    mObjectInfo.setFlag(MaterialProps::HasRoughnessMap, false);
    mObjectInfo.setFlag(MaterialProps::HasDiffuseMap, false);

    auto& render_resource = app->getRendererResource();
    auto& mmesh = mMeshes[mesh->mMaterialIndex];
    if (mmesh.mTexture == nullptr) {
        for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
            mObjectInfo.setFlag(MaterialProps::HasDiffuseMap, true);
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, i, &str);
            /*std::cout << "texture for this part is at " << str.C_Str() << std::endl;*/
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += str.C_Str();
            mmesh.mTexture = new Texture{render_resource.device, texture_path};
            if (mmesh.mTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create diffuse Texture view for {} at {}\n", mName, texture_path);
            }
            mmesh.mTexture->uploadToGPU(render_resource.queue);
            mmesh.isTransparent = mmesh.mTexture->isTransparent();
        }
    }
    if (mmesh.mSpecularTexture == nullptr) {
        for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); i++) {
            mObjectInfo.setFlag(MaterialProps::HasRoughnessMap, true);
            aiString str;
            material->GetTexture(aiTextureType_SPECULAR, i, &str);
            /*std::cout << "texture for this part is at " << str.C_Str() << std::endl;*/
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += str.C_Str();
            mmesh.mSpecularTexture = new Texture{render_resource.device, texture_path};
            if (mmesh.mSpecularTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create diffuse Texture view for {} at {}\n", mName, texture_path);
            }
            mmesh.mSpecularTexture->uploadToGPU(render_resource.queue);
            mmesh.isTransparent = mmesh.mSpecularTexture->isTransparent();
        }
    }
    if (mmesh.mNormalMapTexture == nullptr) {
        for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_HEIGHT); i++) {
            mObjectInfo.setFlag(MaterialProps::HasNormalMap, true);
            aiString str;
            material->GetTexture(aiTextureType_HEIGHT, i, &str);
            /*std::cout << "texture for this part is at " << str.C_Str() << std::endl;*/
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += str.C_Str();
            mmesh.mNormalMapTexture = new Texture{render_resource.device, texture_path};
            if (mmesh.mNormalMapTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create diffuse Texture view for {} at {}\n", mName, texture_path);
            }
            mmesh.mNormalMapTexture->uploadToGPU(render_resource.queue);
            mmesh.isTransparent = mmesh.mNormalMapTexture->isTransparent();
        }
    }
    /*for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {*/
    /*    aiString str;*/
    /*    material->GetTexture(aiTextureType_DIFFUSE, i, &str);*/
    /*    std::cout << "texture for this part is at " << str.C_Str() << std::endl;*/
    /*}*/
    /*for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {*/
    /*    aiString str;*/
    /*    material->GetTexture(aiTextureType_DIFFUSE, i, &str);*/
    /*    std::cout << "texture for this part is at " << str.C_Str() << std::endl;*/
    /*}*/

    /*std::vector<Texture_INT> diffuseMap = loadMaterialTexture(material, aiTextureType_DIFFUSE,
     * "texture_diffuse");*/
    /*textures.insert(textures.end(), diffuseMap.begin(), diffuseMap.end());*/
    /**/
    /*std::vector<Texture_INT> specularMap = loadMaterialTexture(material, aiTextureType_SPECULAR,
     * "texture_specular");*/
    /*textures.insert(textures.end(), specularMap.begin(), specularMap.end());*/
    /*if (!specularMap.empty()) {*/
    /*    this->material.hasSpecular = true;*/
    /*}*/
    /*// 3. normal maps*/
    /*std::vector<Texture_INT> normalMaps = loadMaterialTexture(material, aiTextureType_HEIGHT, "texture_normal");*/
    /*textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());*/
    /*if (!normalMaps.empty()) {*/
    /*    this->material.hasBumpMap = true;*/
    /*}*/
    /*// 4. height maps*/
    /*std::vector<Texture_INT> heightMaps = loadMaterialTexture(material, aiTextureType_AMBIENT,
     * "texture_height");*/
    /*textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());*/
    /**/
    /*if (textures.size() <= 0) {*/
    /*    mat.should_draw = true;*/
    /*}*/

    // return a mesh object created from the extracted mesh data
    /*return Mesh(vertices, indices, textures, mat);*/
}

Model& Model::load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout) {
    // load model from disk
    (void)layout;
    mName = name;

    Drawable::configure(app);

    std::string warn;
    std::string err;
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = "/home/ahmad/Documents/project/cpp/wgputest/resources";

    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(
        path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        // std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        std::cout << std::format("Assimp - Error while loading model {} : {}\n", (const char*)path.c_str(),
                                 import.GetErrorString());
    } else {
        std::cout << std::format("Assimp - Succesfully loaded model {}\n", (const char*)path.c_str());
    }
    // if (getName() != model_name) {
    processNode(app, scene->mRootNode, scene);
    return *this;
    // }

    if (!reader.ParseFromFile(path.string().c_str(), reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning() << '\n';
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Load and upload diffuse texture
    auto& render_resource = app->getRendererResource();

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    std::cout << getName() << " has " << materials.size() << " Tiny Object " << materials[0].bump_texname << " -- \n";

    // Fill in vertexData here

    for (const auto& shape : shapes) {
        // Iterate through faces
        for (size_t faceIdx = 0; faceIdx < shape.mesh.num_face_vertices.size(); ++faceIdx) {
            int materialId = shape.mesh.material_ids[faceIdx];        // Material ID for this face
            int numVertices = shape.mesh.num_face_vertices[faceIdx];  // Number of vertices in this face

            // Iterate through vertices in the face
            for (int v = 0; v < numVertices; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[faceIdx * 3 + v];  // Vertex index
                VertexAttributes vertex;

                vertex.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.position[2] = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.position[1] = attrib.vertices[3 * idx.vertex_index + 2];
                /**/
                // calculating Tangent and biTangent

                /**/
                min.x = std::min(min.x, vertex.position.x);
                min.y = std::min(min.y, vertex.position.y);
                min.z = std::min(min.z, vertex.position.z);
                /**/
                max.x = std::max(max.x, vertex.position.x);
                max.y = std::max(max.y, vertex.position.y);
                max.z = std::max(max.z, vertex.position.z);
                /**/
                vertex.normal = {attrib.normals[3 * idx.normal_index + 0], -attrib.normals[3 * idx.normal_index + 2],
                                 attrib.normals[3 * idx.normal_index + 1]};

                glm::vec3 materialColor = {1.0f, 1.0f, 1.0f};  // Default color (white)
                if (materialId >= 0 && materialId < (int)materials.size()) {
                    materialColor = {materials[materialId].diffuse[0], materials[materialId].diffuse[1],
                                     materials[materialId].diffuse[2]};
                    /*mObjectInfo.useTexture = 1;*/
                }
                vertex.color = materialColor;

                if (attrib.texcoords.empty()) {
                    vertex.uv = {0.0, 0.0};
                } else {
                    vertex.uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                                 1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
                }
                mMeshes[materialId].mVertexData.push_back(vertex);
            }
        }
    }

    for (auto& pair : mMeshes) {  // Iterate through each mesh (by materialId)
        int materialId = pair.first;
        /*Mesh& currentMesh = pair.second;  // Get a reference to the actual Mesh object*/

        for (size_t i = 0; i < mMeshes[materialId].mVertexData.size(); i += 3) {
            for (size_t k = 0; k < 3; k++) {
                VertexAttributes* v = &mMeshes[materialId].mVertexData[i];
                auto tbn = computeTBN(v, v[k].normal);
                v[k].tangent = tbn[0];
                v[k].biTangent = tbn[1];
                v[k].normal = tbn[2];
            }
        }
    }
    if (getName() == "jet") {
        mObjectInfo.isFlat = false;
    }

    for (const auto& material : materials) {
        size_t material_id = &material - &materials[0];
        if (material_id > mMeshes.size() - 1) {
            break;
        }
        auto& mesh = mMeshes[material_id];
        if (!material.diffuse_texname.empty()) {
            mObjectInfo.setFlag(MaterialProps::HasDiffuseMap, true);
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += material.diffuse_texname;
            mesh.mTexture = new Texture{render_resource.device, texture_path};
            if (mesh.mTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create diffuse Texture view for {} at {}\n", mName, texture_path);
            }
            mesh.mTexture->uploadToGPU(render_resource.queue);
            mesh.isTransparent = mesh.mTexture->isTransparent();
        }

        // Load and upload specular texture
        if (!materials[material_id].specular_texname.empty()) {
            mObjectInfo.setFlag(MaterialProps::HasRoughnessMap, true);
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += materials[material_id].specular_texname;
            mesh.mSpecularTexture = new Texture{render_resource.device, texture_path};
            if (mesh.mSpecularTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create Specular Texture view for {}\n", mName);
            }
            mesh.mSpecularTexture->uploadToGPU(render_resource.queue);
        }

        // Load and upload normal texture
        if (!materials[material_id].bump_texname.empty()) {
            mObjectInfo.setFlag(MaterialProps::HasNormalMap, true);
            /*if (!materials[0].normal_texname.empty()) {*/
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += materials[material_id].bump_texname;
            mesh.mNormalMapTexture = new Texture{render_resource.device, texture_path};
            if (mesh.mNormalMapTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create normal Texture view for {} at {}\n", mName, texture_path);
            }
            mesh.mNormalMapTexture->uploadToGPU(render_resource.queue);
            /*}*/
        }
    }

    offset_buffer.setSize(sizeof(glm::vec4) * 10)
        .setLabel("offset buffer")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setMappedAtCraetion()
        .create(app);

    std::array<glm::vec4, 10> dddata = {};
    for (size_t i = 0; i < 10; i++) {
        dddata[i] = glm::vec4{0.0, i * 2, 0.0, 0.0};
    }
    wgpuQueueWriteBuffer(app->getRendererResource().queue, offset_buffer.getBuffer(), 0, &dddata,
                         sizeof(glm::vec4) * 10);
    /*(void)layout;*/
    return *this;
}

void BaseModel::setInstanced(Instance* instance) {
    /*this->instances = instances;*/
    this->instance = instance;
    /*return *this;*/
}

Model& Model::setFoliage() {
    this->mObjectInfo.isFoliage = 1;
    return *this;
}

Model& Model::useTexture(bool use) {
    this->mObjectInfo.useTexture = use ? 1 : 0;
    return *this;
}

Transform& Transform::moveBy(const glm::vec3& translationVec) {
    // justify position by the factor `m`
    // (void)translationVec;
    mPosition += translationVec;
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    getTranformMatrix();
    return *this;
}

Transform& Transform::moveTo(const glm::vec3& moveVec) {
    mPosition = moveVec;
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    getTranformMatrix();
    return *this;
}

Model& Model::uploadToGPU(Application* app) {
    // upload vertex attribute data to GPU
    for (auto& [_mat_id, mesh] : mMeshes) {
        std::cout << getName() << " mesh has " << mesh.mVertexData.size() << '\n';
        mesh.mVertexBuffer.setLabel("Uniform buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
            .setSize((mesh.mVertexData.size() + 1) * sizeof(VertexAttributes))
            .setMappedAtCraetion()
            .create(app);

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mVertexBuffer.getBuffer(), 0,
                             mesh.mVertexData.data(), mesh.mVertexData.size() * sizeof(VertexAttributes));

        /*if (getName() == "steampunk") {*/
        mesh.mIndexBuffer.setLabel("index buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Index)
            .setSize((mesh.mIndexData.size()) * sizeof(uint32_t))
            .setMappedAtCraetion()
            .create(app);

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndexBuffer.getBuffer(), 0, mesh.mIndexData.data(),
                             mesh.mIndexData.size() * sizeof(uint32_t));
        /*}*/
    }
    /*createSomeBinding(app);*/
    return *this;
};

size_t BaseModel::getVertexCount() const { return mMeshes.at(0).mVertexData.size(); }

/*Buffer BaseModel::getVertexBuffer() { return mVertexBuffer; }*/

Buffer BaseModel::getIndexBuffer() { return mIndexBuffer; }

void BaseModel::setTransparent(bool value) {
    for (auto& [mat_id, mesh] : mMeshes) {
        mesh.isTransparent = value;
    }
    mIsTransparent = value;
}

bool BaseModel::isTransparent() { return mIsTransparent; }

void BaseModel::selected(bool selected) { mObjectInfo.isSelected = selected; }
bool BaseModel::isSelected() const { return mObjectInfo.isSelected; }

void Model::createSomeBinding(Application* app, std::vector<WGPUBindGroupEntry> bindingData) {
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = Drawable::getUniformBuffer().getBuffer();
    mBindGroupEntry.offset = 0;
    mBindGroupEntry.size = sizeof(ObjectInfo);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = &mBindGroupEntry;
    mTrasBindGroupDesc.entryCount = 1;
    mTrasBindGroupDesc.label = "translation bind group";
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);

    for (auto& [mat_id, mesh] : mMeshes) {
        mesh.binding_data = bindingData;
        if (mesh.isTransparent) {
            continue;
        }

        if (mesh.mTexture != nullptr) {
            if (mesh.mTexture->getTextureView() != nullptr) {
                mesh.binding_data[0].nextInChain = nullptr;
                mesh.binding_data[0].binding = 0;
                mesh.binding_data[0].textureView = mesh.mTexture->getTextureView();
            }
            if (mesh.mSpecularTexture != nullptr && mesh.mSpecularTexture->getTextureView() != nullptr) {
                mesh.binding_data[1].nextInChain = nullptr;
                mesh.binding_data[1].binding = 1;
                mesh.binding_data[1].textureView = mesh.mSpecularTexture->getTextureView();
            }
            if (mesh.mNormalMapTexture != nullptr && mesh.mNormalMapTexture->getTextureView() != nullptr) {
                mesh.binding_data[2].nextInChain = nullptr;
                mesh.binding_data[2].binding = 2;
                mesh.binding_data[2].textureView = mesh.mNormalMapTexture->getTextureView();
            }
            auto& desc = app->mDefaultTextureBindingGroup.getDescriptor();
            desc.entries = mesh.binding_data.data();
            desc.entryCount = mesh.binding_data.size();
            mesh.mTextureBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &desc);
        }
    }
}

void Model::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    (void)bindingData;
    auto& render_resource = app->getRendererResource();
    WGPUBindGroup active_bind_group = nullptr;

    // Bind Diffuse texture for the model if existed
    /*std::cout << getName() << " has " << mMeshes.size() << " meshes\n";*/
    for (auto& [mat_id, mesh] : mMeshes) {
        if (mesh.isTransparent) {
            continue;
        }

        active_bind_group = app->getBindingGroup().getBindGroup();

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                             wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh.mIndexBuffer.getBuffer(), WGPUIndexFormat_Uint32, 0,
                                            wgpuBufferGetSize(mesh.mIndexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);

        wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,
                             sizeof(ObjectInfo));

        wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(encoder, 2,
                                          mesh.mTextureBindGroup == nullptr
                                              ? app->mDefaultTextureBindingGroup.getBindGroup()
                                              : mesh.mTextureBindGroup,
                                          0, nullptr);

        if (getName() == "grass") {
		wgpuRenderPassEncoderDrawIndexedIndirect(encoder,app->indirectDrawArgsBuffer.getBuffer(), 0);
        } else {
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(),
                                             this->instance == nullptr ? 1 : this->instance->getInstanceCount(), 0, 0,
                                             0);
        }
    }
}

size_t Model::getInstaceCount() { return this->instances; }

#ifdef DEVELOPMENT_BUILD
void Model::userInterface() {
    ImGuiIO& io = ImGui::GetIO();

    // Determine the drag speed based on modifier keys
    float drag_speed = 1.0f;  // Default speed
    bool lock_scale = false;
    if (io.KeyCtrl) {
        drag_speed = 0.1f;  // Finer control when Ctrl is held
    } else if (io.KeyShift) {
        lock_scale = true;
    }

    if (ImGui::CollapsingHeader("Transformations",
                                ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
        ImGui::Text("Position:");
        ImGui::DragFloat3("Position", glm::value_ptr(mPosition), drag_speed);

        if (ImGui::DragFloat("Scale x", &mScale.x, 0.01)) {
            if (lock_scale) {
                mScale = glm::vec3{mScale.x};
            }
        }
        if (ImGui::DragFloat("Scale y", &mScale.y, 0.01)) {
            if (lock_scale) {
                mScale = glm::vec3{mScale.y};
            }
        }
        if (ImGui::DragFloat("Scale z", &mScale.z, 0.01)) {
            if (lock_scale) {
                mScale = glm::vec3{mScale.z};
            }
        }

        if (ImGui::DragFloat3("Rotation", glm::value_ptr(mEulerRotation), drag_speed, 360.0f)) {
            glm::vec3 euler_radians = glm::radians(mEulerRotation);
            this->mRotationMatrix = glm::toMat4(glm::quat(euler_radians));

            if (mEulerRotation.x < 0) mEulerRotation.x += 360.0f;
            if (mEulerRotation.y < 0) mEulerRotation.y += 360.0f;
            if (mEulerRotation.z < 0) mEulerRotation.z += 360.0f;
            if (mEulerRotation.x > 360.0) mEulerRotation.x -= 360.0f;
            if (mEulerRotation.y > 360.0) mEulerRotation.y -= 360.0f;
            if (mEulerRotation.z > 360.0) mEulerRotation.z -= 360.0f;
        }

        moveTo(this->mPosition);
        scale(mScale);
        getTranformMatrix();
    }
    if (ImGui::CollapsingHeader("Materials",
                                ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
        bool has_normal = mObjectInfo.hasFlag(MaterialProps::HasNormalMap);
        if (ImGui::Checkbox("Has Normal Map", &has_normal)) {
            this->mObjectInfo.setFlag(MaterialProps::HasNormalMap, has_normal);
        }
        bool has_specular = mObjectInfo.hasFlag(MaterialProps::HasRoughnessMap);
        if (ImGui::Checkbox("Has Specular Map", &has_specular)) {
            this->mObjectInfo.setFlag(MaterialProps::HasRoughnessMap, has_specular);
        }
        bool has_diffuse = mObjectInfo.hasFlag(MaterialProps::HasDiffuseMap);
        if (ImGui::Checkbox("Has Diffuse Map", &has_diffuse)) {
            this->mObjectInfo.setFlag(MaterialProps::HasDiffuseMap, has_diffuse);
        }
        if (ImGui::SliderFloat("Diffuse Value", &mObjectInfo.roughness, 0.0f, 1.0f)) {
        }
    }
}
#endif  // DEVELOPMENT_BUILD

Transform& Transform::scale(const glm::vec3& s) {
    mScale = s;
    mScaleMatrix = glm::scale(glm::mat4{1.0}, s);
    getTranformMatrix();
    return *this;
}

Transform& Transform::rotate(const glm::vec3& around, float degree) {
    (void)degree;
    mEulerRotation += around;
    glm::vec3 euler_radians = glm::radians(mEulerRotation);
    this->mRotationMatrix = glm::toMat4(glm::quat(euler_radians));
    return *this;
}

glm::vec3& Transform::getPosition() { return mPosition; }

glm::vec3& Transform::getScale() { return mScale; }

glm::mat4& Transform::getTranformMatrix() {
    mTransformMatrix = mTranslationMatrix * mRotationMatrix * mScaleMatrix;
    mObjectInfo.transformation = mTransformMatrix;
    return mTransformMatrix;
}

float AABB::calculateVolume() {
    float dx = std::abs(min.x - max.x);
    float dy = std::abs(min.y - max.y);
    float dz = std::abs(min.z - max.z);

    return dx * dy * dz;
}

glm::vec3 AABB::getAABBSize() {
    float dx = std::abs(min.x - max.x);
    float dy = std::abs(min.y - max.y);
    float dz = std::abs(min.z - max.z);

    return glm::vec3{dx, dy, dz};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldSpaceAABB() {
    glm::vec3 corners[8];
    corners[0] = glm::vec3(min.x, min.y, min.z);
    corners[1] = glm::vec3(max.x, min.y, min.z);
    corners[2] = glm::vec3(min.x, max.y, min.z);
    corners[3] = glm::vec3(min.x, min.y, max.z);
    corners[4] = glm::vec3(max.x, max.y, min.z);
    corners[5] = glm::vec3(max.x, min.y, max.z);
    corners[6] = glm::vec3(min.x, max.y, max.z);
    corners[7] = glm::vec3(max.x, max.y, max.z);

    // 2. Initialize worldMin and worldMax
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());  // Equivalent to -FLT_MAX

    // 3. Transform each corner and update worldMin/worldMax
    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformedCorner = this->getTranformMatrix() * glm::vec4(corners[i], 1.0f);

        worldMin.x = glm::min(worldMin.x, transformedCorner.x);
        worldMin.y = glm::min(worldMin.y, transformedCorner.y);
        worldMin.z = glm::min(worldMin.z, transformedCorner.z);

        worldMax.x = glm::max(worldMax.x, transformedCorner.x);
        worldMax.y = glm::max(worldMax.y, transformedCorner.y);
        worldMax.z = glm::max(worldMax.z, transformedCorner.z);
    }

    return {worldMin, worldMax};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldMin() {
    auto min = this->getTranformMatrix() * glm::vec4(this->min, 1.0);
    auto max = this->getTranformMatrix() * glm::vec4(this->max, 1.0);
    return {min, max};
}

const std::string& BaseModel::getName() { return mName; }

Texture* BaseModel::getDiffuseTexture() { return mMeshes[0].mTexture; }
