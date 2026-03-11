
#include "terrain.h"

#include "application.h"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "model.h"
#include "physics.h"
#include "shapes.h"
#include "terrain_pass.h"
#include "utils.h"

// #define WIREFRAME_ENABLED
Pipeline* custom_shader = nullptr;
WGPUBindGroupLayout custom_layout;
BindingGroup custom_bindgroup;

void createCustomShaderMaterial(Application* mApp, WGPUTextureFormat textureFormat) {
    custom_bindgroup
        .addTexture(0, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(1, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(2, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D)
        .addTexture(3, BindGroupEntryVisibility::FRAGMENT, TextureSampleType::FLAOT, TextureViewDimension::ARRAY_2D);

    custom_layout = custom_bindgroup.createLayout(mApp->getRendererResource(), "Terrain Texture layout bidngroup");

    std::filesystem::path base_path = mApp->getBinaryPathAbsolute() / ".." / RESOURCE_DIR;
    auto mGrass =
        new Texture{mApp->getRendererResource().device,
                    std::vector<std::filesystem::path>{base_path / "mud/diffuse.jpg", base_path / "mud/normal.jpg",
                                                       base_path / "mud/roughness.jpg"},
                    WGPUTextureFormat_RGBA8Unorm, 3};
    mGrass->createViewArray(0, 3);
    mGrass->uploadToGPU(mApp->getRendererResource().queue);

    auto mRock = new Texture{mApp->getRendererResource().device,
                             std::vector<std::filesystem::path>{base_path / "Rock/Rock060_1K-JPG_Color.jpg",
                                                                base_path / "Rock/Rock060_1K-JPG_NormalGL.jpg",
                                                                base_path / "Rock/Rock060_1K-JPG_Roughness.jpg"},
                             WGPUTextureFormat_RGBA8Unorm, 3};
    mRock->createViewArray(0, 3);
    mRock->uploadToGPU(mApp->getRendererResource().queue);

    auto mSand = new Texture{mApp->getRendererResource().device,
                             std::vector<std::filesystem::path>{base_path / "aerial/aerial_beach_01_diff_1k.jpg",
                                                                base_path / "aerial/aerial_beach_01_nor_gl_1k.jpg",
                                                                base_path / "aerial/aerial_beach_01_rough_1k.jpg"},
                             WGPUTextureFormat_RGBA8Unorm, 3};
    mSand->createViewArray(0, 3);
    mSand->uploadToGPU(mApp->getRendererResource().queue);

    auto mSnow = new Texture{mApp->getRendererResource().device,
                             std::vector<std::filesystem::path>{base_path / "snow/snow_02_diff_1k.jpg",
                                                                base_path / "snow/snow_02_nor_gl_1k.jpg",
                                                                base_path / "snow/snow_02_rough_1k.jpg"},
                             WGPUTextureFormat_RGBA8Unorm, 3};
    mSnow->createViewArray(0, 3);
    mSnow->uploadToGPU(mApp->getRendererResource().queue);
    std::vector<Texture*> mTempTextures = {mGrass, mRock, mSand, mSnow};

    std::vector<WGPUBindGroupEntry> mBindingData{4};
    mBindingData[0] = {};
    mBindingData[0].nextInChain = nullptr;
    mBindingData[0].binding = 0;
    mBindingData[0].textureView = mTempTextures[0]->getTextureViewArray();

    mBindingData[1] = {};
    mBindingData[1].nextInChain = nullptr;
    mBindingData[1].binding = 1;
    mBindingData[1].textureView = mTempTextures[1]->getTextureViewArray();

    mBindingData[2] = {};
    mBindingData[2].nextInChain = nullptr;
    mBindingData[2].binding = 2;
    mBindingData[2].textureView = mTempTextures[2]->getTextureViewArray();

    mBindingData[3] = {};
    mBindingData[3].nextInChain = nullptr;
    mBindingData[3].binding = 3;
    mBindingData[3].textureView = mTempTextures[3]->getTextureViewArray();

    custom_bindgroup.create(mApp->getRendererResource(), mBindingData);

    auto* layouts = mApp->getBindGroupLayouts();
    auto& resource = mApp->getRendererResource();
    custom_shader = new Pipeline{
        mApp, {layouts[0], layouts[1], layouts[2], layouts[3], layouts[4], layouts[5], custom_layout}, "Custom Shader"};
    custom_shader->defaultConfiguration(resource, textureFormat);
    custom_shader->setShader(mApp->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / "shaders" / "terrain.wgsl",
                             resource);
    custom_shader->setDepthStencilState(custom_shader->getDepthStencilState());
    setDefault(custom_shader->getDepthStencilState());
    custom_shader->getDepthStencilState().format = WGPUTextureFormat_Depth24PlusStencil8;
    custom_shader->getDepthStencilState().depthWriteEnabled = WGPUOptionalBool_True;
    custom_shader->getDepthStencilState().depthCompare = WGPUCompareFunction_Less;
    custom_shader->setPrimitiveState(WGPUFrontFace_CCW, WGPUCullMode_Back);

    custom_shader->createPipeline(resource);
}

Pipeline* TerrainModel::getPipeline(Application* app) {
    (void)app;
    return custom_shader;
}

TerrainModel::TerrainModel(Application* app) : Model(CoordinateSystem::Z_UP) {
    BaseModel::mName = "terrain";
    BaseModel::setType(ModelTypes::CODE);
    mApp = app;
}

Model& TerrainModel::uploadToGPU(Application* app) {
    for (auto& [_mat_id, mesh] : mFlattenMeshes) {
        mesh.mVertexBuffer.setLabel("Uniform buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
            .setSize((mesh.mVertexData.size() + 1) * sizeof(VertexAttributes))
            .setMappedAtCraetion()
            .create(&mApp->getRendererResource());

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mVertexBuffer.getBuffer(), 0,
                             mesh.mVertexData.data(), mesh.mVertexData.size() * sizeof(VertexAttributes));

        mesh.mIndexBuffer.setLabel("index buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Index)
            .setSize((mesh.mIndexData.size()) * sizeof(uint32_t))
            .setMappedAtCraetion()
            .create(&mApp->getRendererResource());

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndexBuffer.getBuffer(), 0, mesh.mIndexData.data(),
                             mesh.mIndexData.size() * sizeof(uint32_t));
    }

    mRootNode = new Node{"terrain", glm::mat4{1.0}, nullptr, {}, {0}};
    return *this;
};

void TerrainModel::drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node) {
    internalDraw(app, encoder, node);

    for (auto* child : node->mChildrens) {
        drawGraph(app, encoder, child);
    }
}

void TerrainModel::drawHirarchy(Application* app, WGPURenderPassEncoder encoder) {
    TerrainModel::drawGraph(app, encoder, mRootNode);
}

Model& TerrainModel::load(std::string name, Application* app, const std::filesystem::path& path,
                          WGPUBindGroupLayout layout) {
    Drawable::configure(app);

    mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
        .setSize(sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
        .create(&app->getRendererResource());

    mGlobalMeshTransformationData.push_back(glm::mat4{1.0});
    auto& databuffer = mGlobalMeshTransformationData;
    mGlobalMeshTransformationBuffer.queueWrite(0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());

    auto& mesh = mFlattenMeshes[0];

    Terrain terrain;
    std::vector<glm::vec3> out;
    terrain.generate(200, 8, out);
    mesh.mVertexData = std::move(terrain.vertices);
    mesh.mName = "plane";
    for (auto index : terrain.indices) {
        mesh.mIndexData.push_back(index);
    }
    moveTo({0.0, 0.0, 0.0});
    rotate({0.0, 0.0, 0.0}, 0.0);

#ifdef WIREFRAME_ENABLED
    auto mesh_wireframe_lines = generateFromMesh(mesh.mIndexData, mesh.mVertexData);
    wireFrame = app->mLineEngine->create(mesh_wireframe_lines, mTransform.mTransformMatrix, glm::vec3{1.0, 0.0, 0.0});
#endif

    createCustomShaderMaterial(app, WGPUTextureFormat_BGRA8UnormSrgb);

    return *this;
}

void TerrainModel::update(Application* app, float dt, float physicSimulating) {
    if (mTransform.mDirty) {
#ifdef WIREFRAME_ENABLED
        wireFrame.updateTransformation(mTransform.mTransformMatrix);
#endif
        wgpuQueueWriteBuffer(app->getRendererResource().queue, Drawable::getUniformBuffer().getBuffer(), 0,
                             &mTransform.mObjectInfo, sizeof(ObjectInfo));
        for (auto& [id, mesh] : mFlattenMeshes) {
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                                 sizeof(ShaderMaterial));
        }
        mTransform.mDirty = false;
    }
}

void TerrainModel::getCustomBindGroup(Application* app, WGPURenderPassEncoder encoder, Mesh& mesh) {
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, app->getBindingGroup().getBindGroup(), 0, nullptr);

    wgpuRenderPassEncoderSetBindGroup(encoder, 1, getObjectInfoBindGroup(), 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(
        encoder, 2,
        mesh.mTextureBindGroup == nullptr ? app->mDefaultTextureBindingGroup.getBindGroup() : mesh.mTextureBindGroup, 0,
        nullptr);

    wgpuRenderPassEncoderSetBindGroup(encoder, 6, custom_bindgroup.getBindGroup(), 0, nullptr);
}

struct TTerrain : public IModel {
        TTerrain(Application* app) {
            mModel = new TerrainModel{app};
            std::cout << glm::to_string(mModel->mTransform.mTransformMatrix) << std::endl;
            mModel->load("terrain", app, "", nullptr);
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() { return mModel; }

        void onLoad(Application* app, void* params) {
            (void)params;
            (void)app;
        };
};

USER_REGISTER_MODEL("terrain", TTerrain);

Cube::Cube(Application* app) : Model(CoordinateSystem::Z_UP) {
    BaseModel::mName = "cube22";
    BaseModel::setType(ModelTypes::CODE);
    mApp = app;
}

Model& Cube::uploadToGPU(Application* app) {
    for (auto& [_mat_id, mesh] : mFlattenMeshes) {
        mesh.mVertexBuffer.setLabel("Uniform buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
            .setSize((mesh.mVertexData.size() + 1) * sizeof(VertexAttributes))
            .setMappedAtCraetion()
            .create(&mApp->getRendererResource());

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mVertexBuffer.getBuffer(), 0,
                             mesh.mVertexData.data(), mesh.mVertexData.size() * sizeof(VertexAttributes));

        mesh.mIndexBuffer.setLabel("index buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Index)
            .setSize((mesh.mIndexData.size()) * sizeof(uint32_t))
            .setMappedAtCraetion()
            .create(&mApp->getRendererResource());

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndexBuffer.getBuffer(), 0, mesh.mIndexData.data(),
                             mesh.mIndexData.size() * sizeof(uint32_t));
    }

    mRootNode = new Node{"terrain", glm::mat4{1.0}, nullptr, {}, {0}};
    return *this;
};

void Cube::drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node) {
    // std::cout << "terrain model here\n";
    internalDraw(app, encoder, node);

    for (auto* child : node->mChildrens) {
        drawGraph(app, encoder, child);
    }
}

void Cube::drawHirarchy(Application* app, WGPURenderPassEncoder encoder) { Cube::drawGraph(app, encoder, mRootNode); }

Model& Cube::load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout) {
    Drawable::configure(app);

    mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
        .setSize(sizeof(glm::mat4))
        .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
        .create(&app->getRendererResource());

    mGlobalMeshTransformationData.push_back(glm::mat4{1.0});
    auto& databuffer = mGlobalMeshTransformationData;
    mGlobalMeshTransformationBuffer.queueWrite(0, databuffer.data(), sizeof(glm::mat4) * databuffer.size());

    auto& mesh = mFlattenMeshes[0];

    // 24 vertices (4 per face, 6 faces) to allow unique normals/UVs per face
    mesh.mVertexData = {
        // Front face (z = +0.5), normal = (0, 0, 1)
        {{-0.5f, -0.5f, 0.5f}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0, 0, 1}, {1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0.0f, 1.0f}},

        // Back face (z = -0.5), normal = (0, 0, -1)
        {{0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}, {-1, 0, 0}, {0, 1, 0}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}, {-1, 0, 0}, {0, 1, 0}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}, {-1, 0, 0}, {0, 1, 0}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}, {-1, 0, 0}, {0, 1, 0}, {0.0f, 1.0f}},

        // Left face (x = -0.5), normal = (-1, 0, 0)
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {0.0f, 1.0f}},

        // Right face (x = +0.5), normal = (1, 0, 0)
        {{0.5f, -0.5f, 0.5f}, {1, 0, 0}, {1, 1, 0}, {0, 0, -1}, {0, 1, 0}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 1, 0}, {0, 0, -1}, {0, 1, 0}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1, 0, 0}, {1, 1, 0}, {0, 0, -1}, {0, 1, 0}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1, 0, 0}, {1, 1, 0}, {0, 0, -1}, {0, 1, 0}, {0.0f, 1.0f}},

        // Top face (y = +0.5), normal = (0, 1, 0)
        {{-0.5f, 0.5f, 0.5f}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {0, 0, -1}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {0, 0, -1}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {0, 0, -1}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {0, 0, -1}, {0.0f, 1.0f}},

        // Bottom face (y = -0.5), normal = (0, -1, 0)
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0, -1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0, -1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 0, 1}, {0.0f, 1.0f}},
    };

    // 2 triangles per face, 6 faces = 36 indices
    mesh.mIndexData = {
        0,  1,  2,  0,  2,  3,   // Front
        4,  5,  6,  4,  6,  7,   // Back
        8,  9,  10, 8,  10, 11,  // Left
        12, 13, 14, 12, 14, 15,  // Right
        16, 17, 18, 16, 18, 19,  // Top
        20, 21, 22, 20, 22, 23,  // Bottom
    };
    moveTo(glm::vec3{0.0, 0.0, -2.8});
    rotate({0.0, 0.0, 0.0}, 0.0);

    auto* bdy = physics::createPhysicFromShape(mesh.mIndexData, mesh.mVertexData, mTransform.mTransformMatrix);

    if (bdy != nullptr) {
        mPhysicComponent = new PhysicsComponent{bdy->GetID()};
    } else {
        std::cout << "Failed to create physics from mesh!\n";
    }
