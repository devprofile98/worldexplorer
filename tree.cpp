#include <random>

#include "application.h"
#include "model_registery.h"

struct TreeModel : public IModel {
        TreeModel(Application* app) {
            mModel = new Model{};
            mModel->load("tree", app, RESOURCE_DIR "/tree2.obj", app->getObjectBindGroupLayout())
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
                auto scale = glm::scale(glm::mat4{1.0f}, glm::vec3{0.1f * dist(gen)});
                if (i % 40 == 0) {
                    dddata.push_back(trans * rotate * scale);
                }
            }
            auto* ins = new Instance{dddata};
            mModel->setInstanced(ins);
            mModel->mObjectInfo.instanceOffsetId = 1;
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

REGISTER_MODEL("tree", TreeModel);
REGISTER_MODEL("boat", BoatModel);
REGISTER_MODEL("jet", JetModel);
