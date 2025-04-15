#include "ui_lib.h"

#include "pipeline.h"

void GUI::initialize(Application* app, std::vector<WGPUBindGroupLayout> bindGroupLayout) {
    if (mInstace == nullptr) {
        mInstace = new GUI{};
    }

    // creating pipeline
    mBindingGroup.addBuffer(0, BindGroupEntryVisibility::VERTEX, BufferBindingType::UNIFORM, sizeof(glm::mat4));
    mBindingGroup.addSampler(1, BindGroupEntryVisibility::FRAGMENT, SampleType::Filtering);
    mBindingGroup.addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT,
                             TextureViewDimension::CUBE);

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
    /*mInstace->mUIPipeline = new Pipeline{app, bindGroupLayout};*/
    /*mInstace->mUIPipeline->defaultConfiguration(app, WGPUTextureFormat_RGBA8Unorm).createPipeline(app);*/
}
