#include <cstdint>
#include <random>

#include "application.h"
#include "glm/fwd.hpp"
#include "instance.h"
#include "model.h"
#include "model_registery.h"

struct TreeModel : public IModel {
        TreeModel(Application* app) {
            mModel = new Model{};
            mModel->load("tree", app, RESOURCE_DIR "/tree3.gltf", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{0.725, -7.640, 1.125})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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

            for (size_t i = 0; i < app->terrainData.size(); i++) {
                if (i % 40 == 0) {
                    positions.emplace_back(
                        glm::vec3(app->terrainData[i].x, app->terrainData[i].y, app->terrainData[i].z));
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
            std::cout << "--------------------Barrier reached -----------------\n";
        };
};

struct BoatModel : public IModel {
        BoatModel(Application* app) {
            mModel = new Model{};
            mModel->load("boat", app, RESOURCE_DIR "/fourareen.obj", app->getObjectBindGroupLayout())
                .mTransform.scale(glm::vec3{0.8})
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
            mModel->mTransform.moveTo(glm::vec3{-3.950, 13.316, -3.375});
        };
};

struct JetModel : public IModel {
        JetModel(Application* app) {
            mModel = new Model{};
            mModel->load("jet", app, RESOURCE_DIR "/old_jet3.obj", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{5.0, -1., 2.})
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
                .mTransform
                .moveTo(glm::vec3{-2.883, -6.280, -1.709})
                // .scale(glm::vec3{1.0})
                .rotate(glm::vec3{0.0, 0.0, 0.0}, 0.0f)
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0);
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
                .mTransform.moveTo(glm::vec3{-2.0, -1.0, -2.58})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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
                .mTransform.moveTo(glm::vec3{1.0f, 1.0f, 4.0f})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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
                .mTransform.moveTo(glm::vec3{0.725, 0.333, 0.72})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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
                .mTransform.moveTo(glm::vec3{6.125, 2.239, -2.859})
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

            for (size_t i = 0; i < app->terrainData.size(); i++) {
                if (i % 5 == 0) {
                    positions.emplace_back(
                        glm::vec3{app->terrainData[i].x, app->terrainData[i].y, app->terrainData[i].z});
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
                .mTransform.moveTo(glm::vec3{0.725, -1.0, 0.72})
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
                .mTransform  //.moveTo(glm::vec3{2.187, 9.227, 0.0})
                .scale(glm::vec3{0.1f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }

        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;

            // std::vector<glm::mat4> dddata = {};

            // std::vector<glm::vec3> positions;
            // std::vector<float> degrees;
            // std::vector<glm::vec3> scales;
            // positions.reserve(10);
            // degrees.reserve(10);
            // scales.reserve(10);
            //
            // for (const auto& m : mModel->mBonePosition) {
            //     positions.emplace_back(m);
            //     degrees.emplace_back(0.0);
            //     scales.emplace_back(glm::vec3{0.1f});
            // }
            // // positions.emplace_back(glm::vec3{4.187, 9.227, 0.0});
            // // degrees.emplace_back(0.0);
            // // scales.emplace_back(glm::vec3{0.1f});
            // //
            // // positions.emplace_back(glm::vec3{5.187, 9.227, 0.0});
            // // degrees.emplace_back(0.0);
            // // scales.emplace_back(glm::vec3{0.1f});
            // //
            // // positions.emplace_back(glm::vec3{6.187, 9.227, 0.0});
            // // degrees.emplace_back(0.0);
            // // scales.emplace_back(glm::vec3{0.1f});
            // //
            // // positions.emplace_back(glm::vec3{7.187, 9.227, 0.0});
            // // degrees.emplace_back(0.0);
            // // scales.emplace_back(glm::vec3{0.1f});
            // //
            // // positions.emplace_back(glm::vec3{8.187, 9.227, 0.0});
            // // degrees.emplace_back(0.0);
            // // scales.emplace_back(glm::vec3{0.1f});
            //
            // auto* ins = new Instance{positions, glm::vec3{0.0, 0.0, 1.0},     degrees,
            //                          scales,    glm::vec4{mModel->min, 1.0f}, glm::vec4{mModel->max, 1.0f}};
            //
            // wgpuQueueWriteBuffer(app->getRendererResource().queue,
            //                      app->mInstanceManager->getInstancingBuffer().getBuffer(), 0,
            //                      ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size() -
            //                      1));
            //
            // mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + mModel->getName()).c_str())
            //     .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
            //               WGPUBufferUsage_CopyDst)
            //     .setSize(sizeof(DrawIndexedIndirectArgs))
            //     .setMappedAtCraetion()
            //     .create(app);
            //
            // auto indirect = DrawIndexedIndirectArgs{0, 0, 0, 0, 0};
            // wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mIndirectDrawArgsBuffer.getBuffer(), 0,
            //                      &indirect, sizeof(DrawIndexedIndirectArgs));
            //
            // for (auto& [mat_id, mesh] : mModel->mMeshes) {
            //     mesh.mIndirectDrawArgsBuffer.setLabel(("indirect_draw_args_mesh_ " + mModel->getName()).c_str())
            //         .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopyDst)
            //         .setSize(sizeof(DrawIndexedIndirectArgs))
            //         .setMappedAtCraetion()
            //         .create(app);
            //
            //     auto indirect = DrawIndexedIndirectArgs{static_cast<uint32_t>(mesh.mIndexData.size()), 0, 0, 0, 0};
            //     wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0,
            //                          &indirect, sizeof(DrawIndexedIndirectArgs));
            // }
            //
            // mModel->mTransform.mObjectInfo.instanceOffsetId = 0;
            // mModel->setInstanced(ins);
            // std::cout << positions.size() << " --------------------Barrier reached -----------------\n";
        };
};

struct Steampunk : public IModel {
        Steampunk(Application* app) {
            mModel = new Model{};

            mModel->load("steampunk", app, RESOURCE_DIR "/steampunk.obj", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{-1.45, -3.239, -0.810})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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
                .mTransform.moveTo(glm::vec3{-2.45, -3.239, -0.810})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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

            mModel->load("sheep", app, RESOURCE_DIR "/body.dae", app->getObjectBindGroupLayout())
                .mTransform
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

            mModel->load("house", app, RESOURCE_DIR "/house/scene.gltf", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{-0.483, 52.048, -3.309})
                .rotate(glm::vec3{90, 0.0, 324.0}, 0.0)
                .scale(glm::vec3{2.601f});
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

            mModel->load("cube", app, RESOURCE_DIR "/new_cube.obj", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{-6.883, 3.048, -1.709})
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

struct WaterModel : public IModel {
        WaterModel(Application* app) {
            mModel = new Model{};

            mModel->load("water", app, RESOURCE_DIR "/waterplane.glb", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{52.275, 56.360, -3.5})
                .scale(glm::vec3{100.0, 100.0, 1.0});
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
            mModel = new Model{};

            mModel->load("human", app, RESOURCE_DIR "/model2.dae", app->getObjectBindGroupLayout())
                .mTransform.moveTo(glm::vec3{1.0, 9.0, -3.7})
                .scale(glm::vec3{0.3})
                .rotate(glm::vec3{0.0, 0.0, 0.0}, 0.);
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

struct RobotModel : public IModel {
        RobotModel(Application* app) {
            mModel = new Model{};

            mModel->load("robot", app, RESOURCE_DIR "/dance2.dae", app->getObjectBindGroupLayout())
                .mTransform  //.moveTo(glm::vec3{0.0, 9.0, -3.7})
                .scale(glm::vec3{1.0})
                .rotate(glm::vec3{0.0, 0.0, 0.0}, 0.);
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
                .mTransform.moveTo(glm::vec3{-2.0, -1.0, -2.58})
                .rotate(glm::vec3{180.0f, 0.0f, 0.0f}, 0.0)
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

USER_REGISTER_MODEL("tree", TreeModel);
USER_REGISTER_MODEL("boat", BoatModel);
USER_REGISTER_MODEL("car", CarModel);
USER_REGISTER_MODEL("tower", TowerModel);

// USER_REGISTER_MODEL("desk", DeskModel);
USER_REGISTER_MODEL("coniferous", ConiferousModel);
//
// USER_REGISTER_MODEL("grass", GrassModel);
USER_REGISTER_MODEL("steampunk", Steampunk);
USER_REGISTER_MODEL("sheep", SheepModel);
// USER_REGISTER_MODEL("water", WaterModel);

// USER_REGISTER_MODEL("sphere", SphereModel);
//
USER_REGISTER_MODEL("human", HumanModel);
USER_REGISTER_MODEL("robot", RobotModel);
USER_REGISTER_MODEL("stones", StonesModel);
// USER_REGISTER_MODEL("cube", CubeModel);
//
USER_REGISTER_MODEL("house", HouseModel);
// USER_REGISTER_MODEL("motor", Motor);
/*USER_REGISTER_MODEL("jet", JetModel);*/
