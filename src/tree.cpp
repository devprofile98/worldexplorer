#include <cstdint>
#include <random>

#include "GLFW/glfw3.h"
#include "animation.h"
#include "application.h"
#include "glm/ext/matrix_common.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "physics.h"
#include "rendererResource.h"
#include "world.h"

struct TreeModel : public IModel {
        TreeModel(Application* app) {
            mModel = new Model{};
            mModel->load("tree", app, RESOURCE_DIR "/tree3.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, -7.640, 1.125})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.9});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            std::random_device rd;   // Seed the random number generator
            std::mt19937 gen(rd());  // Mersenne Twister PRNG
            std::uniform_real_distribution<float> dist_for_rotation(0.0, 90.0);
            std::uniform_real_distribution<float> dist(1.9, 2.5);

            std::vector<glm::vec3> positions;
            std::vector<float> degrees;
            std::vector<glm::vec3> scales;
            positions.reserve(5000);
            degrees.reserve(5000);
            scales.reserve(5000);

            for (size_t i = 0; i < app->mTerrainPass->terrainData.size(); i++) {
                if (i % 40 == 0) {
                    positions.emplace_back(glm::vec3(app->mTerrainPass->terrainData[i].x,
                                                     app->mTerrainPass->terrainData[i].y,
                                                     app->mTerrainPass->terrainData[i].z));
                    degrees.emplace_back(glm::radians(dist_for_rotation(gen)));
                    scales.emplace_back(glm::vec3{0.9f * dist(gen)});
                }
            }

            // std::cout << mModel->getName() << positions[0];
            positions[1] = mModel->mTransform.getPosition();
            auto* ins = new Instance{positions, glm::vec3{0.0, 0.0, 1.0},     degrees,
                                     scales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(),
                                 100000 * sizeof(InstanceData), ins->mInstanceBuffer.data(),
                                 sizeof(InstanceData) * (ins->mInstanceBuffer.size() - 1));

            std::cout << "(((((((((((((((( in mesh " << mModel->mMeshes.size() << std::endl;

            mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + mModel->getName()).c_str())
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                          WGPUBufferUsage_CopyDst)
                .setSize(sizeof(DrawIndexedIndirectArgs))
                .setMappedAtCraetion()
                .create(app);

            // mModel->mIndirectDrawArgsBuffer2.setLabel(("indirect draw args buffer2 for " +
            // mModel->getName()).c_str())
            //     .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
            //               WGPUBufferUsage_CopyDst)
            //     .setSize(sizeof(DrawIndexedIndirectArgs))
            //     .setMappedAtCraetion()
            //     .create(app);

            auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer.getBuffer(), 0,
                                 &indirect, sizeof(DrawIndexedIndirectArgs));

            // wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer2.getBuffer(), 0,
            //                      &indirect, sizeof(DrawIndexedIndirectArgs));

            for (auto& [mat_id, mesh] : mModel->mMeshes) {
                mesh.mIndirectDrawArgsBuffer.setLabel(("indirect_draw_args_mesh_ " + mModel->getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(app);

                std::cout << ")))))))))))) For " << mModel->getName() << &mesh << " Index count is "
                          << static_cast<uint32_t>(mesh.mIndexData.size()) << std::endl;
                auto indirect = DrawIndexedIndirectArgs{static_cast<uint32_t>(mesh.mIndexData.size()), 0, 0, 0, 0};
                wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0,
                                     &indirect, sizeof(DrawIndexedIndirectArgs));
            }

            mModel->mTransform.mObjectInfo.instanceOffsetId = 1;
            mModel->setInstanced(ins);

            std::cout << "--------------------Barrier reached ----------------- for tree"
                      << mModel->instance->getInstanceCount() << '\n';
        };
};

struct BoatModel : public IModel {
        BoatModel(Application* app) {
            mModel = new Model{};
            mModel->load("boat", app, RESOURCE_DIR "/fourareen.obj", app->getObjectBindGroupLayout())
                .scale(glm::vec3{0.8})
                .rotate(glm::vec3{0.0f, 0.0f, 45.0f}, 0.0)
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            mModel->moveTo(glm::vec3{-3.950, 13.316, -3.375});
        };
};

