#include "model.h"

#include <format>
#include <iostream>

#include "application.h"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
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
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    std::cout << materials.size() << " Tiny Object einladung " << materials[0].specular_texname << " \n";

    // Load and upload diffuse texture
    auto& render_resource = app->getRendererResource();
    if (!materials[0].diffuse_texname.empty()) {
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += materials[0].diffuse_texname;
        // auto a = std::filesystem::path{texture_path.c_str()};
        mTexture = new Texture{render_resource.device, texture_path};
        if (mTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create Diffuse Texture view for {}\n", mName);
        }
        mTexture->uploadToGPU(render_resource.queue);
    }

    // Load and upload specular texture
    if (!materials[0].specular_texname.empty()) {
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += materials[0].specular_texname;
        // auto a = std::filesystem::path{texture_path.c_str()};
        mSpecularTexture = new Texture{render_resource.device, texture_path};
        if (mSpecularTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create Specular Texture view for {}\n", mName);
        }
        mSpecularTexture->uploadToGPU(render_resource.queue);
    }

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    // Fill in vertexData here
    const auto& shape = shapes[0];
    mVertexData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        mVertexData[i].position = {attrib.vertices[3 * idx.vertex_index + 0],
                                   -attrib.vertices[3 * idx.vertex_index + 2],
                                   attrib.vertices[3 * idx.vertex_index + 1]};

        min.x = std::min(min.x, mVertexData[i].position.x);
        min.y = std::min(min.y, mVertexData[i].position.y);
        min.z = std::min(min.z, mVertexData[i].position.z);

        max.x = std::max(max.x, mVertexData[i].position.x);
        max.y = std::max(max.y, mVertexData[i].position.y);
        max.z = std::max(max.z, mVertexData[i].position.z);

        mVertexData[i].normal = {attrib.normals[3 * idx.normal_index + 0], -attrib.normals[3 * idx.normal_index + 2],
                                 attrib.normals[3 * idx.normal_index + 1]};

        mVertexData[i].color = {attrib.colors[3 * idx.vertex_index + 0], attrib.colors[3 * idx.vertex_index + 2],
                                attrib.colors[3 * idx.vertex_index + 1]};

        mVertexData[i].uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                             1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
    }

    (void)layout;
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
    mVertexBuffer.setLabel("Uniform buffer for object info")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
        .setSize(mVertexData.size() * sizeof(VertexAttributes))
        .setMappedAtCraetion()
        .create(app);

    wgpuQueueWriteBuffer(app->getRendererResource().queue, mVertexBuffer.getBuffer(), 0, mVertexData.data(),
                         mVertexData.size() * sizeof(VertexAttributes));

    return *this;
};

size_t BaseModel::getVertexCount() const { return mVertexData.size(); }

Buffer BaseModel::getVertexBuffer() { return mVertexBuffer; }

Buffer BaseModel::getIndexBuffer() { return mIndexBuffer; }

void Model::createSomeBinding(Application* app) {
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
}

void Model::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    auto& render_resource = app->getRendererResource();
    /*auto& uniform_data = app->getUniformData();*/
    WGPUBindGroup active_bind_group = nullptr;

    // Bind Diffuse texture for the model if existed
    if (mTexture != nullptr) {
        if (mTexture->getTextureView() != nullptr) {
            bindingData[1].nextInChain = nullptr;
            bindingData[1].binding = 1;
            bindingData[1].textureView = mTexture->getTextureView();
        }
        if (mSpecularTexture != nullptr && mSpecularTexture->getTextureView() != nullptr) {
            bindingData[5].nextInChain = nullptr;
            bindingData[5].binding = 5;
            bindingData[5].textureView = mSpecularTexture->getTextureView();
        }
        auto& desc = app->getBindingGroup().getDescriptor();
        desc.entries = bindingData.data();
        mBindGroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
        active_bind_group = mBindGroup;
    } else {
        active_bind_group = app->getBindingGroup().getBindGroup();
    }

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, getVertexBuffer().getBuffer(), 0,
                                         wgpuBufferGetSize(getVertexBuffer().getBuffer()));

    wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);
    wgpuQueueWriteBuffer(render_resource.queue, Drawable::getUniformBuffer().getBuffer(), 0, &mObjectInfo,
                         sizeof(ObjectInfo));

    createSomeBinding(app);
    wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);

    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
    wgpuBindGroupRelease(ggg);
    // if we created a binding group and didn't use the default appliaction binding-group
    if (mBindGroup != nullptr) {
        wgpuBindGroupRelease(mBindGroup);
        mBindGroup = nullptr;
    }
}

#ifdef DEVELOPMENT_BUILD
void Model::userInterface() {
    ImGui::SliderFloat("X", &mPosition.x, -10.0f, 10.0f);
    ImGui::SliderFloat("Y", &mPosition.y, -10.0f, 10.0f);
    ImGui::SliderFloat("Z", &mPosition.z, -10.0f, 10.0f);

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

const std::string& BaseModel::getName() { return mName; }
