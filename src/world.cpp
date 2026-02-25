
#include "world.h"

#include <array>
#include <fstream>
#include <functional>
#include <glm/fwd.hpp>
#include <ios>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "animation.h"
#include "application.h"
#include "extern/json.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "instance.h"
#include "model.h"
#include "model_registery.h"
#include "physics.h"
#include "point_light.h"
#include "utils.h"
#include "webgpu/webgpu.h"

using json = nlohmann::json;
using AddColliderFunction = std::function<uint32_t(const std::string&, const glm::vec3&, const glm::vec3&, bool)>;

void to_json(json& j, const PhysicsComponent& p) { j = json{{"enabled", true}, {"type", "dynamic"}}; }

void to_json(json& j, const Light& light, const std::string& name) {
    j = json();
    j["name"] = name;
    j["color"] = json::array({light.mAmbient.x, light.mAmbient.y, light.mAmbient.z});
    j["position"] = json::array({light.mPosition.x, light.mPosition.y, light.mPosition.z});
    j["linear"] = light.mLinear;
    j["quadratic"] = light.mQuadratic;

    if (light.type == LightType::SPOT) {
        j["type"] = "spot";
        j["direction"] = json::array({light.mDirection.x, light.mDirection.y, light.mDirection.z});
        j["inner_cutoff"] = light.mInnerCutoff;
        j["outer_cutoff"] = light.mOuterCutoff;
    } else if (light.type == LightType::POINT) {
        j["type"] = "point";
        j["constant"] = light.mConstant;
    }
}

void to_json(json& j, const physics::BoxCollider& cld) {
    j = json();
    j["active"] = true;
    j["name"] = cld.mName;
    j["type"] = "static";
    j["shape"] = "box";
    j["center"] = json::array({cld.mCenter.x, cld.mCenter.y, cld.mCenter.z});
    j["half_extent"] = json::array({cld.mHalfExtent.x, cld.mHalfExtent.y, cld.mHalfExtent.z});
}

void to_json(json& j, const Instance& ins) {
    j = json::array();
    std::cout << ins.mPositions.size() << " " << ins.mScale.size() << " " << ins.mRotation.size() << std::endl;
    for (size_t i = 0; i < ins.mPositions.size(); ++i) {
        json obj;
        obj["position"] = json::array({ins.mPositions[i].x, ins.mPositions[i].y, ins.mPositions[i].z});
        obj["scale"] = json::array({ins.mScale[i].x, ins.mScale[i].y, ins.mScale[i].z});
        obj["rotation"] = json::array({0.0, 0.0, 0.0});
        j.push_back(obj);
    }
}

void to_json(json& j, const BoneSocket& p) {
    j = json{{"model_name", p.model->getName()}};
    switch (p.type) {
        case AnchorType::Bone: {
            j["bone_name"] = p.anchorName;
            break;
        }
        case AnchorType::Mesh: {
            j["mesh_name"] = p.anchorName;
            break;
        }
        case AnchorType::Model:
        default:
            break;
    }

    const auto& t = p.positionOffset;
    const auto& s = p.scaleOffset;
    const auto& r = glm::eulerAngles(p.rotationOffset);
    j["offset"] = {
        {"translate", json::array({t.x, t.y, t.z})},
        {"scale", json::array({s.x, s.y, s.z})},
        {"rotate", json::array({r.x, r.y, r.z})},
    };
}

std::string coordinateSystemToString(const CoordinateSystem& cs) {
    switch (cs) {
        case Z_UP:
            return "z";
        case Y_UP:
            return "y";
    }
}