struct JetModel : public IModel {
        JetModel(Application* app) {
            mModel = new Model{};
            mModel->load("jet", app, RESOURCE_DIR "/old_jet3.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{5.0, -1., 2.})
                .scale(glm::vec3{0.8});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct CarModel : public IModel {
        CarModel(Application* app) {
            mModel = new Model{};

            mModel->load("car", app, RESOURCE_DIR "/jeep.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.883, -6.280, -1.709})
                // .scale(glm::vec3{1.0})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->useTexture(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;

            (void)app;
        };
};

struct TowerModel : public IModel {
        TowerModel(Application* app) {
            mModel = new Model{};

            mModel->load("tower", app, RESOURCE_DIR "/tower.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.0, -1.0, -2.58})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.5});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct ConiferousModel : public IModel {
        ConiferousModel(Application* app) {
            mModel = new Model{};

            mModel->load("coniferous", app, RESOURCE_DIR "/coniferous/scene.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1.0f, 1.0f, 4.0f})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.2});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;

            (void)app;
        };
};

struct DeskModel : public IModel {
        DeskModel(Application* app) {
            mModel = new Model{};

            mModel->load("desk", app, RESOURCE_DIR "/desk.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, 0.333, 0.72})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.3});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct GrassModel : public IModel {
        GrassModel(Application* app) {
            mModel = new Model{};

            mModel->load("grass", app, RESOURCE_DIR "/grass.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{6.125, 2.239, -2.859})
                .scale(glm::vec3{0.2});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;

            // std::vector<glm::mat4> dddata = {};

            std::random_device rd;   // Seed the random number generator
            std::mt19937 gen(rd());  // Mersenne Twister PRNG
            std::uniform_real_distribution<float> dist(1.5, 2.5);
            std::uniform_real_distribution<float> dist_for_rotation(0.0, 90.0);
            std::uniform_real_distribution<float> dist_for_tree(1.9, 2.5);
            std::uniform_real_distribution<float> dist_for_grass(1.0, 1.8);

            std::vector<glm::vec3> positions;
            std::vector<float> degrees;
            std::vector<glm::vec3> scales;
            positions.reserve(5000);
            degrees.reserve(5000);
            scales.reserve(5000);

            for (size_t i = 0; i < app->mTerrainPass->terrainData.size(); i++) {
                if (i % 5 == 0) {
                    positions.emplace_back(glm::vec3{app->mTerrainPass->terrainData[i].x,
                                                     app->mTerrainPass->terrainData[i].y,
                                                     app->mTerrainPass->terrainData[i].z});
                    degrees.emplace_back(glm::radians(dist_for_rotation(gen)));
                    scales.emplace_back(glm::vec3{0.15f * dist(gen)});
                }
            }

            positions[1000] = mModel->mTransform.getPosition();
            positions[1000].z += 1.0;
            positions[1] = mModel->mTransform.getPosition();

            auto* ins = new Instance{positions, glm::vec3{0.0, 0.0, 1.0},     degrees,
                                     scales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};
            // auto* ins = new Instance{tmppositions, glm::vec3{0.0, 0.0, 1.0},     tmpdegrees,
            //                          tmpscales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(), 0,
                                 ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size() - 1));

            std::cout << "(((((((((((((((( in mesh " << mModel->mMeshes.size() << std::endl;

            mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + mModel->getName()).c_str())
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                          WGPUBufferUsage_CopyDst)
                .setSize(sizeof(DrawIndexedIndirectArgs))
                .setMappedAtCraetion()
                .create(app);

            auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer.getBuffer(), 0,
                                 &indirect, sizeof(DrawIndexedIndirectArgs));

            for (auto& [mat_id, mesh] : mModel->mMeshes) {
                mesh.mIndirectDrawArgsBuffer.setLabel(("indirect_draw_args_mesh_ " + mModel->getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(app);

                std::cout << ")))))))))))) For " << mModel->getName() << &mesh << " Index count is "
                          << static_cast<uint32_t>(mesh.mIndexData.size()) << std::endl;
                auto indirect = DrawIndexedIndirectArgs{static_cast<uint32_t>(mesh.mIndexData.size()), 0, 0, 0, 0};
                wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0,
                                     &indirect, sizeof(DrawIndexedIndirectArgs));
            }

            mModel->mTransform.mObjectInfo.instanceOffsetId = 0;
            mModel->setInstanced(ins);
            std::cout << "--------------------Barrier reached -----------------\n";
        };
};

struct CylinderModel : public IModel {
        CylinderModel(Application* app) {
            mModel = new Model{};

            mModel->load("cylinder", app, RESOURCE_DIR "/cyliner.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, -1.0, 0.72})
                .scale(glm::vec3{0.2})
                .rotate(glm::vec3{0.0, 0.0, 1.0}, 90.0f);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct SphereModel : public IModel {
        SphereModel(Application* app) {
            mModel = new Model{};

            mModel->load("sphere", app, RESOURCE_DIR "/sphere.glb", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1, 1, 1})
                .scale(glm::vec3{0.005f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }

        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;

            std::vector<glm::mat4> dddata = {};

            std::vector<glm::vec3> positions;
            std::vector<float> degrees;
            std::vector<glm::vec3> scales;
            positions.reserve(20);
            degrees.reserve(20);
            scales.reserve(20);

            // for (const auto& m : mModel->mBonePosition) {
            //     positions.emplace_back(m);
            //     degrees.emplace_back(0.0);
            //     scales.emplace_back(glm::vec3{0.1f});
            // }
            positions.emplace_back(glm::vec3{4.187, 9.227, 0.0});
            degrees.emplace_back(0.0);
            scales.emplace_back(glm::vec3{0.1f});

            positions.emplace_back(glm::vec3{5.187, 9.227, 0.0});
            degrees.emplace_back(0.0);
            scales.emplace_back(glm::vec3{0.1f});

            positions.emplace_back(glm::vec3{6.187, 9.227, 0.0});
            degrees.emplace_back(0.0);
            scales.emplace_back(glm::vec3{0.1f});

            positions.emplace_back(glm::vec3{7.187, 9.227, 0.0});
            degrees.emplace_back(0.0);
            scales.emplace_back(glm::vec3{0.1f});

            positions.emplace_back(glm::vec3{8.187, 9.227, 0.0});
            degrees.emplace_back(0.0);
            scales.emplace_back(glm::vec3{0.1f});

            auto* ins = new Instance{positions, glm::vec3{0.0, 0.0, 1.0},     degrees,
                                     scales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(), 0,
                                 ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size() - 1));

            mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + mModel->getName()).c_str())
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                          WGPUBufferUsage_CopyDst)
                .setSize(sizeof(DrawIndexedIndirectArgs))
                .setMappedAtCraetion()
                .create(app);

            auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer.getBuffer(), 0,
                                 &indirect, sizeof(DrawIndexedIndirectArgs));

            for (auto& [mat_id, mesh] : mModel->mMeshes) {
                mesh.mIndirectDrawArgsBuffer.setLabel(("indirect_draw_args_mesh_ " + mModel->getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(app);

                auto indirect = DrawIndexedIndirectArgs{static_cast<uint32_t>(mesh.mIndexData.size()), 0, 0, 0, 0};
                wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0,
                                     &indirect, sizeof(DrawIndexedIndirectArgs));
            }

            mModel->mTransform.mObjectInfo.instanceOffsetId = 0;
            mModel->setInstanced(ins);
            std::cout << positions.size() << " --------------------Barrier reached -----------------\n";
        };
};

struct Steampunk : public IModel {
        Steampunk(Application* app) {
            mModel = new Model{};

            mModel->load("steampunk", app, RESOURCE_DIR "/steampunk.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-1.45, -3.239, -0.810})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.002f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct Motor : public IModel {
        Motor(Application* app) {
            mModel = new Model{};

            mModel->load("motor", app, RESOURCE_DIR "/motor.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.45, -3.239, -0.810})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{1.0f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct SheepModel : public IModel {
        SheepModel(Application* app) {
            mModel = new Model{};

            mModel->load("sheep", app, RESOURCE_DIR "/fox.dae", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{5.125, 2.239, -2.859})
                // .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.3f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);

            mModel->mSkiningTransformationBuffer.setLabel("default skining data transform")
                .setSize(100 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .setMappedAtCraetion(false)
                .create(app);

            static std::vector<glm::mat4> bones;
            for (int i = 0; i < 100; i++) {
                bones.emplace_back(glm::mat4{1.0});
            }
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mSkiningTransformationBuffer.getBuffer(), 0,
                                 bones.data(), sizeof(glm::mat4) * bones.size());

            // WGPUBindGroupEntry mSkiningDataEntry = {};
            // mSkiningDataEntry.nextInChain = nullptr;
            // mSkiningDataEntry.binding = 0;
            // mSkiningDataEntry.buffer = mModel->mSkiningTransformationBuffer.getBuffer();
            // mSkiningDataEntry.offset = 0;
            // mSkiningDataEntry.size = sizeof(glm::mat4) * 100;
            //
            // WGPUBindGroupDescriptor descriptor = {};
            // descriptor.nextInChain = nullptr;
            // descriptor.entries = &mSkiningDataEntry;
            // descriptor.entryCount = 1;
            // descriptor.label = {"skining bind group", WGPU_STRLEN};
            // descriptor.layout = app->getBindGroupLayouts()[6];
            //
            // mModel->mSkiningBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &descriptor);

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct HouseModel : public IModel {
        HouseModel(Application* app) {
            mModel = new Model{};

            // mModel->load("house", app, RESOURCE_DIR "/house/scene.gltf", app->getObjectBindGroupLayout())
            // mModel->load("house2", app, RESOURCE_DIR "/house2/house3.gltf", app->getObjectBindGroupLayout())
            mModel->load("house2", app, RESOURCE_DIR "/ourhome.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-0.483, 2.048, -3.309})
                .rotate(glm::vec3{0.0, 0.0, 0.0}, 0.0)
                .scale(glm::vec3{.601f});
            mModel->uploadToGPU(app);
            // mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct CubeModel : public IModel {
        CubeModel(Application* app) {
            mModel = new Model{};

            mModel->load("cube", app, RESOURCE_DIR "/cube.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1, 1, 1})
                .scale(glm::vec3{0.2f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct PistolModel : public IModel {
        PistolModel(Application* app) {
            mModel = new Model{};

            mModel->load("pistol", app, RESOURCE_DIR "/pistol/scene.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-0.4, -2.1, 4.6})
                .scale(glm::vec3{0.2f})
                .rotate({0, 335, 270}, 0.0);

            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct HumanModel : public IModel {
        HumanModel(Application* app) {
            mModel = new Model{Z_UP};

            mModel->load("human", app, RESOURCE_DIR "/model2.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0, 0, 0})
                .scale(glm::vec3{3.})
                .rotate(glm::vec3{90.0, 0.0, 0.0}, 0.);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);

            mModel->mSkiningTransformationBuffer.setLabel("default skining data transform for human")
                .setSize(100 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .create(app);

            std::vector<glm::mat4> bones;
            for (int i = 0; i < 100; i++) {
                bones.emplace_back(glm::mat4{1.0});
            }
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mSkiningTransformationBuffer.getBuffer(), 0,
                                 bones.data(), sizeof(glm::mat4) * bones.size());

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }

        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct CameraTransition {
        bool active;
        glm::vec3 from;
        glm::vec3 to;
        float timeLeft;
};

enum CharacterState {
    Idle = 0,
    Running,
    Walking,
    Jumping,
    Falling,
    Aiming,
};

struct HumanBehaviour : public Behaviour {
        std::string name;
        glm::vec3 front;  // Character's forward direction
        glm::vec3 up;     // Up vector (typically {0, 1, 0} for world up)
        glm::vec3 targetDistance;
        glm::vec3 targetOffset;
        float yaw;      // Horizontal rotation angle (degrees)
        float speed;    // Movement speed (units per second)
        bool isMoving;  // Track movement state for animation
        bool isAiming;
        bool isShooting;
        bool isInJump;
        float cameraOffset;
        float lastX;
        float lastY;
        CharacterState state;
        CameraTransition transitoin;
        float groundLevel = -3.262;
        glm::vec3 velocity{0.0};
        Model* weapon;
        std::string attackAction;
        std::string idleAction;
        JPH::CharacterVirtual* physicalCharacter;

        HumanBehaviour(std::string name)
            : name(name),
              front(0.0f, -1.0f, 0.0f),  // Forward after 90-degree X rotation
              up(0.0f, 0.0f, -1.0f),     // Up after 90-degree X rotation
              targetDistance(0.3),
              targetOffset(0.0, 0.0, 0.2),
              yaw(90.0f),
              speed(30.0f),
              isMoving(false),
              isAiming(false),
              isShooting(false),
              isInJump(false),
              cameraOffset(0.0),
              lastX(0),
              lastY(0),
              state(Idle),
              transitoin({false, glm::vec3{0.0}, glm::vec3{0.0}, 0.0}),
              weapon(nullptr),
              attackAction("Punch_Jab"),
              idleAction("Idle_Loop"),
              physicalCharacter(nullptr) {
            ModelRegistry::instance().registerBehaviour(name, this);
        }

        void onModelLoad(Model* model) override {
            std::cout << "Character Human just created\n";
            physicalCharacter = physics::createCharacter();
        }

        void sayHello() override { std::cout << "Hello from " << name << '\n'; }

        // Handle mouse movement for rotation
        void handleMouseMove(Model* model, MouseEvent event) override {
            // Sensitivity for mouse movement
            if (physicalCharacter == nullptr) return;
            auto mouse = std::get<Move>(event);
            float diffX = mouse.xPos - lastX;
            float diffY = mouse.yPos - lastY;
            lastX = mouse.xPos;
            lastY = mouse.yPos;

            float sensitivity = 0.3f;

            // Update yaw based on mouse X movement
            yaw -= diffX * sensitivity;

            glm::vec3 baseFront(0.0f, -1.0f, 0.0f);  // Model's forward after initial X rotation

            // Apply yaw rotation around the world Y-axis (since yaw is horizontal)
            glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 0.0f, 1.0f));
            front = glm::normalize(glm::vec3(yawRotation * glm::vec4(baseFront, 0.0f)));

            // Update model orientation
            glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::mat4 totalRotation = yawRotation * initialRotation;  // Combine yaw and initial X rotation

            // 1. +90° rotation around X (converts Y-up → Z-up orientation)
            JPH::Quat rot90X = JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::DegreesToRadians(90.0f));

            // 2. Yaw rotation around world Y (Jolt's up axis)
            JPH::Quat yawQuat = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), JPH::DegreesToRadians(-yaw));
            JPH::Quat finalRot = yawQuat * rot90X;

            auto y = finalRot.GetY();
            finalRot.SetY(finalRot.GetZ());
            finalRot.SetZ(-y);

            physicalCharacter->SetRotation(finalRot);
        }

        Model* getWeapon() override { return weapon; }

        void handleMouseClick(Model* model, MouseEvent event) override {
            auto mouse = std::get<Click>(event);
            if (mouse.click == GLFW_MOUSE_BUTTON_RIGHT) {
                if (mouse.action == GLFW_PRESS) {
                    state = Aiming;
                    isAiming = true;
                    if (!transitoin.active) {
                        transitoin.from = cameraOffset + glm::vec3{0.4};
                        transitoin.to = glm::vec3{0.3};
                        transitoin.active = true;
                        transitoin.timeLeft = 0.5;
                    }
                } else if (mouse.action == GLFW_RELEASE) {
                    state = Idle;
                    isAiming = false;
                    if (!transitoin.active) {
                        transitoin.from = glm::vec3{0.3};
                        transitoin.to = cameraOffset + glm::vec3{0.4};
                        transitoin.timeLeft = 0.5;
                        transitoin.active = true;
                    }
                }
            } else if (mouse.click == GLFW_MOUSE_BUTTON_LEFT) {
                isShooting = true;
            }
        }
        void handleMouseScroll(Model* model, MouseEvent event) override {
            auto scroll = std::get<Scroll>(event);
            cameraOffset += scroll.yOffset < 0 ? 0.1 : -0.1;
            targetDistance += scroll.yOffset < 0 ? 0.1 : -0.1;
        }

        void handleAttachedCamera(Model* model, Camera* camera) override {
            auto forward = getForward();

            // Compute the offset direction (modify as needed based on your coordinate system)
            glm::vec3 offset_dir = {forward.y, -forward.x, forward.z};  // Your existing transformation
            offset_dir = glm::normalize(offset_dir);                    // Normalize to get a unit vector

            // Apply the user-defined distance and any additional offset (e.g., vertical adjustment)
            glm::vec3 camera_offset = offset_dir * targetDistance + (-targetOffset);

            // Compute the camera position by subtracting the offset from the actor's position
            glm::vec3 cam_pos = model->mTransform.getPosition() - camera_offset;

            if (model->mBehaviour) {
                // Set the camera position
                camera->setPosition(cam_pos);
                // Set the camera to look at the actor's position
                camera->setTarget(model->mTransform.getPosition() + targetOffset - cam_pos);
            }
        }

        // Handle keyboard input for movement
        void handleKey(Model* model, KeyEvent event, float dt) override {
            auto space_value = InputManager::keys[GLFW_KEY_SPACE];
            if (space_value) {
                state = Jumping;
                isInJump = true;
            } else if (state == Jumping) {
                state = Falling;
            }

            if (InputManager::keys[GLFW_KEY_1]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "pistol") {
                        std::cout << "find pistol\n";
                        weapon = m;
                        idleAction = "Pistol_Idle_Loop";
                        attackAction = "Pistol_Shoot";
                        return;
                    }
                }
            }
            if (InputManager::keys[GLFW_KEY_2]) {
                for (auto* m : ModelRegistry::instance().getLoadedModel(Visibility_User)) {
                    if (m->mName == "sword") {
                        std::cout << "find sword\n";
                        weapon = m;
                        idleAction = "Sword_Idle";
                        attackAction = "Sword_Attack";
                        return;
                    }
                }
            }
        }

        glm::vec3 processInput() {
            glm::vec3 moveDir(0.0f);  // Movement direction relative to front vector
            bool noKey = true;
            if (InputManager::isKeyDown(GLFW_KEY_SPACE) && state != Jumping) {
                state = Jumping;
                moveDir.z += 1;
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_W)) {
                if (state != Jumping) {
                    state = Running;
                }
                moveDir += front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_S)) {
                state = Running;
                moveDir -= front;
                noKey = false;
            }
            if (InputManager::isKeyDown(GLFW_KEY_A)) {
                state = Running;
                moveDir += glm::normalize(glm::cross(front, up));
                noKey = false;
            }

            if (InputManager::isKeyDown(GLFW_KEY_D)) {
                state = Running;
                moveDir -= glm::normalize(glm::cross(front, up));
                noKey = false;
            }
            if (noKey) {
                state = Idle;
            }

            return moveDir;
        }

        void decideCurrentAnimation(Model* model, float dt) {
            static std::string last_clip;
            std::string clip_name;
            bool loop;
            model->anim->isEnded();
            if (isAiming) {
                if (isShooting && weapon != nullptr) {
                    if (!model->anim->isEnded()) {
                        clip_name = attackAction;
                        loop = false;
                    } else {
                        clip_name = idleAction;
                        loop = true;
                        isShooting = false;
                    }
                } else {
                    isShooting = false;
                    clip_name = idleAction;
                    loop = true;
                }
            } else {
                switch (state) {
                    case Idle: {
                        clip_name = "Idle_Loop";
                        loop = true;
                        break;
                    }
                    case Running: {
                        if (!isInJump) {
                            clip_name = "Jog_Fwd_Loop";
                            loop = true;
                        }
                        break;
                    }
                    case Falling:
                    case Jumping: {
                        clip_name = "Jump_Start";
                        loop = false;
                        break;
                    }
                    default:
                        clip_name = "Idle_Loop";
                        loop = true;
                        break;
                }
            }
            if (last_clip != clip_name) {
                model->anim->playAction(clip_name, loop);
                last_clip = clip_name;
            }
        }

        void update(Model* model, float dt) override {
            if (physicalCharacter == nullptr) return;
            auto movement = processInput();

            decideCurrentAnimation(model, dt);

            // auto& bodyInterface = physics::getBodyInterface();
            auto move_amount = movement * speed * dt;
            // bodyInterface.SetLinearVelocity(model->mPhysicComponent->bodyId, {move_amount.x, 0, 0});
            // if (move_amount.length() < 0.0001) {
            //     return;
            // }

            // model->moveBy(movement * speed * dt);

            if (state == Jumping && physicalCharacter->IsSupported()) {
                move_amount.z += 3.0;
                isInJump = true;
            }

            if (!physicalCharacter->IsSupported()) {
                // state = Idle;
                // isInJump = false;
            }

            JPH::Vec3 jolt_movement = {move_amount.x, physicalCharacter->GetLinearVelocity().GetY(), move_amount.y};
            physicalCharacter->SetLinearVelocity(jolt_movement);
            physics::updateCharacter(physicalCharacter, dt, jolt_movement);

            JPH::RVec3 new_position = physicalCharacter->GetPosition();
            JPH::Quat new_rotation = physicalCharacter->GetRotation();
            model->moveTo({new_position.GetX(), new_position.GetZ(), new_position.GetY()});

            glm::quat desired_rotation;

            desired_rotation.x = new_rotation.GetX();
            desired_rotation.y = new_rotation.GetY();
            desired_rotation.z = new_rotation.GetZ();
            desired_rotation.w = new_rotation.GetW();
            model->rotate(desired_rotation);

            // if (model->mTransform.mPosition.z < groundLevel) {
            //     model->mTransform.mPosition.z = groundLevel;
            //     velocity.z = 0;
            //     state = Idle;
            //     isInJump = false;
            // }

            if (transitoin.active) {
                transitoin.timeLeft -= dt;
                auto t = glm::clamp(transitoin.timeLeft, 0.0f, 1.0f);

                float eased_t = t * t * (3.0f - 2.0f * t);
                targetDistance = glm::mix(transitoin.from, transitoin.to, 1 - eased_t);

                if (transitoin.timeLeft <= 0) {
                    transitoin.active = false;
                }
            }
        }
        //
        glm::vec3 getForward() override { return glm::normalize(glm::cross(front, up)); }
};

HumanBehaviour humanbehaviour{"human"};

struct HumanModel2 : public IModel {
        HumanModel2(Application* app) {
            mModel = new Model{};

            mModel->load("human2", app, RESOURCE_DIR "/model2.dae", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1.0, 9.0, -3.7})
                .scale(glm::vec3{0.3})
                .rotate(glm::vec3{0.0, 0.0, 0.0}, 0.);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);

            mModel->mSkiningTransformationBuffer.setLabel("default skining data transform for human 2")
                .setSize(100 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .create(app);

            std::vector<glm::mat4> bones;
            for (int i = 0; i < 100; i++) {
                bones.emplace_back(glm::mat4{1.0});
            }
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mSkiningTransformationBuffer.getBuffer(), 0,
                                 bones.data(), sizeof(glm::mat4) * bones.size());

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }

        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct RobotModel : public IModel {
        RobotModel(Application* app) {
            mModel = new Model{Z_UP};

            mModel->load("robot", app, RESOURCE_DIR "/dance2.gltf", app->getObjectBindGroupLayout())
                .scale(glm::vec3{1.0})
                .rotate(glm::vec3{90.0, 0.0, 0.0}, 0.);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);

            mModel->mSkiningTransformationBuffer.setLabel("default skining data transform")
                .setSize(100 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                .setMappedAtCraetion(false)
                .create(app);

            static std::vector<glm::mat4> bones;
            for (int i = 0; i < 100; i++) {
                bones.emplace_back(glm::mat4{1.0});
            }
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mSkiningTransformationBuffer.getBuffer(), 0,
                                 bones.data(), sizeof(glm::mat4) * bones.size());

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }

        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};
struct StonesModel : public IModel {
        StonesModel(Application* app) {
            mModel = new Model{};

            mModel->load("stones", app, RESOURCE_DIR "/stones/stones.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.0, -1.0, -2.58})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{0.5});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

struct PlatformModel : public IModel {
        PlatformModel(Application* app) {
            mModel = new Model{};

            mModel->load("platform", app, RESOURCE_DIR "/platform.gltf", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.0, -1.0, -3.68})
                .rotate(glm::vec3{0.0f, 0.0f, 0.0f}, 0.0)
                .scale(glm::vec3{3.370, 3.370, 0.860});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            // mModel->mTransform.mObjectInfo.uvMultiplier = {50, 50.0, 1.0};
            mModel->mTransform.mDirty = true;
        };
};
// USER_REGISTER_MODEL("tree", TreeModel);
// USER_REGISTER_MODEL("boat", BoatModel);
// USER_REGISTER_MODEL("car", CarModel);
// USER_REGISTER_MODEL("tower", TowerModel);

// USER_REGISTER_MODEL("desk", DeskModel);
// USER_REGISTER_MODEL("coniferous", ConiferousModel);
//
// USER_REGISTER_MODEL("grass", GrassModel);
// USER_REGISTER_MODEL("steampunk", Steampunk);
// USER_REGISTER_MODEL("sheep", SheepModel);

// USER_REGISTER_MODEL("sphere", SphereModel);
// USER_REGISTER_MODEL("platform", PlatformModel);
//
// USER_REGISTER_MODEL("human", HumanModel);  // Upper_Arm.L
// USER_REGISTER_MODEL("human2", HumanModel2);
// USER_REGISTER_MODEL("robot", RobotModel);
// USER_REGISTER_MODEL("stones", StonesModel);
// USER_REGISTER_MODEL("cube", CubeModel);
// USER_REGISTER_MODEL("pistol", PistolModel);

// USER_REGISTER_MODEL("house", HouseModel);
// USER_REGISTER_MODEL("motor", Motor);
/*USER_REGISTER_MODEL("jet", JetModel);*/
