#include "model.h"

#include <iostream>

#include "application.h"
#include "tinyobjloader/tiny_obj_loader.h"

Model::Model()
    : mScaleMatrix(glm::mat4{1.0}),
      mTranslationMatrix(glm::mat4{1.0}),
      mRotationMatrix(glm::mat4{1.0}),
      mPosition(glm::vec3{0.0}) {
    mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
}

Model& Model::load(WGPUDevice device, WGPUQueue queue, const std::filesystem::path& path) {
    // load model from disk
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
    auto& materials = reader.GetMaterials();

    std::cout << materials.size() << " ::: " << materials[0].diffuse_texname << " \n";

    std::string ddd = RESOURCE_DIR;
    ddd += "/";
    ddd += materials[0].diffuse_texname;
    // auto a = std::filesystem::path{ddd.c_str()};
    std::cout << "TinyObjReader: " << ddd << std::endl;
    mTexture = new Texture{device, ddd};
    mTextureView = mTexture->createView();
    mTexture->uploadToGPU(queue);

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
    mVertexData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        mVertexData[i].position = {attrib.vertices[3 * idx.vertex_index + 0],
                                   -attrib.vertices[3 * idx.vertex_index + 2],
                                   attrib.vertices[3 * idx.vertex_index + 1]};

        mVertexData[i].normal = {attrib.normals[3 * idx.normal_index + 0], -attrib.normals[3 * idx.normal_index + 2],
                                 attrib.normals[3 * idx.normal_index + 1]};

        mVertexData[i].color = {attrib.colors[3 * idx.vertex_index + 0], attrib.colors[3 * idx.vertex_index + 1],
                                attrib.colors[3 * idx.vertex_index + 2]};

        mVertexData[i].uv = {attrib.texcoords[2 * idx.texcoord_index + 0],
                             1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
    }

    return *this;
}

Model& Model::moveBy(const glm::vec3& translationVec) {
    // justify position by the factor `m`
    (void)translationVec;
    return *this;
}
Model& Model::moveTo(const glm::vec3& moveVec) {
    // move to the position specified by `moveVec`
    // (void)moveVec;
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, moveVec);
    // mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
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

    // buffer_descriptor.size = mVertexData.size() * sizeof(uint16_t);
    // buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    // buffer_descriptor.size = (buffer_descriptor.size + 3) & ~3;
    // mIndexBuffer = wgpuDeviceCreateBuffer(device, &buffer_descriptor);
    // wgpuQueueWriteBuffer(queue, mIndexBuffer, 0, index_data.data(), buffer_descriptor.size);
    return *this;
};

size_t Model::getVertexCount() const { return mVertexData.size(); }

WGPUBuffer Model::getVertexBuffer() {
    //
    return mVertexBuffer;
}

WGPUBuffer Model::getIndexBuffer() { return mIndexBuffer; }

void Model::draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) {
    bindingData[1].nextInChain = nullptr;
    bindingData[1].binding = 1;
    bindingData[1].textureView = mTextureView;
    auto& desc = app->getBindingGroup().getDescriptor();
    desc.entries = bindingData.data();
    auto& render_resource = app->getRendererResource();
    auto& uniform_data = app->getUniformData();
    mBindGroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
    // app->getBindingGroup().getBindGroup()
    uniform_data.modelMatrix = getModelMatrix();

    std::cout << "boat model is: \n";
    for (int i = 0; i < 4; ++i) {
        std::cout << uniform_data.modelMatrix[i][0] << " " << uniform_data.modelMatrix[i][1] << " "
                  << uniform_data.modelMatrix[i][2] << " " << uniform_data.modelMatrix[i][3] << std::endl;
    }

    wgpuQueueWriteBuffer(render_resource.queue, app->getUniformBuffer(), offsetof(MyUniform, modelMatrix),
                         glm::value_ptr(uniform_data.modelMatrix), sizeof(glm::mat4));

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, getVertexBuffer(), 0, wgpuBufferGetSize(getVertexBuffer()));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindGroup, 0, nullptr);
    wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
}

void Model::draw(Application* app) {
    (void)app;
    // auto bind_data = app->getBindingGroup().getEntryData();
    // mBindGroup[1].nextInChain = nullptr;
    // bindingData[1].binding = 1;
    // bindingData[1].textureView = mTextureView;
    // desc->entries = bindingData.data();
    // bindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, desc);
    // // (void)device;

    // wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, getVertexBuffer(), 0, wgpuBufferGetSize(getVertexBuffer()));
    // // wgpuRenderPassEncoderSetIndexBuffer(render_pass_encoder, mIndexBuffer, WGPUIndexFormat_Uint16, 0,
    // //                                     wgpuBufferGetSize(mIndexBuffer));
    // wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindGroup, 0, nullptr);

    // // Draw 1 instance of a 3-vertices shape
    // // wgpuRenderPassEncoderDrawIndexed(encoder, mIndexCount, 1, 0, 0, 0);
    // wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
}

glm::mat4 Model::getModelMatrix() {
    mModelMatrix = mRotationMatrix * mTranslationMatrix * mScaleMatrix;
    return mModelMatrix;
}

glm::vec3& Model::getPosition() { return mPosition; }