void to_json(json& j, const BaseModel& m) {
    const auto& t = m.mTransform.mPosition;
    const auto& s = m.mTransform.mScale;
    const auto& r = glm::eulerAngles(m.mTransform.mOrientation);
    // std::string default_clip = m.mDefaultAction != nullptr ? m.mDefaultAction->name : "";
    std::string default_clip = "";

    j = json{
        {"id", m.mName},
        {"name", m.mName},
        {"path", m.mPath},
        {"enabled", true},
        {"animated", m.mTransform.mObjectInfo.isAnimated == 1},
        {"cs", coordinateSystemToString(m.getCoordinateSystem())},
        {"translate", json::array({t.x, t.y, t.z})},
        {"scale", json::array({s.x, s.y, s.z})},
        {"rotate", json::array({glm::degrees(r.x), glm::degrees(r.y), glm::degrees(r.z)})},
        {"childrens", json::array({})},
        {"default_clip", default_clip},
    };
    if (m.mSocket == nullptr) {
        j["socket"] = nullptr;
    } else {
        j["socket"] = *m.mSocket;
    }

    if (m.mPhysicComponent == nullptr) {
        j["physics"] = nullptr;
    } else {
        j["physics"] = *m.mPhysicComponent;
    }

    if (m.instance == nullptr) {
        j["instance"] = nullptr;
    } else {
        j["instance"] = *m.instance;
    }
    // std::cout << "Model: " << j << std::endl;
}

TransformProperties calculateChildTransform(BaseModel* parent, BaseModel* child) {
    auto new_local_transform = glm::inverse(parent->getGlobalTransform()) * child->mTransform.mTransformMatrix;
    return decomposeTransformation(new_local_transform);
}
void World::exportScene() {
    json objects = json::array();
    for (const auto& model : rootContainer) {
        json j = *model;
        objects.push_back(j);
    }

    json colliders = json::array();
    for (const auto& collider : physics::PhysicSystem::mColliders) {
        json j;
        to_json(j, collider);
        colliders.push_back(j);
    }

    json jlights = json::array();
    const auto& lights = app->mLightManager->getLights();
    const auto& names = app->mLightManager->getLightsNames();
    for (size_t l = 0; l < lights.size(); ++l) {
        json j;
        to_json(j, lights[l], names[l]);
        jlights.push_back(j);
    }

    json scene;
    scene["objects"] = objects;
    scene["colliders"] = colliders;
    scene["lights"] = jlights;
    scene["cameras"] = json::array();
    scene["actor"] = actor != nullptr ? actor->getName() : "";

    auto scene_dump = scene.dump(2);
    std::cout << scene_dump << std::endl;
    std::ofstream out;
    // out.open("resources/world.json", std::ios::out);
    // out.write(scene_dump.c_str(), scene_dump.size());
    out << scene_dump;
}

