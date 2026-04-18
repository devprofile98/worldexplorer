
#include "world.h"

#include <array>
#include <fstream>
#include <functional>
#include <glm/fwd.hpp>
#include <ios>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
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
#include "shapes.h"
#include "texture.h"
#include "utils.h"
#include "webgpu/webgpu.h"

using json = nlohmann::json;
using AddColliderFunction = std::function<uint32_t(const std::string&, const glm::vec3&, const glm::vec3&, MotionType)>;

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
        obj["rotation"] = json::array({ins.mRotation[i].x, ins.mRotation[i].y, ins.mRotation[i].z});
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

void to_json(json& j, const Material& mat) {
    j = json{{"name", mat.mName}};

    if (mat.mNormalMap && mat.mNormalMap->isValid()) {
        j["normal_map"] = mat.mNormalMap->getPath();
    } else {
        j["normal_map"] = nullptr;
    }
    if (mat.mDiffuseMap && mat.mDiffuseMap->isValid()) {
        j["diffuse_map"] = mat.mDiffuseMap->getPath();
    } else {
        j["diffuse_map"] = nullptr;
    }
    if (mat.mSpecularMap && mat.mSpecularMap->isValid()) {
        j["specular_map"] = mat.mSpecularMap->getPath();
    } else {
        j["specular_map"] = nullptr;
    }
}
json to_json(const BaseModel* model) {
    json j = json::array();

    for (auto& [id, mat] : model->mFlattenMeshes) {
        if (mat.mTextureMaterial != nullptr) {
            json o;
            o[mat.mName] = mat.mTextureMaterial->mName;
            j.push_back(o);
        }
    }
    return j;
}

