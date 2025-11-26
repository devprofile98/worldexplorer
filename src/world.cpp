
#include "world.h"

#include <array>
#include <fstream>
#include <glm/fwd.hpp>
#include <iostream>
#include <optional>
#include <utility>

#include "animation.h"
#include "application.h"
#include "extern/json.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "model.h"
#include "model_registery.h"
#include "physics.h"
#include "point_light.h"
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

World::World(Application* app) : app(app) {
    auto& input_manager = InputManager::instance();
    input_manager.mKeyListener.push_back(this);
    input_manager.mMouseMoveListeners.push_back(this);
    input_manager.mMouseButtonListeners.push_back(this);
    input_manager.mMouseScrollListeners.push_back(this);
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

void World::togglePlayer() {
    if (actor == nullptr) {
        for (const auto& model : rootContainer) {
            if (model->mName == actorName) {
                actor = model;
                return;
            }
        }
    } else {
        actor = nullptr;
    }
}

void World::onNewModel(Model* loadedModel) {
    rootContainer.push_back(loadedModel);
    // Set the active actor
    if (actorName == loadedModel->mName) {
        actor = loadedModel;
    }
    // traverse the world to check if the parent and which are its childs
    // There are 2 questions for the newly loaded model:
    // 1 - Is any of the loaded models your child in term of parent/child relations?
    for (auto* model : rootContainer) {
        if (map.contains(loadedModel->mName)) {
            for (const auto& name : map.at(loadedModel->mName).childrens) {
                if (model->mName == name) {
                    removeParent(model);
                    makeChild(loadedModel, model);
                    std::cout << "@@@@@@@@@@@@@@@@ " << loadedModel->mName << " is the parent for " << model->mName
                              << '\n';
                    goto stage2;
                }
            }
        }
    }
stage2:
    // 2 - are you a child for any of these loaded models?
    for (auto* model : rootContainer) {
        if (map.contains(model->mName)) {
            for (const auto& name : map.at(model->mName).childrens) {
                if (name == loadedModel->mName) {
                    removeParent(loadedModel);
                    makeChild(model, loadedModel);
                    std::cout << "@@@@@@@@@@@@@@@@ " << loadedModel->mName << " is a child for " << model->mName
                              << '\n';
                    goto load_socket;
                }
            }
        }
    }
load_socket:

    // for (auto* model : rootContainer) {
    //     if (map.contains(model->mName)) {
    {
        if (map.contains(loadedModel->mName)) {
            auto param = map.at(loadedModel->mName).socketParam;
            Model* target = nullptr;
            if (param.isValid) {
                for (auto* suspect : rootContainer) {
                    if (suspect->mName == param.name) {
                        target = suspect;
                        break;
                    }
                }
                if (target != nullptr) {
                    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@ " << param.bone << " Is the attaching bone for "
                              << loadedModel->mName << "\n";
                    loadedModel->mSocket =
                        new BoneSocket{target, param.bone, param.translate, param.scale, param.rotate};
                    goto end;
                }
            }
        }
    }

    for (auto* model : rootContainer) {
        if (!map.contains(model->mName)) {
            continue;
        }
        auto param = map.at(model->mName).socketParam;
        // auto loaded_model_param = map.at(loadedModel->mName).socketParam;
        if (param.name == loadedModel->mName) {
            std::cout << "@@@@@@@@@@1@@@@@@@@@@@@@@@@ " << param.bone << " from " << loadedModel->mName
                      << " Is the attaching bone for " << model->mName << "\n";
            model->mSocket = new BoneSocket{loadedModel, param.bone, param.translate, param.scale, param.rotate};
        }
        // if (map.contains(model->mName)) {
        // }
    }

end:
}

void World::onKey(KeyEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    /* Send the event to every registered model with a behaviour*/
    for (const auto& model : models) {
        if (model->mBehaviour != nullptr) {
            model->mBehaviour->handleKey(model, event, delta);
        }
    }
}

void World::onMouseMove(MouseEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    /* Send the event to every registered model with a behaviour*/
    for (const auto& model : models) {
        if (model->mBehaviour != nullptr) {
            model->mBehaviour->handleMouseMove(model, event);
        }
    }
}

void World::onMouseClick(MouseEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    for (const auto& model : models) {
        if (model->mBehaviour != nullptr) {
            model->mBehaviour->handleMouseClick(model, event);
        }
    }
}

void World::onMouseScroll(MouseEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    for (const auto& model : models) {
        if (model->mBehaviour != nullptr) {
            model->mBehaviour->handleMouseScroll(model, event);
        }
    }
}

glm::vec3 toGlm(std::array<float, 3> arr) { return glm::vec3{arr[0], arr[1], arr[2]}; }

ObjectLoaderParam::ObjectLoaderParam(std::string name, std::string path, bool animated, CoordinateSystem cs,
                                     Vec translate, Vec scale, Vec rotate, std::vector<std::string> childrens,
                                     std::string defaultClip, SocketParams socketParam)
    : socketParam(socketParam),
      name(name),
      path(path),
      animated(animated),
      cs(cs),
      childrens(childrens),
      defaultClip(defaultClip) {
    this->translate = toGlm(translate);
    this->scale = toGlm(scale);
    this->rotate = toGlm(rotate);
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

            if (getModel()->mTransform.mObjectInfo.isAnimated) {
                for (auto [name, action] : getModel()->anim->actions) {
                    action->loop = true;
                }
                getModel()->anim->playAction(param.defaultClip, true);
            }
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
            // getModel()->mBehaviour->app = app;
        };
};

