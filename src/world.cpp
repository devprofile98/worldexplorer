
#include "world.h"

#include <fstream>
#include <glm/fwd.hpp>
#include <iostream>
#include <utility>

#include "application.h"
#include "extern/json.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "model.h"
#include "model_registery.h"
#include "utils.h"

using json = nlohmann::json;

TransformProperties calculateChildTransform(Model* parent, Model* child) {
    auto new_local_transform = glm::inverse(parent->getGlobalTransform()) * child->mTransform.mTransformMatrix;
    return decomposeTransformation(new_local_transform);
}

Model* World::makeChild(Model* parent, Model* child) {
    // remove from the last parent
    removeParent(child);

    // calculate new pos/rot/scale according to the new parent
    auto [pos, scale, rot] = calculateChildTransform(parent, child);
    // std::cout << glm::to_string(pos) << " Is the calculated child transform\n";
    child->moveTo(pos).scale(scale).rotate(rot);
    child->mTransform.getLocalTransform();

    // assign to the new parent
    child->mParent = parent;
    parent->mChildrens.push_back(child);

    return nullptr;
}

void World::removeParent(Model* child) {
    /* In this case, the model should be in the rootContainer of the world, we should remove it from here*/
    if (child->mParent == nullptr) {
        auto& vec = rootContainer;
        for (size_t i = 0; i < vec.size(); ++i) {
            auto* model = vec[i];
            if (model == child) {
                // calculate new poistion as global world space position
                // std::swap(model, vec.back());
                vec[i] = vec.back();
                vec.pop_back();
            }
        }
        return;
    }

    auto& vec = child->mParent->mChildrens;
    for (size_t i = 0; i < vec.size(); ++i) {
        auto* model = vec[i];
        if (model == child) {
            // calculate new poistion as global world space position
            std::swap(model, vec.back());
            vec.pop_back();
        }
    }
}

void World::onNewModel(Model* loadedModel) {
    rootContainer.push_back(loadedModel);
    std::cout << "Model " << loadedModel->mName << " Loaded at " << map.at(loadedModel->getName()).path << std::endl;
    // traverse the world to check if the parent and which are its childs
    // There are 2 questions for the newly loaded model:
    // 1 - Is any of the loaded models your child?
    for (auto* model : rootContainer) {
        for (const auto& name : map.at(loadedModel->mName).childrens) {
            if (model->mName == name) {
                removeParent(model);
                makeChild(loadedModel, model);
                std::cout << "@@@@@@@@@@@@@@@@ " << loadedModel->mName << " is the parent for " << model->mName << '\n';
                goto stage2;
            }
        }
    }
stage2:
    // 2 - are you a child for any of these loaded models?
    for (auto* model : rootContainer) {
        for (const auto& name : map.at(model->mName).childrens) {
            if (name == loadedModel->mName) {
                removeParent(loadedModel);
                makeChild(model, loadedModel);
                std::cout << "@@@@@@@@@@@@@@@@ " << loadedModel->mName << " is a child for " << model->mName << '\n';
                goto end;
            }
        }
    }
end:
}

ObjectLoaderParam::ObjectLoaderParam(std::string name, std::string path, bool animated, CoordinateSystem cs,
                                     Vec translate, Vec scale, Vec rotate, std::vector<std::string> childrens)
    : name(name), path(path), animated(animated), cs(cs), childrens(childrens) {
    this->translate = glm::vec3{translate[0], translate[1], translate[2]};
    this->scale = glm::vec3{scale[0], scale[1], scale[2]};
    this->rotate = glm::vec3{rotate[0], rotate[1], rotate[2]};
}

struct BaseModelLoader : public IModel {
        BaseModelLoader(Application* app, ObjectLoaderParam param) {
            mModel = new Model{param.cs};
            mModel->load(param.name, app, RESOURCE_DIR + std::string{"/"} + param.path, app->getObjectBindGroupLayout())
                .moveTo(param.translate)
                .scale(param.scale)
                .rotate(param.rotate, 0.0);
            mModel->uploadToGPU(app);
            // mModel->setTransparent(false);
            // mModel->setFoliage();
            //
            //
            //

            if (param.animated) {
                mModel->mSkiningTransformationBuffer.setLabel("default skining data transform for human")
                    .setSize(100 * sizeof(glm::mat4))
                    .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                    .create(app);

                std::vector<glm::mat4> bones;
                for (int i = 0; i < 100; i++) {
                    bones.emplace_back(glm::mat4{1.0});
                }
                wgpuQueueWriteBuffer(app->getRendererResource().queue, mModel->mSkiningTransformationBuffer.getBuffer(),
                                     0, bones.data(), sizeof(glm::mat4) * bones.size());
            }
            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            // mModel->moveTo(glm::vec3{-3.950, 13.316, -3.375});
        };
};

void World::loadWorld() {
    std::ifstream world_file(RESOURCE_DIR "/world.json");

    json j;
    json res = j.parse(world_file);
    std::cout << res << std::endl;
    for (const auto& object : res["objects"]) {
        std::cout << "Object " << object["name"] << " at " << object["path"] << "\n";

        std::string name = object["name"].get<std::string>();
        std::string path = object["path"].get<std::string>();
        bool is_animated = object["animated"].get<bool>();
        std::string _cs = object["cs"].get<std::string>();
        CoordinateSystem cs = _cs == "z" ? Z_UP : Y_UP;
        std::array<float, 3> translate = object["translate"].get<std::array<float, 3>>();
        std::array<float, 3> scale = object["scale"].get<std::array<float, 3>>();
        std::array<float, 3> rotate = object["rotate"].get<std::array<float, 3>>();
        std::vector<std::string> childs = object["childrens"].get<std::vector<std::string>>();

        ObjectLoaderParam param{name, path, is_animated, cs, translate, scale, rotate, childs};

        ModelRegistry::instance().registerModel(name, [param](Application* app) -> LoadModelResult {
            BaseModelLoader model = BaseModelLoader{app, param};
            model.onLoad(app, nullptr);
            return {model.getModel(), Visibility_User};
        });
        map.emplace(name, param);
    }
}