json to_json(const std::unordered_map<int, Mesh>& meshes) {
    json j = json::array();

    for (auto& [id, mesh] : meshes) {
        json o;
        json k;
        o["uv"] = json::array({mesh.mMaterial.uvMultiplier.x, mesh.mMaterial.uvMultiplier.y});
        k[mesh.mName] = o;
        j.push_back(k);
    }
    std::cout << " +++++++++++++++++++++++++++++++++++++++\n";
    std::cout << j << std::endl;
    std::cout << " +++++++++++++++++++++++++++++++++++++++\n";
    return j;
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
    const auto& r = glm::eulerAngles(glm::normalize(m.mTransform.mOrientation));

    std::string default_clip = "";

    j = json{
        {"type", m.getType()},
        {"id", m.mName},
        {"name", m.mName},
        {"path", m.mPath},
        {"visible", m.getVisible()},
        {"enabled", true},
        {"animated", m.mTransform.mObjectInfo.isAnimated == 1},
        {"cs", coordinateSystemToString(m.getCoordinateSystem())},
        {"translate", json::array({t.x, t.y, t.z})},
        {"scale", json::array({s.x, s.y, s.z})},
        {"rotate", json::array({glm::degrees(r.x), glm::degrees(r.y), glm::degrees(r.z)})},
        {"childrens", json::array({})},
        {"default_clip", default_clip},
        {"materials", to_json(&m)},
        {"material_props", to_json(m.mFlattenMeshes)},
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

    json materials = json::array();
    for (auto& [name, mat] : app->mMaterialRegistery->list()) {
        json j;
        to_json(j, *mat);
        materials.push_back(j);
    }

    json scene;
    scene["objects"] = objects;
    scene["colliders"] = colliders;
    scene["lights"] = jlights;
    scene["cameras"] = json::array();
    scene["actor"] = actor != nullptr ? actor->getName() : "";
    scene["materials"] = materials;

    auto scene_dump = scene.dump(2);
    std::cout << scene_dump << std::endl;
    std::ofstream out;
    out.open(app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / mSceneFilePath, std::ios::trunc);
    out.write(scene_dump.c_str(), scene_dump.size());
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

World::World(Application* app, const std::string& sceneFilePath) : app(app), mSceneFilePath(sceneFilePath) {
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

    // if (loadedModel->getName() == "tower" || loadedModel->getName() == "terrain" || loadedModel->getName() ==
    // "chair") {
    // if (loadedModel->getName() == "container2") {
    //     auto& mesh = loadedModel->mFlattenMeshes.begin()->second;
    //     loadedModel->getGlobalTransform();
    //     std::cout << glm::to_string(loadedModel->mTransform.getPosition()) << "is the position\n";
    //     std::cout << glm::to_string(loadedModel->mTransform.getEulerRotation()) << "is the rotation\n";
    //     std::cout << glm::to_string(loadedModel->mTransform.getScale()) << "is the scale\n";
    //     std::cout << glm::to_string(loadedModel->mTransform.mTransformMatrix) << "is the transformation\n";
    //     auto* bdy =
    //         physics::createPhysicFromShape(mesh.mIndexData, mesh.mVertexData,
    //         loadedModel->mTransform.mTransformMatrix);
    //
    //     if (bdy != nullptr) {
    //         loadedModel->mPhysicComponent = new PhysicsComponent{bdy->GetID()};
    //         loadedModel->mPhysicComponent->colliderType = ColliderType::TriList;
    //         std::cout << "Success to create physics from mesh " << loadedModel->getName() << "!\n";
    //     } else {
    //         std::cout << "Failed to create physics from mesh!\n";
    //     }
    //     if (loadedModel->getName() == "container") {
    //         auto mesh_wireframe_lines = generateFromMesh(mesh.mIndexData, mesh.mVertexData);
    //         loadedModel->mPhysicComponent->mDebugLines = app->mLineEngine->create(
    //             mesh_wireframe_lines, loadedModel->mTransform.mTransformMatrix, glm::vec3{1.0, 0.0, 0.0});
    //         loadedModel->mPhysicComponent->mDebugLines->updateVisibility(false);
    //     }
    // }

    if (map.contains(loadedModel->mName)) {
        auto params = map.at(loadedModel->mName);

        if (!params.materialList.empty()) {
            for (auto [name, mat_name] : params.materialList) {
                app->mMaterialRegistery->applyMaterialTo(app, loadedModel, name, mat_name);
            }
        }

        if (!params.matPropMap.empty()) {
            for (auto& [id, mesh] : loadedModel->mFlattenMeshes) {
                auto prop = params.matPropMap.find(mesh.mName);
                if (prop != params.matPropMap.end()) {
                    mesh.mMaterial.uvMultiplier = glm::vec3{prop->second.uv[0], prop->second.uv[1], 1.0};
                    loadedModel->mTransform.mDirty = true;
                    std::cout << loadedModel->mName << " " << prop->first << " ::::: Has " << params.matPropMap.size()
                              << " " << prop->second.uv[0] << " " << prop->second.uv[1] << " Materials\n";
                } else {
                }
            }
        }

        if (params.type == ModelTypes::CODE) {
            for (auto* model : rootContainer) {
                if (model->mName == params.name) {
                    model->setVisible(params.isVisible);
                    std::cout << model->getName() << " is of type 2 " << std::endl;
                    model->moveTo(params.translate);
                    model->scale(params.scale);
                    std::cout << glm::to_string(params.scale) << std::endl;
                }
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
            model->mSocket =
                new BoneSocket{loadedModel, param.anchor, param.translate, param.scale, param.rotate, param.type};
        }
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
        if (model->mInputHandler != nullptr) {
            model->mInputHandler->handleKey(model, event, delta);
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
        if (model->mInputHandler != nullptr) {
            model->mInputHandler->handleMouseMove(model, event);
        }
    }
}

void World::onMouseClick(MouseEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    for (const auto& model : models) {
        if (model->mInputHandler != nullptr) {
            model->mInputHandler->handleMouseClick(model, event);
        }
    }
}

void World::onMouseScroll(MouseEvent event) {
    auto& models = rootContainer;
    if (actor == nullptr) {
        return;
    }

    for (const auto& model : models) {
        if (model->mInputHandler != nullptr) {
            model->mInputHandler->handleMouseScroll(model, event);
        }
    }
}

glm::vec3 toGlm(std::array<float, 3> arr) { return glm::vec3{arr[0], arr[1], arr[2]}; }
glm::vec2 toGlm(std::array<float, 2> arr) { return glm::vec2{arr[0], arr[1]}; }

ObjectLoaderParam::ObjectLoaderParam(int type, std::string name, std::string path, bool animated, bool isVisible,
                                     CoordinateSystem cs, Vec translate, Vec scale, Vec rotate,
                                     std::vector<std::string> childrens, std::string defaultClip,
                                     SocketParams socketParam, MaterialList matList, MaterialPropsMap matPropMap)
    : socketParam(socketParam),
      type(type),
      name(name),
      path(path),
      animated(animated),
      isVisible(isVisible),
      cs(cs),
      childrens(childrens),
      defaultClip(defaultClip),
      materialList(matList),
      matPropMap(matPropMap) {
    this->translate = toGlm(translate);
    this->scale = toGlm(scale);
    this->rotate = toGlm(rotate);
}

struct BaseModelLoader : public IModel {
        BaseModelLoader(Application* app, ObjectLoaderParam param) {
            mModel = new Model{param.cs};
            mModel->setVisible(param.isVisible);
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
                .setSize(mModel->mFlattenMeshes.size() * sizeof(glm::mat4))
                .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst)
                .create(&app->getRendererResource());

            // auto& databuffer = mModel->mGlobalMeshTransformationData;
            // mModel->mGlobalMeshTransformationBuffer.queueWrite(0, databuffer.data(),
            //                                                    sizeof(glm::mat4) * databuffer.size());

            // If model is instanced
            if (param.instanceTransformations.size() > 0) {
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> scales;
                std::vector<glm::vec3> rotations;
                std::vector<bool> has_physics;
                for (const auto& instance : param.instanceTransformations) {
                    positions.emplace_back(instance.position);
                    scales.emplace_back(instance.scale);
                    rotations.emplace_back(instance.rotation);
                    has_physics.emplace_back(instance.hasPhysics);
                }

                auto* ins = new Instance{positions,
                                         rotations,
                                         scales,
                                         has_physics,
                                         glm::vec4{mModel->min, 1.0f},
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

        MotionType motion_type = MotionType::Static;
        if (type == "dynamic") {
            motion_type = MotionType::Dynamic;
        } else if (type == "static") {
            motion_type = MotionType::Static;
        } else if (type == "kinematic") {
            motion_type = MotionType::Kinematic;
        }

        if (active) {
            fn(name, toGlm(center), toGlm(h_extent), motion_type);
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
                                     glm::vec4{toGlm(color), 1.0}, inner_cutoff, outer_cutoff, linear, quadratic, 1.0,
                                     name.c_str());
        }
        if (type == "point") {
            std::cout << "One point light" << color[0] << " " << color[1] << " " << color[2] << std::endl;
            float constant = light_obj["constant"].get<float>();
            float linear = light_obj["linear"].get<float>();
            float quadratic = light_obj["quadratic"].get<float>();
            auto glm_color = glm::vec4{toGlm(color), 1.0};
            manager->createPointLight(glm::vec4{toGlm(position), 1.0}, glm_color, glm_color, glm_color, constant,
                                      linear, quadratic, 1.0, name.c_str());
        }
    }
}

void parseMaterial(Application* app, const json& materials) {
    for (auto& mat_obj : materials) {
        std::string name = mat_obj["name"].get<std::string>();
        json dif_map_json = mat_obj["diffuse_map"];
        json nor_map_json = mat_obj["normal_map"];
        json spec_map_json = mat_obj["specular_map"];

        std::shared_ptr<Texture> dif_tex = nullptr;
        std::shared_ptr<Texture> nor_tex = nullptr;
        std::shared_ptr<Texture> spec_tex = nullptr;

        if (!dif_map_json.is_null()) {
            std::string dif_map = mat_obj["diffuse_map"].get<std::string>();
            if (dif_map.starts_with("rc://")) {
                dif_map.replace(0, 5, "");
                dif_map = app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / dif_map;
            }
            dif_tex = Texture::asyncLoadTexture(app->mTextureRegistery, app->getRendererResource(), dif_map);
        }
        if (!nor_map_json.is_null()) {
            std::string nor_map = mat_obj["normal_map"].get<std::string>();
            if (nor_map.starts_with("rc://")) {
                nor_map.replace(0, 5, "");
                nor_map = app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / nor_map;
            }
            nor_tex = Texture::asyncLoadTexture(app->mTextureRegistery, app->getRendererResource(), nor_map);
        }
        if (!spec_map_json.is_null()) {
            std::string spec_map = mat_obj["specular_map"].get<std::string>();
            if (spec_map.starts_with("rc://")) {
                spec_map.replace(0, 5, "");
                spec_map = app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / spec_map;
            }
            spec_tex = Texture::asyncLoadTexture(app->mTextureRegistery, app->getRendererResource(), spec_map);
        }

        app->mMaterialRegistery->addToRegistery(name, std::make_shared<Material>(name, dif_tex, nor_tex, nullptr));
        app->mMaterialRegistery->applyWaiters(app, name);
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

std::vector<InstanceInfo> parseinstanceinfo(const json& params) {
    std::vector<InstanceInfo> res;

    for (const auto& instance : params) {
        glm::vec3 translate = toGlm(instance["position"].get<std::array<float, 3>>());
        glm::vec3 scale = toGlm(instance["scale"].get<std::array<float, 3>>());
        glm::vec3 rotate = toGlm(instance["rotation"].get<std::array<float, 3>>());
        bool has_physic = false;
        if (instance.contains("physics")) {
            has_physic = !instance["physics"].is_null();
        }
        res.push_back({translate, scale, rotate, has_physic});
    }

    return res;
}

MaterialPropsMap parseMeshMaterialProps(std::vector<json>& props) {
    std::unordered_map<std::string, MaterialProperties> res;
    for (json& properties : props) {
        for (auto [k, v] : properties.items()) {
            // std::cout << " LLLLLLLLLLl " << k << v << std::endl;
            auto uv = v["uv"].get<std::array<float, 2>>();
            res[k] = MaterialProperties{k, uv};
        }
    }
    return res;
}

MaterialList parseMeshMaterials(std::vector<json>& materials) {
    MaterialList res;
    for (json& mat : materials) {
        for (auto [k, v] : mat.items()) {
            res.emplace_back(std::pair{k, v});
        }
    }
    return res;
}

std::optional<PhysicsParams> parsePhysics(const json& params) {
    if (params.is_null()) {
        return std::nullopt;
    }
    std::string type = params["type"].get<std::string>();
    bool is_sensor = false;
    if (params.contains("sensor")) {
        is_sensor = params["sensor"].get<bool>();
    }
    PhysicGenMethod method = PhysicGenMethod::AABB;
    if (params.contains("method")) {
        std::string method_str = params["method"].get<std::string>();
        if (method_str == "aabb") {
            method = PhysicGenMethod::AABB;
        } else if (method_str == "mesh") {
            method = PhysicGenMethod::MESH;
        } else if (method_str == "custom") {
            method = PhysicGenMethod::CUSTOM;
        }
    }

    return PhysicsParams{type, is_sensor, method};
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
            auto physic_offset = center - model.getModel()->mTransform.getPosition();
            auto half_extent = (max - min) * 0.5f;

            MotionType motion_type = MotionType::Static;
            if (param.physicsParams.type == "dynamic") {
                motion_type = MotionType::Dynamic;
            } else if (param.physicsParams.type == "static") {
                motion_type = MotionType::Static;
            } else if (param.physicsParams.type == "kinematic") {
                motion_type = MotionType::Kinematic;
            }

            if (param.physicsParams.method == PhysicGenMethod::MESH) {
                auto* loadedModel = model.getModel();
                auto& mesh = loadedModel->mFlattenMeshes.begin()->second;
                loadedModel->getGlobalTransform();
                auto* bdy = physics::createPhysicFromShape(mesh.mIndexData, mesh.mVertexData,
                                                           loadedModel->mTransform.mTransformMatrix);

                if (bdy != nullptr) {
                    loadedModel->mPhysicComponent = new PhysicsComponent{bdy->GetID()};
                    loadedModel->mPhysicComponent->colliderType = ColliderType::TriList;
                    std::cout << "Success to create physics from mesh " << loadedModel->getName() << "!\n";
                }
                // if (loadedModel->getName() == "container") {
                auto mesh_wireframe_lines = generateFromMesh(mesh.mIndexData, mesh.mVertexData);
                loadedModel->mPhysicComponent->mDebugLines = app->mLineEngine->create(
                    mesh_wireframe_lines, loadedModel->mTransform.mTransformMatrix, glm::vec3{1.0, 0.0, 0.0});
                loadedModel->mPhysicComponent->mDebugLines->updateVisibility(false);
                // }
            } else if (param.physicsParams.method == PhysicGenMethod::AABB) {
                // if (param.name == "tree" || param.name == "zombie") {
                auto* target = model.getModel();
                target->mPhysicComponent =
                    physics::CreatePhysicsBox(physic_offset, half_extent, target->mTransform.getPosition(), target);
                // } else {
                //     JPH::BodyID id{};
                //     id = physics::createAndAddBody(half_extent, center, qu, motion_type, 0.5, 0.0f, 0.0f, 1.f,
                //                                    param.physicsParams.isSensor, model.getModel());
                //     model.getModel()->mPhysicComponent = new PhysicsComponent{id};
                // }

                target->mPhysicComponent->mDebugLines =
                    app->mLineEngine
                        ->create(generateBox(),
                                 glm::translate(glm::mat4{1.0}, center) * glm::scale(glm::mat4{1.0}, half_extent),
                                 (motion_type == MotionType::Static)
                                     ? (false ? glm::vec3{0.3, 0.3, 0.1} : glm::vec3{0.0, 1.0, 0.0})
                                     : glm::vec3{1.0, 0.209, 0.0784})
                        .updateVisibility(false);
                target->mPhysicComponent->mDebugLines.value().setScaleFatcor(half_extent);
            }

            auto* ins = model.getModel()->instance;
            if (ins != nullptr) {
                for (size_t i = 0; i < ins->getInstanceCount(); ++i) {
                    if (ins->mHasPhysic[i]) {
                        if (param.physicsParams.method == PhysicGenMethod::MESH) {
                            // std::cout << "creating instance physic for " << model.getModel()->getName() << " "
                            //           << glm::to_string(ins->mPositions[i]) << std::endl;
                            auto& mesh = model.getModel()->mFlattenMeshes.begin()->second;
                            auto* bdy = physics::createPhysicFromShape(mesh.mIndexData, mesh.mVertexData,
                                                                       ins->mInstanceBuffer[i].modelMatrix);

                            if (bdy != nullptr) {
                                ins->mPhysicsComponents[i] =
                                    std::shared_ptr<PhysicsComponent>(new PhysicsComponent{bdy->GetID()});
                                ins->mPhysicsComponents[i]->colliderType = ColliderType::TriList;

                                auto mesh_wireframe_lines = generateFromMesh(mesh.mIndexData, mesh.mVertexData);
                                ins->mPhysicsComponents[i]->mDebugLines =
                                    app->mLineEngine->create(mesh_wireframe_lines, ins->mInstanceBuffer[i].modelMatrix,
                                                             glm::vec3{1.0, 0.0, 0.0});
                                ins->mPhysicsComponents[i]->mDebugLines->updateVisibility(false);
                            }

                        } else if (param.physicsParams.method == PhysicGenMethod::AABB) {
                            ins->mPhysicsComponents[i] = std::shared_ptr<PhysicsComponent>(physics::CreatePhysicsBox(
                                physic_offset, half_extent, ins->mPositions[i], model.getModel()));

                            ins->mPhysicsComponents[i]->mDebugLines =
                                app->mLineEngine
                                    ->create(generateBox(),
                                             glm::translate(glm::mat4{1.0}, ins->mPositions[i]) *
                                                 glm::scale(glm::mat4{1.0}, half_extent),
                                             (motion_type == MotionType::Static)
                                                 ? (false ? glm::vec3{0.3, 0.3, 0.1} : glm::vec3{0.0, 1.0, 0.0})
                                                 : glm::vec3{1.0, 0.209, 0.0784})
                                    .updateVisibility(false);
                            ins->mPhysicsComponents[i]->mDebugLines.value().setScaleFatcor(half_extent);
                        }
                    }
                }
            }
        }

        return {model.getModel(), Visibility_User};
    });
}

void World::loadWorld() {
    std::ifstream world_file(app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / mSceneFilePath);

    json j;
    json res = j.parse(world_file);

    auto* light_manager = app->mLightManager;
    parseLights(light_manager, res["lights"]);
    parseMaterial(app, res["materials"]);

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
        bool is_visible = object["visible"].get<bool>();
        bool is_animated = object["animated"].get<bool>();
        std::string _cs = object["cs"].get<std::string>();
        CoordinateSystem cs = _cs == "z" ? Z_UP : Y_UP;
        std::array<float, 3> translate = object["translate"].get<std::array<float, 3>>();
        std::array<float, 3> scale = object["scale"].get<std::array<float, 3>>();
        std::array<float, 3> rotate = object["rotate"].get<std::array<float, 3>>();
        std::vector<std::string> childs = object["childrens"].get<std::vector<std::string>>();
        std::string default_clip = object["default_clip"].get<std::string>();

        MaterialList mat_list{};
        if (object.count("materials") > 0) {
            auto materials = object["materials"].get<std::vector<json>>();
            mat_list = parseMeshMaterials(materials);
        }

        MaterialPropsMap mat_map{};
        if (object.count("material_props") > 0) {
            auto props = object["material_props"].get<std::vector<json>>();
            mat_map = parseMeshMaterialProps(props);
        }

        auto physics_props = parsePhysics(object["physics"]);
        SocketParams socket_param;
        socket_param.isValid = false;

        auto socket_opt = parseSocketParam(object["socket"]);
        if (socket_opt.has_value()) {
            socket_param = socket_opt.value();
            socket_param.isValid = true;
        }

        std::vector<InstanceInfo> instance_info;
        if (object.contains("instance")) {
            instance_info = parseinstanceinfo(object["instance"]);
        } else {
        }

        ObjectLoaderParam param{type,  name,   path,   is_animated,  is_visible,   cs,       translate,
                                scale, rotate, childs, default_clip, socket_param, mat_list, mat_map};

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
            param.path = app->getBinaryPathAbsolute() / ".." / RESOURCE_DIR / param.path;
        }
        if (type == ModelTypes::FS) {
            loadModel(param);
        }
        map.emplace(name, param);
    }

    auto f = std::bind(physics::PhysicSystem::createCollider, app, std::placeholders::_1, std::placeholders::_2,
                       std::placeholders::_3, std::placeholders::_4, false, nullptr);
    parseColliders(f, res["colliders"]);
    app->mLightManager->uploadToGpu(app, app->mLightBuffer.getBuffer());
}