void parseLights(LightManager* manager, const json& lights) {
    // light_manager->createPointLight({2.0, 2.0, 1.0, 1.0}, red, red, red, 1.0, 0.7, 1.8, "blue light 3");
    std::cout << lights << '\n';
    for (auto& light_obj : lights) {
        std::string type = light_obj["type"].get<std::string>();
        std::string name = light_obj["name"].get<std::string>();
        std::array<float, 3> color = light_obj["color"].get<std::array<float, 3>>();
        std::array<float, 3> position = light_obj["position"].get<std::array<float, 3>>();
        if (type == "spot") {
            std::cout << "One spot light" << color[0] << " " << color[1] << " " << color[2] << std::endl;
            std::array<float, 3> direction = light_obj["direction"].get<std::array<float, 3>>();
            float inner_cutoff = light_obj["inner_cutoff"].get<float>();
            float outer_cutoff = light_obj["outer_cutoff"].get<float>();
            float linear = light_obj["linear"].get<float>();
            float quadratic = light_obj["quadratic"].get<float>();
            manager->createSpotLight(glm::vec4{toGlm(position), 1.0}, glm::vec4{toGlm(direction), 1.0},
                                     glm::vec4{toGlm(color), 1.0}, inner_cutoff, outer_cutoff, linear, quadratic,
                                     name.c_str());
        }
    }
}

std::optional<SocketParams> parseSocketParam(const json& params) {
    if (params.is_null()) {
        return std::nullopt;
    }
    std::string name = params["model_name"].get<std::string>();
    std::string bone_name = params["bone_name"].get<std::string>();

    glm::vec3 translate = toGlm(params["offset"]["translate"].get<std::array<float, 3>>());
    glm::vec3 scale = toGlm(params["offset"]["scale"].get<std::array<float, 3>>());
    glm::vec3 rotate = toGlm(params["offset"]["rotate"].get<std::array<float, 3>>());
    return SocketParams{name, bone_name, translate, scale, glm::quat{1.0, rotate}, true};
}

std::optional<PhysicsParams> parsePhysics(const json& params) {
    if (params.is_null()) {
        return std::nullopt;
    }
    std::string type = params["type"].get<std::string>();

    return PhysicsParams{type};
}

void World::loadWorld() {
    std::ifstream world_file(RESOURCE_DIR "/world.json");

    json j;
    json res = j.parse(world_file);

    actorName = res["actor"].get<std::string>();

    std::cout << res << std::endl;
    for (const auto& object : res["objects"]) {
        std::cout << "Object " << object["name"] << " at " << object["path"] << "\n";

        std::string name = object["name"].get<std::string>();
        std::string path = object["path"].get<std::string>();
        bool is_enabled = object["enabled"].get<bool>();
        if (!is_enabled) {
            continue;
        }
        bool is_animated = object["animated"].get<bool>();
        std::string _cs = object["cs"].get<std::string>();
        CoordinateSystem cs = _cs == "z" ? Z_UP : Y_UP;
        std::array<float, 3> translate = object["translate"].get<std::array<float, 3>>();
        std::array<float, 3> scale = object["scale"].get<std::array<float, 3>>();
        std::array<float, 3> rotate = object["rotate"].get<std::array<float, 3>>();
        std::vector<std::string> childs = object["childrens"].get<std::vector<std::string>>();
        std::string default_clip = object["default_clip"].get<std::string>();
        auto physics_props = parsePhysics(object["physics"]);
        SocketParams socket_param;
        socket_param.isValid = false;

        auto socket_opt = parseSocketParam(object["socket"]);
        if (socket_opt.has_value()) {
            socket_param = socket_opt.value();
            socket_param.isValid = true;
        }

        ObjectLoaderParam param{name,  path,   is_animated, cs,           translate,
                                scale, rotate, childs,      default_clip, socket_param};
        param.isDefaultActor = actorName == name;
        if (physics_props.has_value()) {
            param.isPhysicEnabled = true;
            param.physicsParams = *physics_props;
        }

        ModelRegistry::instance().registerModel(name, [param](Application* app) -> LoadModelResult {
            BaseModelLoader model = BaseModelLoader{app, param};
            model.onLoad(app, nullptr);

            if (param.isDefaultActor) {
            }
            if (param.isPhysicEnabled) {
                glm::vec3 euler_radians = glm::radians(param.rotate);
                auto qu = glm::normalize(glm::quat(euler_radians));
                auto [min, max] = model.getModel()->getWorldSpaceAABB();
                auto center = (min + max) / 2.0f;
                auto diff = (max - min) / 2.0f;
                model.getModel()->mPhysicComponent = physics::createAndAddBody(
                    diff, center, qu, param.physicsParams.type == "dynamic" ? true : false, 0.5, 0.0f, 0.0f, 0.1f);
            }

            return {model.getModel(), Visibility_User};
        });
        map.emplace(name, param);
    }

    auto* light_manager = app->mLightManager;
    parseLights(light_manager, res["lights"]);
    app->mLightManager->uploadToGpu(app, app->mLightBuffer.getBuffer());
}
