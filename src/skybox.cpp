#include "skybox.h"

#include <webgpu/webgpu.h>

#include "application.h"
#include "glm/fwd.hpp"
#include "stb_image.h"

float cubeVertexData[] = {

    -1.0f, -1.0f, -1.0f,  // Back face
    1.0f,  -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  // Front face
    1.0f,  -1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,

};

uint16_t cubeIndexData[] = {
    // Back face
    0, 1, 2, 2, 3, 0,  //
    // Front face
    4, 5, 6, 6, 7, 4,  //
    // Left face
    0, 3, 7, 7, 4, 0,  //
    // Right face
    1, 2, 6, 6, 5, 1,  //
    // Bottom face
    0, 1, 5, 5, 4, 0,  //
    // Top face
    3, 2, 6, 6, 7, 3,  //

};

Pipeline* SkyBox::getPipeline() { return mRenderPipeline; }

void SkyBox::CreateBuffer() {
    WGPUBufferDescriptor buffer_descriptor = {};
    buffer_descriptor.nextInChain = nullptr;
    // Create Uniform buffers
    buffer_descriptor.size = sizeof(mMatrix);
    buffer_descriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    buffer_descriptor.mappedAtCreation = false;
    mMatrixBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &buffer_descriptor);
}

SkyBox::SkyBox(Application* app, const std::filesystem::path& cubeTexturePath) {
    this->app = app;

    // create Texture descriptor
    WGPUTextureDescriptor texture_descriptor = {};
    texture_descriptor.dimension = WGPUTextureDimension_2D;
    texture_descriptor.size = {2048, 2048, 6};
    texture_descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    texture_descriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
    texture_descriptor.mipLevelCount = 1;
    texture_descriptor.sampleCount = 1;
    WGPUTexture cubeMapTetxure = wgpuDeviceCreateTexture(app->getRendererResource().device, &texture_descriptor);

    std::vector<Texture> textures;

    const char* sides_index[] = {"right", "left", "top", "bottom", "front", "back"};
    for (uint32_t i = 0; i < 6; i++) {
        // load each side first
        WGPUTexelCopyTextureInfo copy_texture = {};
        copy_texture.texture = cubeMapTetxure;
        copy_texture.origin = {0, 0, i};

        std::string path = cubeTexturePath.string() + "/";
        path += sides_index[i];
        path += ".jpg";

        // textures.push_back(Texture{app->getRendererResource().device, path});

        int width, height, channels;
        unsigned char* pixel_data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        // If data is null, loading failed.
        if (nullptr == pixel_data) return;

        WGPUTexelCopyBufferLayout texture_data_layout = {};
        texture_data_layout.offset = 0;
        texture_data_layout.bytesPerRow = 2048 * 4;
        texture_data_layout.rowsPerImage = 2048;

        WGPUExtent3D copy_size = {2048, 2048, 1};

        if (channels == 3) {
            auto buffer_data = Texture::expandToRGBA(pixel_data, height, width);

            std::cout << "Loading texture " << path.c_str() << ' ' << channels << std::endl;
            wgpuQueueWriteTexture(app->getRendererResource().queue, &copy_texture, buffer_data.data(), 2048 * 2048 * 4,
                                  &texture_data_layout, &copy_size);

        } else {
            std::cout << "Skybox: No support for textures channel != 3 channels\n";
        }
        stbi_image_free(pixel_data);
    }

    // creating pipeline
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(glm::mat4));
    mBindingGroup.addSampler(1, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);
    mBindingGroup.addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::CUBE);

    mReflectedBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM,
                                     sizeof(glm::mat4));
    mReflectedBindingGroup.addSampler(1, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);
    mReflectedBindingGroup.addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                                      TextureViewDimension::CUBE);

    auto bind_group_layout = mBindingGroup.createLayout(app, "skybox pipeline");
    mReflectedBindingGroup.createLayout(app, "skybox reflected bindgroup");

    mRenderPipeline = new Pipeline{app, {bind_group_layout}, "skybox pipeline"};
    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
                                   .configure(sizeof(glm::vec3), VertexStepMode::VERTEX);
    mRenderPipeline->setShader(RESOURCE_DIR "/skybox.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState(

            )
        .setPrimitiveState()
        .setDepthStencilState(false, 0, 0, WGPUTextureFormat_Depth24PlusStencil8)
        .setBlendState()
        .setColorTargetState()
        .setFragmentState()
        .createPipeline(app);

    // initialize buffers
    CreateBuffer();

    mBindingData.reserve(3);

    // Matrix Uniform Data
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].buffer = mMatrixBuffer;
    mBindingData[0].offset = 0;
    mBindingData[0].size = sizeof(glm::mat4);

    // sampler data
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    WGPUSampler sampler = wgpuDeviceCreateSampler(app->getRendererResource().device, &samplerDesc);

    // setDefault(mBindingData[1]);
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].sampler = sampler;

    WGPUTextureViewDescriptor texture_view_descriptor = {};
    texture_view_descriptor.nextInChain = nullptr;
    texture_view_descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    texture_view_descriptor.dimension = WGPUTextureViewDimension_Cube;
    texture_view_descriptor.baseMipLevel = 0;
    texture_view_descriptor.mipLevelCount = 1;
    texture_view_descriptor.baseArrayLayer = 0;
    texture_view_descriptor.arrayLayerCount = 6;

    WGPUTextureView texture_view = wgpuTextureCreateView(cubeMapTetxure, &texture_view_descriptor);
    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].textureView = texture_view;

    mReflectedCameraMatrix.setLabel("Reflected Camera skybox")
        .setMappedAtCraetion()
        .setSize(sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .create(app);

    mReflectedBindingData = mBindingData;
    mReflectedBindingData[0].buffer = mReflectedCameraMatrix.getBuffer();

    mBindingGroup.create(app, mBindingData);
    mReflectedBindingGroup.create(app, mReflectedBindingData);

    WGPUBufferDescriptor vertex_buffer = {};
    vertex_buffer.nextInChain = nullptr;
    // Create Uniform buffers
    vertex_buffer.size = sizeof(cubeVertexData);
    vertex_buffer.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    vertex_buffer.mappedAtCreation = false;
    mCubeVertexDataBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &vertex_buffer);
    wgpuQueueWriteBuffer(app->getRendererResource().queue, mCubeVertexDataBuffer, 0, cubeVertexData,
                         sizeof(cubeVertexData));

    WGPUBufferDescriptor index_buffer = {};
    index_buffer.nextInChain = nullptr;
    // Create Uniform buffers
    index_buffer.size = sizeof(cubeIndexData);
    index_buffer.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    index_buffer.mappedAtCreation = false;
    mCubeIndexDataBuffer = wgpuDeviceCreateBuffer(app->getRendererResource().device, &index_buffer);
    wgpuQueueWriteBuffer(app->getRendererResource().queue, mCubeIndexDataBuffer, 0, cubeIndexData,
                         sizeof(cubeIndexData));
}

void SkyBox::draw(Application* app, WGPURenderPassEncoder encoder, const glm::mat4& mvp) {
    auto& render_resource = app->getRendererResource();

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mCubeVertexDataBuffer, 0,
                                         wgpuBufferGetSize(mCubeVertexDataBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mCubeIndexDataBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mCubeIndexDataBuffer));

    wgpuQueueWriteBuffer(render_resource.queue, mMatrixBuffer, 0, &mvp, sizeof(glm::mat4));

    // // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDrawIndexed(encoder, sizeof(cubeIndexData) / sizeof(uint16_t), 1, 0, 0, 0);
}
