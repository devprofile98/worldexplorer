#include <random>

#include "application.h"
#include "model_registery.h"

struct TreeModel : public IModel {
        TreeModel(Application* app) {
            mModel = new Model{};
            mModel->load("tree", app, RESOURCE_DIR "/tree1.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, -7.640, 1.125})
                .scale(glm::vec3{0.9});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override {
            std::random_device rd;   // Seed the random number generator
            std::mt19937 gen(rd());  // Mersenne Twister PRNG
            std::uniform_real_distribution<float> dist_for_rotation(0.0, 90.0);
            std::uniform_real_distribution<float> dist(1.9, 2.5);
            std::vector<glm::mat4> dddata;

            (void)app;
            for (size_t i = 0; i < app->terrainData.size(); i++) {
                glm::vec3 position = glm::vec3(app->terrainData[i].x, app->terrainData[i].y, app->terrainData[i].z);
                auto trans = glm::translate(glm::mat4{1.0f}, position);
                auto rotate =
                    glm::rotate(glm::mat4{1.0f}, glm::radians(dist_for_rotation(gen)), glm::vec3{0.0, 0.0, 1.0});
                auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.9f * dist(gen)});
                if (i % 200 == 0) {
                    dddata.push_back(trans * rotate * scale);
                }
            }
            auto* ins = new Instance{dddata};
            mModel->setInstanced(ins);
            mModel->mObjectInfo.instanceOffsetId = 1;

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(), 100000 * sizeof(glm::mat4),
                                 dddata.data(), sizeof(glm::mat4) * (dddata.size() - 1));
        };
};

struct BoatModel : public IModel {
        BoatModel(Application* app) {
            mModel = new Model{};
            mModel->load("boat", app, RESOURCE_DIR "/fourareen.obj", app->getObjectBindGroupLayout())
                .scale(glm::vec3{0.8});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override {
            (void)app;
            mModel->moveTo(glm::vec3{2.0, -1., 1.});
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
        void onLoad(Application* app) override { (void)app; };
};

struct CarModel : public IModel {
        CarModel(Application* app) {
            mModel = new Model{};

            mModel->load("car", app, RESOURCE_DIR "/ford.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-6.883, 3.048, -1.709})
                .scale(glm::vec3{0.005})
                .rotate(glm::vec3{0.0, 0.0, 1.0}, 90.0f);
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->useTexture(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override { (void)app; };
};

struct TowerModel : public IModel {
        TowerModel(Application* app) {
            mModel = new Model{};

            mModel->load("tower", app, RESOURCE_DIR "/tower.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{-2.0, -1.0, -2.58})
                .scale(glm::vec3{0.5});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override { (void)app; };
};

struct ArrowModel : public IModel {
        ArrowModel(Application* app) {
            mModel = new Model{};

            mModel->load("arrow", app, RESOURCE_DIR "/arrow.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{1.0f, 1.0f, 4.0f})
                .scale(glm::vec3{0.2});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override { (void)app; };
};

struct DeskModel : public IModel {
        DeskModel(Application* app) {
            mModel = new Model{};

            mModel->load("desk", app, RESOURCE_DIR "/desk.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, 0.333, 0.72})
                .scale(glm::vec3{0.3});
            mModel->uploadToGPU(app);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override { (void)app; };
};

struct GrassModel : public IModel {
        GrassModel(Application* app) {
            mModel = new Model{};

            mModel->load("grass", app, RESOURCE_DIR "/grass.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.725, -1.0, 0.72})
                .scale(glm::vec3{0.3});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->setFoliage();
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override {
            (void)app;

            std::vector<glm::mat4> dddata = {};

            std::random_device rd;   // Seed the random number generator
            std::mt19937 gen(rd());  // Mersenne Twister PRNG
            std::uniform_real_distribution<float> dist(1.5, 2.5);
            std::uniform_real_distribution<float> dist_for_rotation(0.0, 90.0);
            std::uniform_real_distribution<float> dist_for_tree(1.9, 2.5);
            std::uniform_real_distribution<float> dist_for_grass(1.0, 1.8);

            for (size_t i = 0; i < app->terrainData.size(); i++) {
                glm::vec3 position = glm::vec3(app->terrainData[i].x, app->terrainData[i].y, app->terrainData[i].z);
                auto trans = glm::translate(glm::mat4{1.0f}, position);
                auto rotate =
                    glm::rotate(glm::mat4{1.0f}, glm::radians(dist_for_rotation(gen)), glm::vec3{0.0, 0.0, 1.0});
                auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f * dist(gen)});
                dddata.push_back(trans * rotate * scale);
            }

            auto* ins = new Instance{dddata};
            mModel->setInstanced(ins);
            mModel->mObjectInfo.instanceOffsetId = 0;

            wgpuQueueWriteBuffer(app->getRendererResource().queue,
                                 app->mInstanceManager->getInstancingBuffer().getBuffer(), 0, dddata.data(),
                                 sizeof(glm::mat4) * (dddata.size() - 1));
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
        void onLoad(Application* app) override { (void)app; };
};

struct SphereModel : public IModel {
        SphereModel(Application* app) {
            mModel = new Model{};

            mModel->load("sphere", app, RESOURCE_DIR "/sphere.obj", app->getObjectBindGroupLayout())
                .moveTo(glm::vec3{0.0, 0.0, 0.0})
                .scale(glm::vec3{1.0f});
            mModel->uploadToGPU(app);
            mModel->setTransparent(false);
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app) override { (void)app; };
};

REGISTER_MODEL("tree", TreeModel);
REGISTER_MODEL("boat", BoatModel);
/*REGISTER_MODEL("jet", JetModel);*/
REGISTER_MODEL("car", CarModel);
REGISTER_MODEL("tower", TowerModel);
REGISTER_MODEL("desk", DeskModel);
REGISTER_MODEL("arrow", ArrowModel);
REGISTER_MODEL("grass", GrassModel);
/*REGISTER_MODEL("cylinder", CylinderModel);*/
/*REGISTER_MODEL("sphere", SphereModel);*/
