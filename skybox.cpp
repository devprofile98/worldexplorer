#include "skybox.h"

#include "application.h"
// Sample shader code
/*

*/

float ASPECT_RATIO = 1024.0f / 1080.0f;

// float cubeVertexData[] = {

//     -0.5, -0.5, 0.0,  //
//     0.5, -0.5, 0.0,   //
//     -0.5, 0.5, 0.0,   //
//     0.5, 0.5, 0.0,    //
//     0.5, -0.5, 0.0,   //
//     -0.5, 0.5, 0.0,   //
//                       //
//     -0.5, -0.5, 0.5,  //
//     0.5, -0.5, 0.5,   //
//     -0.5, 0.5, 0.5,   //
//     0.5, 0.5, 0.5,    //
//     0.5, -0.5, 0.5,   //
//     -0.5, 0.5, 0.5,   //
//                       //
//     -0.5, -0.5, 0.0,  //
//     -0.5, -0.5, 0.5,  //
//     -0.5, 0.5, 0.5,   //
//     0.5, -0.5, 0.5,   //
//     -0.5, -0.5, 0.5,  //
//     -0.5, -0.5, 0.0,  //

// };

float cubeVertexData[] = {

    -1.0f, -1.0f, -1.0f,  // Back face
    1.0f,  -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  // Front face
    1.0f,  -1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,

};

uint16_t cubeIndexData[] = {
    // Back face
    0,
    1,
    2,
    2,
    3,
    0,
    // Front face
    4,
    5,
    6,
    6,
    7,
    4,
    // Left face
    0,
    3,
    7,
    7,
    4,
    0,
    // Right face
    1,
    2,
    6,
    6,
    5,
    1,
    // Bottom face
    0,
    1,
    5,
    5,
    4,
    0,
    // Top face
    3,
    2,
    6,
    6,
    7,
    3,

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
    // const char* sides_index[] = {"top", "bottom", "front", "back", "right", "left"};
    // (void)cubeTexturePath;

    this->app = app;

    for (int i = 0; i < 6; i++) {
        // load each side first
    }

    // creating pipeline
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(glm::mat4));
    mBindingGroup.addSampler(1, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);
    // mBindingGroup.addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
    //                          TextureViewDimension::CUBE);

    auto bind_group_layout = mBindingGroup.createLayout(app, "skybox pipeline");

    mRenderPipeline = new Pipeline{app, {bind_group_layout}};
    WGPUVertexBufferLayout d = mRenderPipeline->mVertexBufferLayout.addAttribute(0, 0, WGPUVertexFormat_Float32x3)
                                   .configure(sizeof(glm::vec3), VertexStepMode::VERTEX);
    mRenderPipeline->setShader(RESOURCE_DIR "/skybox.wgsl")
        .setVertexBufferLayout(d)
        .setVertexState(

            )
        .setPrimitiveState()
        .setDepthStencilState()
        .setBlendState()
        .setColorTargetState()
        .setFragmentState()
        .createPipeline(app);

    // initialize buffers
    CreateBuffer();

    mBindingData.reserve(2);

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

    // mBindingData[2].

    mBindingGroup.create(app, mBindingData);

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
    std::cout << cubeTexturePath.c_str() << std::endl;
}

void SkyBox::draw(Application* app, WGPURenderPassEncoder encoder, const glm::mat4& mvp) {
    // (void)bindingData;

    // 1 - SET THE TEXTURE
    // 2 - SET VERTEX BUFFER
    // 3 - SET INDEX BUFFER

    // 4 - DRAW INDEXED
    // WGPUBindGroup mbidngroup = {};
    // bindingData[1].nextInChain = nullptr;
    // bindingData[1].binding = 1;
    // bindingData[1].textureView = mTextureView;
    // auto& desc = app->getBindingGroup().getDescriptor();
    // desc.entries = bindingData.data();
    // glm::mat4 matrix{1.0};
    // // matrix = glm::rotate(matrix, 0.0f, glm::vec3{1.0f});
    // // matrix = glm::scale(matrix, glm::vec3{1.0f});
    // matrix = glm::translate(matrix, glm::vec3{0.3f, 0.0f, 0.0f});
    // matrix = view * matrix;

    auto& render_resource = app->getRendererResource();
    // mbidngroup = wgpuDeviceCreateBindGroup(render_resource.device, &desc);
    // std::cout << "@@###@@@###@@@###@@@### " << wgpuBufferGetSize(mCubeIndexDataBuffer) << ' '
    //           << wgpuBufferGetSize(mCubeVertexDataBuffer) << '\n';

    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mCubeVertexDataBuffer, 0,
                                         wgpuBufferGetSize(mCubeVertexDataBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(encoder, mCubeIndexDataBuffer, WGPUIndexFormat_Uint16, 0,
                                        wgpuBufferGetSize(mCubeIndexDataBuffer));
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, mBindingGroup.getBindGroup(), 0, nullptr);
    // wgpuRenderPassEncoderSetBindGroup(encoder, 0, mbidngroup, 0, nullptr);

    // wgpuRenderPassEncoderSetBindGroup(encoder, 0, bindGroup, 0, nullptr);

    (void)mvp;

    wgpuQueueWriteBuffer(render_resource.queue, mMatrixBuffer, 0, &mvp, sizeof(glm::mat4));

    // createSomeBinding(app);
    // wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);

    // // Draw 1 instance of a 3-vertices shape
    wgpuRenderPassEncoderDrawIndexed(encoder, sizeof(cubeIndexData) / 2, 1, 0, 0, 0);
    // wgpuRenderPassEncoderDraw(encoder, sizeof(cubeVertexData) / (3 * 4), 1, 0, 0);

    // wgpuRenderPassEncoderDraw(encoder, getVertexCount(), 1, 0, 0);
}