#ifdef WIREFRAME_ENABLED
    auto mesh_wireframe_lines = generateFromMesh(mesh.mIndexData, mesh.mVertexData);
    wireFrame = app->mLineEngine->create(mesh_wireframe_lines, mTransform.mTransformMatrix, glm::vec3{1.0, 0.0, 0.0});
#endif

    return *this;
}

void Cube::update(Application* app, float dt, float physicSimulating) {
    if (mTransform.mDirty) {
#ifdef WIREFRAME_ENABLED
        wireFrame.updateTransformation(mTransform.mTransformMatrix);
#endif
        wgpuQueueWriteBuffer(app->getRendererResource().queue, Drawable::getUniformBuffer().getBuffer(), 0,
                             &mTransform.mObjectInfo, sizeof(ObjectInfo));
        for (auto& [id, mesh] : mFlattenMeshes) {
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                                 sizeof(ShaderMaterial));
        }
        mTransform.mDirty = false;
    }
}

struct CubeLoader : public IModel {
        CubeLoader(Application* app) {
            mModel = new Cube{app};
            // mModel->moveTo(glm::vec3{1}).scale(glm::vec3{1}).rotate(glm::vec3{0.0}, 0.0);
            std::cout << glm::to_string(mModel->mTransform.mTransformMatrix) << std::endl;
            mModel->load("cube", app, "", nullptr);
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() { return mModel; }

        void onLoad(Application* app, void* params) {
            (void)params;
            (void)app;
        };
};

USER_REGISTER_MODEL("cube22", CubeLoader);
