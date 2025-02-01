#include "model.h"

#include <format>
#include <iostream>

#include "application.h"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "webgpu.h"

Model::Model() : Transform({glm::vec3{0.0}}, glm::vec3{1.0}, glm::mat4{1.0}, glm::mat4{1.0}, glm::mat4{1.0}, glm::mat4{1.0}) {
    mScaleMatrix = glm::scale(mScaleMatrix, mScale);
    mTransformMatrix =  mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    mObjectInfo.transformation =mTransformMatrix ;
    mObjectInfo.isFlat = 0;
}

Model& Model::load(std::string name, WGPUDevice device, WGPUQueue queue, const std::filesystem::path& path,
                   WGPUBindGroupLayout layout) {
    // load model from disk
    mName = name;
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
    if (!materials[0].diffuse_texname.empty()) {
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += materials[0].diffuse_texname;
        // auto a = std::filesystem::path{texture_path.c_str()};
        mTexture = new Texture{device, texture_path};
        if (mTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create Diffuse Texture view for {}\n", mName);
        }
        mTexture->uploadToGPU(queue);
    }

    // Load and upload specular texture
    if (!materials[0].specular_texname.empty()) {
        std::string texture_path = RESOURCE_DIR;
        texture_path += "/";
        texture_path += materials[0].specular_texname;
        // auto a = std::filesystem::path{texture_path.c_str()};
        mSpecularTexture = new Texture{device, texture_path};
        if (mSpecularTexture->createView() == nullptr) {
            std::cout << std::format("Failed to create Specular Texture view for {}\n", mName);
        }
        mSpecularTexture->uploadToGPU(queue);
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

        mBoundingBox.min.x = std::min(mBoundingBox.min.x, mVertexData[i].position.x);
        mBoundingBox.min.y = std::min(mBoundingBox.min.y, mVertexData[i].position.y);
        mBoundingBox.min.z = std::min(mBoundingBox.min.z, mVertexData[i].position.z);

        mBoundingBox.max.x = std::max(mBoundingBox.max.x, mVertexData[i].position.x);
        mBoundingBox.max.y = std::max(mBoundingBox.max.y, mVertexData[i].position.y);
        mBoundingBox.max.z = std::max(mBoundingBox.max.z, mVertexData[i].position.z);

        mVertexData[i].normal = {attrib.normals[3 * idx.normal_index + 0], -attrib.normals[3 * idx.normal_index + 2],
                                 attrib.normals[3 * idx.normal_index + 1]};

        mVertexData[i].color = {attrib.colors[3 * idx.vertex_index + 0], attrib.colors[3 * idx.vertex_index + 2],
                                attrib.colors[3 * idx.vertex_index + 1]};

        mVertexData[i].uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                             1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
    }

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    buffer_descriptor.size = sizeof(ObjectInfo);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mUniformBuffer = wgpuDeviceCreateBuffer(device, &buffer_descriptor);
    (void)layout;
    std::cout << mName << " M Uniform buffer address after creation: +++++++++++++++\n";
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
    return *this;
}

Model& Model::uploadToGPU(WGPUDevice device, WGPUQueue queue) {
    // upload vertex attribute data to GPU

    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    buffer_descriptor.size = mVertexData.size() * sizeof(VertexAttributes);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    buffer_descriptor.mappedAtCreation = false;

    mVertexBuffer = wgpuDeviceCreateBuffer(device, &buffer_descriptor);
    // Uploading data to GPU
    wgpuQueueWriteBuffer(queue, mVertexBuffer, 0, mVertexData.data(), buffer_descriptor.size);

    return *this;
};

size_t Model::getVertexCount() const { return mVertexData.size(); }

WGPUBuffer Model::getVertexBuffer() {
    //
    return mVertexBuffer;
}

WGPUBuffer Model::getIndexBuffer() { return mIndexBuffer; }

void Model::createSomeBinding(Application* app) {
    WGPUBindGroupEntry mBindGroupEntry = {};
    mBindGroupEntry.nextInChain = nullptr;
    mBindGroupEntry.binding = 0;
    mBindGroupEntry.buffer = mUniformBuffer;
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
    auto& uniform_data = app->getUniformData();
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

    uniform_data.modelMatrix = getModelMatrix();
    wgpuQueueWriteBuffer(render_resource.queue, app->getUniformBuffer(), offsetof(MyUniform, modelMatrix),
                         glm::value_ptr(uniform_data.modelMatrix), sizeof(glm::mat4));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, getVertexBuffer(), 0, wgpuBufferGetSize(getVertexBuffer()));

    wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);

    wgpuQueueWriteBuffer(render_resource.queue, mUniformBuffer, 0, &mObjectInfo, sizeof(ObjectInfo));
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
    getModelMatrix();
}
#endif  // DEVELOPMENT_BUILD

glm::mat4 Model::getModelMatrix() {
    mTransformMatrix =  mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    mObjectInfo.transformation = mTransformMatrix;
    return mObjectInfo.transformation;
}

Transform& Transform::scale(const glm::vec3& s) {
    mScale = s;
    mScaleMatrix = glm::scale(glm::mat4{1.0}, s);
    return *this;
}
glm::vec3& Transform::getPosition() { return mPosition; }
glm::vec3& Transform::getScale() { return mScale; }

glm::mat4& Transform::getTranformMatrix() {
    mTransformMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    return mTransformMatrix;
}

AABB& Model::getAABB() { return mBoundingBox; }

float Model::calculateVolume() {
    float dx = std::abs(mBoundingBox.min.x - mBoundingBox.max.x);
    float dy = std::abs(mBoundingBox.min.y - mBoundingBox.max.y);
    float dz = std::abs(mBoundingBox.min.z - mBoundingBox.max.z);

    return dx * dy * dz;
}

glm::vec3 Model::getAABBSize() {
    float dx = std::abs(mBoundingBox.min.x - mBoundingBox.max.x);
    float dy = std::abs(mBoundingBox.min.y - mBoundingBox.max.y);
    float dz = std::abs(mBoundingBox.min.z - mBoundingBox.max.z);

    return glm::vec3{dx, dy, dz};
}

const std::string& Model::getName() { return mName; }