Model* World::makeChild(BaseModel* parent, BaseModel* child) {
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

void World::removeParent(BaseModel* child) {
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

    if (map.contains(loadedModel->mName)) {
        auto params = map.at(loadedModel->mName);
        if (params.type == 1) {
            for (auto* model : rootContainer) {
                std::cout << model->getName() << " is of type 2 " << std::endl;
                model->moveTo(params.translate);
                model->scale(params.scale);
                std::cout << glm::to_string(params.scale) << std::endl;
            }
        }
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
            BaseModel* target = nullptr;
            if (param.isValid) {
                for (auto* suspect : rootContainer) {
                    if (suspect->mName == param.name) {
                        target = suspect;
                        break;
                    }
                }
                if (target != nullptr) {
                    // std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@ " << param.anchor << " Is the attaching bone for "
                    //           << loadedModel->mName << "\n";
                    loadedModel->mSocket = new BoneSocket{/*TODO*/ static_cast<Model*>(target),
                                                          param.anchor,
                                                          param.translate,
                                                          param.scale,
                                                          param.rotate,
                                                          param.type};
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
            // std::cout << "@@@@@@@@@@1@@@@@@@@@@@@@@@@ " << param.anchor << " from " << loadedModel->mName
            //           << " Is the attaching bone for " << model->mName << "\n";
            model->mSocket =
                new BoneSocket{loadedModel, param.anchor, param.translate, param.scale, param.rotate, param.type};
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

ObjectLoaderParam::ObjectLoaderParam(int type, std::string name, std::string path, bool animated, CoordinateSystem cs,
                                     Vec translate, Vec scale, Vec rotate, std::vector<std::string> childrens,
                                     std::string defaultClip, SocketParams socketParam)
    : socketParam(socketParam),
      type(type),
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
            mModel->mPath = param.path;
            mModel->load(param.name, app, param.path, app->getObjectBindGroupLayout())
                .moveTo(param.translate)
                .scale(param.scale)
                .rotate(param.rotate, 0.0);
            mModel->uploadToGPU(app);

            mModel->mTransform.mObjectInfo.isAnimated = param.animated;
            if (param.animated) {
                mModel->mSkiningTransformationBuffer.setLabel("default skining data transform for human")
                    .setSize(100 * sizeof(glm::mat4))
                    .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
                    .create(&app->getRendererResource());

                std::vector<glm::mat4> bones;
                for (int i = 0; i < 100; i++) {
                    bones.emplace_back(glm::mat4{1.0});
                }
                mModel->mSkiningTransformationBuffer.queueWrite(0, bones.data(), sizeof(glm::mat4) * bones.size());
            }
            // if mesh in node animated
            mModel->mGlobalMeshTransformationBuffer.setLabel("global mesh transformations buffer")
                .setSize(20 * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(&app->getRendererResource());

            auto& databuffer = mModel->mGlobalMeshTransformationData;
            mModel->mGlobalMeshTransformationBuffer.queueWrite(0, databuffer.data(),
                                                               sizeof(glm::mat4) * databuffer.size());

            // If model is instanced
            if (param.instanceTransformations.size() > 0) {
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> scales;
                std::vector<glm::vec3> rotations;
                for (const auto& instance : param.instanceTransformations) {
                    positions.emplace_back(instance.position);
                    scales.emplace_back(instance.scale);
                    rotations.emplace_back(instance.rotation);
                }

                auto* ins = new Instance{positions, rotations, scales, glm::vec4{mModel->min, 1.0f},
                                         glm::vec4{mModel->max, 1.0f}};
                ins->parent = mModel;
                ins->mManager = app->mInstanceManager;
                ins->mPositions = positions;
                ins->mScale = scales;

                mModel->setInstanced(ins);

                mModel->instance->mOffsetID = app->mInstanceManager->getNewId();
                mModel->mTransform.mObjectInfo.instanceOffsetId = mModel->instance->mOffsetID;

                ins->mManager->getInstancingBuffer().queueWrite(
                    (InstanceManager::MAX_INSTANCE_COUNT * ins->mOffsetID) * sizeof(InstanceData),
                    ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size()));

                mModel->mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + mModel->getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                              WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(&app->getRendererResource());
            }

            mModel->createSomeBinding(app, app->getDefaultTextureBindingData());

            if (mModel->mTransform.mObjectInfo.isAnimated) {
                for (auto [name, action] : mModel->getAnimation()->actions) {
                    action->loop = true;
                }
                mModel->getAnimation()->playAction(param.defaultClip, true);
                // mModel->mDefaultAction = mModel->anim->activeAction;
                mModel->setDefaultAction(mModel->getAnimation()->getActiveAction());
            }
        }

        Model* getModel() override { return mModel; }
        void onLoad(Application* app, void* params) override {
            (void)params;
            (void)app;
        };
};

void parseColliders(AddColliderFunction fn, const json& colliders) {
    for (auto& cld : colliders) {
        bool active = cld["active"].get<bool>();
        std::string type = cld["type"].get<std::string>();
        std::string name = cld["name"].get<std::string>();
        std::string shape = cld["shape"].get<std::string>();
        std::array<float, 3> center = cld["center"].get<std::array<float, 3>>();
        std::array<float, 3> h_extent = cld["half_extent"].get<std::array<float, 3>>();
        if (active) {
            fn(name, toGlm(center), toGlm(h_extent), type == "dynamic");
        }
    }
}

void parseLights(LightManager* manager, const json& lights) {
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
        if (type == "point") {
            std::cout << "One point light" << color[0] << " " << color[1] << " " << color[2] << std::endl;
            float constant = light_obj["constant"].get<float>();
            float linear = light_obj["linear"].get<float>();
            float quadratic = light_obj["quadratic"].get<float>();
            auto glm_color = glm::vec4{toGlm(color), 1.0};
            manager->createPointLight(glm::vec4{toGlm(position), 1.0}, glm_color, glm_color, glm_color, constant,
                                      linear, quadratic, name.c_str());
        }
    }
}

std::optional<SocketParams> parseSocketParam(const json& params) {
    if (params.is_null()) {
        return std::nullopt;
    }
    std::string name = params["model_name"].get<std::string>();
    std::string anchor = "";  // anchor to model itself
    AnchorType type = AnchorType::Model;
    if (params.contains("bone_name")) {
        anchor = params["bone_name"].get<std::string>();
        type = AnchorType::Bone;
    } else if (params.contains("mesh_name")) {
        anchor = params["mesh_name"].get<std::string>();
        type = AnchorType::Mesh;
    }

    glm::vec3 translate = toGlm(params["offset"]["translate"].get<std::array<float, 3>>());
    glm::vec3 scale = toGlm(params["offset"]["scale"].get<std::array<float, 3>>());
    glm::vec3 rotate = toGlm(params["offset"]["rotate"].get<std::array<float, 3>>());
    return SocketParams{name, anchor, translate, scale, glm::quat(rotate), true, type};
}

std::vector<Transformation> parseinstanceinfo(const json& params) {
    std::vector<Transformation> res;

    for (const auto& instance : params) {
        glm::vec3 translate = toGlm(instance["position"].get<std::array<float, 3>>());
        glm::vec3 scale = toGlm(instance["scale"].get<std::array<float, 3>>());
        glm::vec3 rotate = toGlm(instance["rotation"].get<std::array<float, 3>>());
        res.push_back({translate, scale, rotate});
    }

    return res;
}

std::optional<PhysicsParams> parsePhysics(const json& params) {
    if (params.is_null()) {
        return std::nullopt;
    }
    std::string type = params["type"].get<std::string>();

    return PhysicsParams{type};
}

void World::loadModel(const ObjectLoaderParam& param) {
    ModelRegistry::instance().registerModel(param.name, [param](Application* app) -> LoadModelResult {
        BaseModelLoader model = BaseModelLoader{app, param};
        model.onLoad(app, nullptr);

        if (param.isPhysicEnabled) {
            glm::vec3 euler_radians = glm::radians(param.rotate);
            glm::quat qu = glm::normalize(glm::quat(euler_radians));
            qu.z *= -1;
            qu = glm::normalize(qu);
            model.getModel()->rotate(qu);
            // CRITICAL to sync the physical world with the renderer world

            auto [min, max] = model.getModel()->getPhysicsAABB();
            auto center = (min + max) * 0.5f;
            auto half_extent = (max - min) * 0.5f;
            model.getModel()->mPhysicComponent = new PhysicsComponent{
                physics::createAndAddBody(half_extent, center, qu, param.physicsParams.type == "dynamic" ? true : false,
                                          0.5, 0.0f, 0.0f, 1.f, false, model.getModel())};
        }

        return {model.getModel(), Visibility_User};
    });
}

void World::loadWorld() {
    std::ifstream world_file(RESOURCE_DIR "/world.json");

    json j;
    json res = j.parse(world_file);

    actorName = res["actor"].get<std::string>();

    std::cout << res << std::endl;
    for (const auto& object : res["objects"]) {
        std::cout << "Object " << object["name"] << " at " << object["path"] << "\n";

        int type = object["type"].get<int>();
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

        std::vector<Transformation> instance_info;
        if (object.contains("instance")) {
            instance_info = parseinstanceinfo(object["instance"]);
        } else {
        }

        ObjectLoaderParam param{type,  name,   path,   is_animated,  cs,          translate,
                                scale, rotate, childs, default_clip, socket_param};

        param.instanceTransformations = instance_info;

        param.isDefaultActor = actorName == name;
        if (physics_props.has_value()) {
            param.isPhysicEnabled = true;
            param.physicsParams = *physics_props;
        }

        // Add resource type and allow loading of arbitrary model
        if (param.path.starts_with("rc://")) {
            param.path.replace(0, 5, "");
            std::cout << "path read is " << param.path << std::endl;
            param.path = RESOURCE_DIR + std::string{"/"} + param.path;
        }
        // param.path = RESOURCE_DIR + std::string{"/"} + param.path;
        if (type == 0) {
            loadModel(param);
        }
        map.emplace(name, param);
    }

    auto* light_manager = app->mLightManager;
    parseLights(light_manager, res["lights"]);

    auto f = std::bind(physics::PhysicSystem::createCollider, app, std::placeholders::_1, std::placeholders::_2,
                       std::placeholders::_3, std::placeholders::_4, false, nullptr);
    parseColliders(f, res["colliders"]);
    app->mLightManager->uploadToGpu(app, app->mLightBuffer.getBuffer());
}
