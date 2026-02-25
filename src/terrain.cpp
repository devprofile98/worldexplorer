
#include "terrain.h"

#include "application.h"
#include "glm/gtx/string_cast.hpp"
#include "model.h"
#include "terrain_pass.h"

TerrainModel::TerrainModel(Application* app) : Model(CoordinateSystem::Z_UP) {
    BaseModel::mName = "terrain";
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
    // std::cout << "terrain model here\n";
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
        .setSize(20 * sizeof(glm::mat4))
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
    for (auto index : terrain.indices) {
        mesh.mIndexData.push_back(index);
    }
    return *this;
}

void TerrainModel::update(Application* app, float dt, float physicSimulating) {
    if (mTransform.mDirty) {
        wgpuQueueWriteBuffer(app->getRendererResource().queue, Drawable::getUniformBuffer().getBuffer(), 0,
                             &mTransform.mObjectInfo, sizeof(ObjectInfo));
        for (auto& [id, mesh] : mFlattenMeshes) {
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                                 sizeof(Material));
        }
        mTransform.mDirty = false;
    }
}

struct TTerrain : public IModel {
        TTerrain(Application* app) {
            mModel = new TerrainModel{app};
            // mModel->moveTo(glm::vec3{1}).scale(glm::vec3{1}).rotate(glm::vec3{0.0}, 0.0);
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
        .setSize(20 * sizeof(glm::mat4))
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

    return *this;
}

void Cube::update(Application* app, float dt, float physicSimulating) {
    if (mTransform.mDirty) {
        wgpuQueueWriteBuffer(app->getRendererResource().queue, Drawable::getUniformBuffer().getBuffer(), 0,
                             &mTransform.mObjectInfo, sizeof(ObjectInfo));
        for (auto& [id, mesh] : mFlattenMeshes) {
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                                 sizeof(Material));
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

USER_REGISTER_MODEL("cube", CubeLoader);
