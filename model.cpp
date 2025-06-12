#include "model.h"

#include <format>
#include <iostream>

#include "application.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "webgpu.h"

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

Model& Model::load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout) {
    // load model from disk
    mName = name;

    Drawable::configure(app);

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

    std::cout << getName() << " has " << materials.size() << " Tiny Object " << materials[0].normal_texname << " \n";

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
            const auto& pos1 = mMeshes[materialId].mVertexData[i].position;
            const auto& pos2 = mMeshes[materialId].mVertexData[i + 1].position;
            const auto& pos3 = mMeshes[materialId].mVertexData[i + 2].position;

            const auto& uv1 = mMeshes[materialId].mVertexData[i].uv;
            const auto& uv2 = mMeshes[materialId].mVertexData[i + 1].uv;
            const auto& uv3 = mMeshes[materialId].mVertexData[i + 2].uv;

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

            if (glm::dot(mMeshes[materialId].mVertexData[i].normal, cnormal) < 0.0) {
                tangent = -tangent;
                bitangent = -bitangent;
                /*cnormal = -cnormal;*/
            }

            /*cnormal = expectedN;*/
            /*tangent = normalize(tangent - dot(tangent, cnormal) * cnormal);*/
            /*bitangent = cross(cnormal, tangent);*/

            mMeshes[materialId].mVertexData[i].tangent = tangent;
            mMeshes[materialId].mVertexData[i].biTangent = bitangent;
            mMeshes[materialId].mVertexData[i].normal = cnormal;
            mMeshes[materialId].mVertexData[i + 1].tangent = tangent;
            mMeshes[materialId].mVertexData[i + 1].biTangent = bitangent;
            mMeshes[materialId].mVertexData[i + 1].normal = cnormal;
            mMeshes[materialId].mVertexData[i + 2].tangent = tangent;
            mMeshes[materialId].mVertexData[i + 2].biTangent = bitangent;
            mMeshes[materialId].mVertexData[i + 2].normal = cnormal;
            /*std::cout << "------------------" << i << std::endl;*/
        }
    }

    for (const auto& material : materials) {
        size_t material_id = &material - &materials[0];
        if (material_id > mMeshes.size() - 1) {
            break;
        }
        auto& mesh = mMeshes[material_id];
        if (!material.diffuse_texname.empty()) {
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += material.diffuse_texname;
            mesh.mTexture = new Texture{render_resource.device, texture_path};
            if (mesh.mTexture->createView() == nullptr) {
                std::cout << std::format("Failed to create Diffuse Texture view for {} at \n", mName, texture_path);
            }
            mesh.mTexture->uploadToGPU(render_resource.queue);
            mesh.isTransparent = mesh.mTexture->isTransparent();
        }
    }
    // Load and upload specular texture
    if (!materials[0].specular_texname.empty()) {
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += materials[0].specular_texname;
        mSpecularTexture = new Texture{render_resource.device, texture_path};
        if (mSpecularTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create Specular Texture view for {}\n", mName);
        }
        mSpecularTexture->uploadToGPU(render_resource.queue);
    }

    // Load and upload normal texture
    if (name == "tower" || name == "cylinder") {
        std::cout << "tower\n";
        /*if (!materials[0].normal_texname.empty()) {*/
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += name == "tower" ? "Wood_Tower_Nor.jpg" : "cobblestone_normal.png";
        mNormalMapTexture = new Texture{render_resource.device, texture_path};
        if (mNormalMapTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create normal Texture view for {}\n", mName);
        }
        mNormalMapTexture->uploadToGPU(render_resource.queue);
        /*}*/
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

    (void)layout;
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
    (void)translationVec;
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
            .setSize(mesh.mVertexData.size() * sizeof(VertexAttributes))
            .setMappedAtCraetion()
            .create(app);

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mVertexBuffer.getBuffer(), 0,
                             mesh.mVertexData.data(), mesh.mVertexData.size() * sizeof(VertexAttributes));
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
            if (mSpecularTexture != nullptr && mSpecularTexture->getTextureView() != nullptr) {
                mesh.binding_data[1].nextInChain = nullptr;
                mesh.binding_data[1].binding = 1;
                mesh.binding_data[1].textureView = mSpecularTexture->getTextureView();
            }
            if (mNormalMapTexture != nullptr && mNormalMapTexture->getTextureView() != nullptr) {
                std::cout << ":::::::::::::::::::::::::::::::::::::\n\n::::::::::::::::;\n";
                mesh.binding_data[2].nextInChain = nullptr;
                mesh.binding_data[2].binding = 2;
                mesh.binding_data[2].textureView = mNormalMapTexture->getTextureView();
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
    for (auto& [mat_id, mesh] : mMeshes) {
        if (mesh.isTransparent) {
            continue;
        }

        active_bind_group = app->getBindingGroup().getBindGroup();

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                             wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));

        wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);

        wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,
                             sizeof(ObjectInfo));

        wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(encoder, 2,
                                          mesh.mTextureBindGroup == nullptr
                                              ? app->mDefaultTextureBindingGroup.getBindGroup()
                                              : mesh.mTextureBindGroup,
                                          0, nullptr);

        wgpuRenderPassEncoderDraw(encoder, mesh.mVertexData.size() - 1,
                                  this->instance == nullptr ? 1 : this->instance->getInstanceCount(), 0, 0);
    }
}

size_t Model::getInstaceCount() { return this->instances; }

#ifdef DEVELOPMENT_BUILD
void Model::userInterface() {
    ImGui::SliderFloat("X", &mPosition.x, -20.0f, 20.0f);
    ImGui::SliderFloat("Y", &mPosition.y, -100.0f, 100.0f);
    ImGui::SliderFloat("Z", &mPosition.z, -100.0f, 100.0f);

    ImGui::SliderFloat3("Scale", glm::value_ptr(mScale), 0.0f, 10.0f);
    moveTo(this->mPosition);
    scale(mScale);
    getTranformMatrix();
}
#endif  // DEVELOPMENT_BUILD

Transform& Transform::scale(const glm::vec3& s) {
    mScale = s;
    mScaleMatrix = glm::scale(glm::mat4{1.0}, s);
    getTranformMatrix();
    return *this;
}

Transform& Transform::rotate(const glm::vec3& around, float degree) {
    glm::quat delta = glm::angleAxis(glm::radians(degree), around);
    /*mOrientation = mOrientation * delta;*/
    /*mOrientation = delta;*/
    /*mOrientation = glm::normalize(mOrientation);*/
    /*this->mRotationMatrix = glm::rotate(glm::mat4{1.0f}, glm::radians(degree), around);*/
    this->mRotationMatrix = glm::toMat4(delta);
    return *this;
}

glm::vec3& Transform::getPosition() { return mPosition; }

glm::vec3& Transform::getScale() { return mScale; }

glm::mat4& Transform::getTranformMatrix() {
    mTransformMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
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

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldMin() {
    auto min = this->getTranformMatrix() * glm::vec4(this->min, 1.0);
    auto max = this->getTranformMatrix() * glm::vec4(this->max, 1.0);
    return {min, max};
}

const std::string& BaseModel::getName() { return mName; }

Texture* BaseModel::getDiffuseTexture() { return mMeshes[0].mTexture; }
