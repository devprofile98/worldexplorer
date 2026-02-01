#include "model.h"

#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "animation.h"
#include "assimp/matrix4x4.h"
#include "assimp/mesh.h"
#include "assimp/quaternion.h"
#include "assimp/vector3.h"
#include "glm/ext/matrix_common.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/matrix.hpp"
#include "instance.h"
#include "mesh.h"
#include "physics.h"
#include "texture.h"
// #include "tracy/Tracy.hpp"
#include "profiling.h"
#include "utils.h"
#include "wgpu_utils.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "../tinyobjloader/tiny_obj_loader.h"
#include "application.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "rendererResource.h"
#include "webgpu/webgpu.h"

extern bool flip_x;
extern bool flip_y;
extern bool flip_z;

Drawable::Drawable() {}

void Drawable::configure(Application* app) {
    mUniformBuffer.setLabel("Uniform buffer for object info")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(ObjectInfo))
        .setMappedAtCraetion()
        .create(&app->getRendererResource());
}

Transformable& Transformable::moveTo(const glm::vec3&) { return *this; }

void Drawable::draw(Application* app, WGPURenderPassEncoder encoder) {
    (void)app;
    (void)encoder;
}

void Drawable::drawHirarchy(Application* app, WGPURenderPassEncoder encoder) {
    (void)app;
    (void)encoder;
}

Node* Node::buildNodeTree(aiNode* ainode, Node* parent, std::unordered_map<std::string, Node*>& nodemap) {
    Node* node = new Node{};

    node->mName = ainode->mName.C_Str();
    node->mLocalTransform = glm::mat4{1.0};  // AiToGlm(ainode->mTransformation);
    node->mParent = parent;

    // store mesh indices attached to this node
    for (size_t i = 0; i < ainode->mNumMeshes; ++i) {
        node->mMeshIndices.push_back(ainode->mMeshes[i]);
    }
    nodemap[node->mName] = node;

    // add child nodes to this newly created node for its children recursively
    for (size_t i = 0; i < ainode->mNumChildren; ++i) {
        node->mChildrens.emplace_back(buildNodeTree(ainode->mChildren[i], node, nodemap));
    }
    return node;
}

void populateGlobalMeshesTransformationBuffer(Application* app, Node* node, std::vector<glm::mat4>& buffer,
                                              std::unordered_map<int, Mesh>& meshes,
                                              const std::unordered_map<std::string, glm::mat4>& animation) {
    if (node == nullptr) {
        return;
    }

    for (auto idx : node->mMeshIndices) {
        glm::mat4 anim{1.0};
        if (animation.contains(node->mName)) {
            anim = animation.at(node->mName);
        }
        buffer[idx] = node->getGlobalTransform() * anim;
    }
    for (auto* child : node->mChildrens) {
        populateGlobalMeshesTransformationBuffer(app, child, buffer, meshes, animation);
    }
}

glm::mat4 Node::getGlobalTransform() const {
    if (mParent) {
        return mParent->getGlobalTransform() * mLocalTransform;
    }
    return mLocalTransform;
}

Buffer& Drawable::getUniformBuffer() { return mUniformBuffer; }

Model::Model(CoordinateSystem cs) : BaseModel(), mCoordinateSystem(cs) {
    mTransform.mScaleMatrix = glm::scale(mTransform.mScaleMatrix, mTransform.mScale);
    mTransform.mTransformMatrix = mTransform.mRotationMatrix * mTransform.mTranslationMatrix * mTransform.mScaleMatrix;
    mTransform.mObjectInfo.transformation = mTransform.mTransformMatrix;
    mTransform.mObjectInfo.isAnimated = 0;
    mTransform.mDirty = true;
    anim = new Animation{};
}

glm::mat3x3 computeTBN(VertexAttributes* vertex, const glm::vec3& expectedN) {
    const auto& pos1 = vertex[0].position;
    const auto& pos2 = vertex[1].position;
    const auto& pos3 = vertex[2].position;

    const auto& uv1 = vertex[0].uv;
    const auto& uv2 = vertex[1].uv;
    const auto& uv3 = vertex[2].uv;

    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent{};
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    glm::vec3 bitangent{};
    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    glm::vec3 cnormal = glm::cross(tangent, bitangent);
    /*const auto& expectedN = mMeshes[materialId].mVertexData[i].normal;*/

    if (glm::dot(expectedN, cnormal) < 0.0) {
        tangent = -tangent;
        bitangent = -bitangent;
        cnormal = -cnormal;
    }

    /*cnormal = expectedN;*/
    /*tangent = normalize(tangent - dot(tangent, cnormal) * cnormal);*/
    /*bitangent = cross(cnormal, tangent);*/

    return glm::mat3x3(tangent, bitangent, cnormal);
}

void Model::updateAnimation(float dt) {
    auto* action = anim->getActiveAction();
    if (action == nullptr) {
        return;
    }
    if (mTransform.mObjectInfo.isAnimated) {
        action->mAnimationSecond += dt * 1000.0;
        auto duration_ms = action->mAnimationDuration * 1000.0;

        if (action->loop) {
            // wrap aroun when looping
            action->mAnimationSecond = std::fmod(action->mAnimationSecond, duration_ms);
        } else {
            // Clamp to end
            if (action->mAnimationSecond >= duration_ms) {
                action->mAnimationSecond = duration_ms;
            }
        }
        anim->update(mScene->mRootNode);

    } else {
        return;
    }

    if (action->hasSkining) {
        if (action != nullptr) {
            for (const auto& [boneName, _] : action->Bonemap) {
                const auto& global = action->calculatedTransform[boneName];
                const auto& offset = action->Bonemap[boneName]->offsetMatrix;

                auto final = global * offset;

                // if (std::isfinite(final[0][0])) {
                anim->mFinalTransformations[action->Bonemap[boneName]->id] = final;
                // }
            }
        } else {
            for (auto& transformation : anim->mFinalTransformations) {
                transformation = glm::mat4{1.0};
            }
        }
    } else {
        bool shoulddo = false;
        std::unordered_map<std::string, glm::mat4> anims;
        for (const auto& [node_name, node] : mNodeNameMap) {
            shoulddo = true;
            const auto& animation_transform = action->localTransformation.at(node_name);
            anims[node_name] = node->mLocalTransform * animation_transform;
        }
        if (shoulddo) {
            populateGlobalMeshesTransformationBuffer(mApp, mRootNode, mGlobalMeshTransformationData, mFlattenMeshes,
                                                     anims);
            auto& databuffer = mGlobalMeshTransformationData;
            wgpuQueueWriteBuffer(mApp->getRendererResource().queue, mGlobalMeshTransformationBuffer.getBuffer(), 0,
                                 databuffer.data(), sizeof(glm::mat4) * databuffer.size());
            mTransform.mDirty = true;
        }
    }
}

CoordinateSystem Model::getCoordinateSystem() const { return mCoordinateSystem; }

glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    // Assimp is row-major, GLM is column-major, so transpose
    to[0][0] = from.a1;
    to[0][1] = from.b1;
    to[0][2] = from.c1;
    to[0][3] = from.d1;
    to[1][0] = from.a2;
    to[1][1] = from.b2;
    to[1][2] = from.c2;
    to[1][3] = from.d2;
    to[2][0] = from.a3;
    to[2][1] = from.b3;
    to[2][2] = from.c3;
    to[2][3] = from.d3;
    to[3][0] = from.a4;
    to[3][1] = from.b4;
    to[3][2] = from.c4;
    to[3][3] = from.d4;
    return to;
}
// Modified processNode to pass parent transformation
void Model::processNode(Application* app, aiNode* node, const aiScene* scene, const glm::mat4& parentTransform) {
    // Compute global transformation for this node
    glm::mat4 nodeTransform = glm::mat4{1.0};  // AiToGlm(node->mTransformation);
    // aiMatrix4x4ToGlm(node->mTransformation);
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Process meshes in this node
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        unsigned int mid = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[mid];
        processMesh(app, mesh, scene, mid, globalTransform);
    }

    // Recursively process child nodes
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        processNode(app, node->mChildren[i], scene, nodeTransform);
    }
}

void loaderCallback(TextureLoader::LoadRequest* request) {
    // Heavy work: read file + generate mipmaps on CPU
    request->baseTexture->writeBaseTexture(request->path);
    request->baseTexture->uploadToGPU(request->queue);
    request->promise.set_value(request->baseTexture);
}

void Model::processMesh(Application* app, aiMesh* mesh, const aiScene* scene, unsigned int meshId,
                        const glm::mat4& globalTransform) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    std::map<size_t, std::vector<std::pair<size_t, float>>> vertex_weights;
    mFlattenMeshes[mMeshNumber] = {};
    size_t index_offset = mFlattenMeshes[mMeshNumber].mVertexData.size();

    if (mesh->HasBones() && !anim->actions.empty()) {
        Action* action = anim->actions.begin()->second;
        for (uint32_t i = 0; i < mesh->mNumBones; ++i) {
            const auto* bone = mesh->mBones[i];
            for (size_t j = 0; j < bone->mNumWeights; ++j) {
                const char* name = bone->mName.C_Str();

                auto& vid = bone->mWeights[j].mVertexId;
                if (vertex_weights.count(vid) == 0) {
                    vertex_weights[vid] = {};
                }
                if (action && action->Bonemap.contains(name)) {
                    vertex_weights[vid].emplace_back(action->Bonemap[name]->id, bone->mWeights[j].mWeight);
                }
            }
        }
    }

    // Compute normal transformation (inverse transpose for normals)
    glm::mat4 normalTransform = glm::transpose(glm::inverse(globalTransform));
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(globalTransform)));

    glm::mat4 yUpToZUp = glm::mat4(1.0f);
    yUpToZUp = glm::rotate(yUpToZUp, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // 90° around X
    glm::mat3 rot3 = glm::mat3(yUpToZUp);
    //
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        VertexAttributes vertex;

        // Transform vertex position
        glm::vec4 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        pos = globalTransform * pos;
        glm::vec3 vector(pos.x, pos.y, pos.z);
        pos = yUpToZUp * pos;
        vertex.position = glm::vec3{std::move(pos)};

        glm::mat4 swap{1.0};  // identity matrix for default case
        if (mCoordinateSystem == CoordinateSystem::Z_UP) {
            swap = {{1, 0, 0, 0}, {0, 0, -1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}};
        }
        auto temp = swap * glm::vec4{vertex.position, 1.0};
        vertex.position = glm::vec3{temp};

        size_t bid_cnt = 0;
        if (mesh->HasBones()) {
            for (const auto& [bid, bwg] : vertex_weights[i]) {
                vertex.boneIds[bid_cnt % 4] = bid;
                vertex.weights[bid_cnt % 4] = bwg;
                ++bid_cnt;
            }
        }

        // Update bounding box
        min.x = std::min(min.x, vertex.position.x);
        min.y = std::min(min.y, vertex.position.y);
        min.z = std::min(min.z, vertex.position.z);
        max.x = std::max(max.x, vertex.position.x);
        max.y = std::max(max.y, vertex.position.y);
        max.z = std::max(max.z, vertex.position.z);

        if (mesh->HasNormals()) {
            // Transform normal
            glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            normal = normalMatrix * normal;
            vertex.normal = rot3 * normal;
            auto temp = swap * glm::vec4{vertex.normal, 0.0};
            vertex.normal = glm::vec3{std::move(temp)};
        }

        aiColor4D baseColor(1.0f, 0.0f, 1.0f, 1.0f);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        vertex.color = glm::vec3(baseColor.r, baseColor.g, baseColor.b);

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.uv = vec;
            if (mesh->mTangents && mesh->mBitangents) {
                // Transform tangent
                glm::vec3 tangent(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                tangent = normalMatrix * tangent;
                vertex.tangent = rot3 * tangent;
                vertex.tangent = glm::vec3{swap * glm::vec4{vertex.tangent, 0.0}};
                vertex.tangent =
                    glm::normalize(vertex.tangent - vertex.normal * glm::dot(vertex.normal, vertex.tangent));

                // Transform bitangent
                glm::vec3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                bitangent = normalMatrix * bitangent;
                vertex.biTangent = rot3 * bitangent;
                vertex.biTangent = glm::vec3{swap * glm::vec4{vertex.biTangent, 0.0}};
                vertex.biTangent = glm::cross(vertex.normal, vertex.tangent);
            }
        } else {
            vertex.uv = glm::vec2{0.0f, 0.0f};
        }

        mFlattenMeshes[mMeshNumber].mVertexData.push_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            mFlattenMeshes[mMeshNumber].mIndexData.push_back((uint32_t)face.mIndices[j] + index_offset);
        }
    }

    auto& render_resource = app->getRendererResource();
    auto& mmesh = mFlattenMeshes[mMeshNumber];

    auto load_texture = [&](aiTextureType type, MaterialProps flag, std::shared_ptr<Texture>* target) {
        for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
            mmesh.mMaterial.setFlag(flag, true);
            mmesh.mMaterialName = material->GetName().C_Str();

            aiString str;
            material->GetTexture(type, i, &str);
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += str.C_Str();

            std::cout << "Loading " << texture_path << " As " << getName() << std::endl;

            size_t pos = 0;
            while ((pos = texture_path.find("%20", pos)) != std::string::npos) {
                texture_path.replace(pos, 3, " ");  // replace 3 chars ("%20") with 1 space
                pos += 1;                           // move past the inserted space
            }

            auto cached = mApp->mTextureRegistery->get(texture_path);
            if (cached != nullptr) {
                *target = cached;
                mmesh.isTransparent = (*target)->isTransparent();
                continue;
            }

            auto texture =
                std::make_shared<Texture>(mApp->getRendererResource().device, texture_path);  // reads file here

            // queue async load
            auto future = mApp->mTextureRegistery->mLoader.loadAsync(texture_path, render_resource.queue, texture,
                                                                     loaderCallback);

            mApp->mTextureRegistery->addToRegistery(texture_path, texture);

            *target = texture;
            mmesh.isTransparent = texture->isTransparent();

            if (texture->createView() == nullptr) {
                std::cout << std::format("Failed to create view for {} at {}\n", mName, texture_path);
            } else {
                std::cout << std::format("Successfully loaded texture {} at {}\n", mName, texture_path);
            }
            // mmesh.isTransparent = (*target)->isTransparent();
            // }).detach();
        }
    };

    if (mmesh.mTexture == nullptr) {
        load_texture(aiTextureType_DIFFUSE, MaterialProps::HasDiffuseMap, &mmesh.mTexture);
    }
    if (mmesh.mSpecularTexture == nullptr) {
        load_texture(aiTextureType_DIFFUSE_ROUGHNESS, MaterialProps::HasRoughnessMap, &mmesh.mSpecularTexture);
    }
    if (mmesh.mNormalMapTexture == nullptr) {
        load_texture(aiTextureType_HEIGHT, MaterialProps::HasNormalMap, &mmesh.mNormalMapTexture);
    }
    if (mmesh.mNormalMapTexture == nullptr) {
        load_texture(aiTextureType_NORMALS, MaterialProps::HasNormalMap, &mmesh.mNormalMapTexture);
    }
    mFlattenMeshes[mMeshNumber].meshId = meshId;
    mMeshNumber++;
}

Model& Model::load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout) {
    // load model from disk
    (void)layout;
    mName = name;
    mApp = app;

    Drawable::configure(app);

    std::string warn;
    std::string err;

    const aiScene* const scene =
        mImport.ReadFile(path.string().c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
                                                    aiProcess_JoinIdenticalVertices);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << std::format("Assimp - Error while loading model {} : {}\n", (const char*)path.c_str(),
                                 mImport.GetErrorString());
    } else {
        std::cout << std::format("Assimp - Succesfully loaded model {}\n", (const char*)path.c_str());
    }
    mScene = scene;

    // if (getName() != "house") {
    mTransform.mObjectInfo.isAnimated = anim->initAnimation(mScene, getName());
    updateAnimation(0);
    // }

    mRootNode = Node::buildNodeTree(scene->mRootNode, nullptr, mNodeNameMap);

    if (mRootNode != nullptr) {
        std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$ Node hirarchy created for" << getName() << " Succesfuly\n";
        std::cout << glm::to_string(AiToGlm(mScene->mRootNode->mTransformation)) << "\n";
    }

    processNode(app, scene->mRootNode, scene, glm::mat4{1.0});

    mGlobalMeshTransformationData.reserve(mFlattenMeshes.size());
    mGlobalMeshTransformationData.resize(mFlattenMeshes.size());
    populateGlobalMeshesTransformationBuffer(app, mRootNode, mGlobalMeshTransformationData, mFlattenMeshes, {});

    return *this;
}

void BaseModel::setInstanced(Instance* instance) { this->instance = instance; }

void BaseModel::addChildren(BaseModel* child) {
    auto new_local_transform = glm::inverse(getGlobalTransform()) * child->mTransform.mTransformMatrix;

    glm::vec3 position = glm::vec3(new_local_transform[3]);
    // child->mTransform.mPosition = position;
    child->moveTo(position);

    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(new_local_transform[0]));
    scale.y = glm::length(glm::vec3(new_local_transform[1]));
    scale.z = glm::length(glm::vec3(new_local_transform[2]));
    // child->mTransform.mScale = scale;
    child->mParent = this;
    child->scale(scale);
    child->mTransform.getLocalTransform();

    mChildrens.push_back(child);
}

Model& Model::setFoliage() {
    mTransform.mDirty = true;
    // mTransform.mObjectInfo.isFoliage = 1;
    return *this;
}

Model& Model::useTexture(bool use) {
    mTransform.mDirty = true;
    // mTransform.mObjectInfo.useTexture = use ? 1 : 0;
    return *this;
}

BaseModel& BaseModel::moveBy(const glm::vec3& translationVec) {
    // justify position by the factor `m`
    mTransform.mDirty = true;
    mTransform.mPosition += translationVec;
    mTransform.mTranslationMatrix = glm::translate(glm::mat4{1.0}, mTransform.mPosition);
    // getLocalTransform();
    getGlobalTransform();
    return *this;
}

BaseModel& BaseModel::moveTo(const glm::vec3& moveVec) {
    mTransform.mDirty = true;
    mTransform.mPosition = moveVec;
    mTransform.mTranslationMatrix = glm::translate(glm::mat4{1.0}, mTransform.mPosition);
    // getLocalTransform();
    getGlobalTransform();
    return *this;
}

Model& Model::uploadToGPU(Application* app) {
    for (auto& [_mat_id, mesh] : mFlattenMeshes) {
        // std::cout << getName() << " mesh has " << mesh.mVertexData.size() << '\n';
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
    // }
    return *this;
};

size_t BaseModel::getVertexCount() const { return mFlattenMeshes.at(0).mVertexData.size(); }

Buffer BaseModel::getIndexBuffer() { return mIndexBuffer; }

void BaseModel::setTransparent(bool value) {
    for (auto& [mat_id, mesh] : mFlattenMeshes) {
        mesh.isTransparent = value;
    }
    mIsTransparent = value;
}

bool BaseModel::isTransparent() { return mIsTransparent; }

void BaseModel::selected(bool selected) {
    mTransform.mDirty = true;
    mTransform.mObjectInfo.isSelected = selected;
}
bool BaseModel::isSelected() const { return mTransform.mObjectInfo.isSelected; }

void Model::createSomeBinding(Application* app, std::vector<WGPUBindGroupEntry> bindingData) {
    std::array<WGPUBindGroupEntry, 3> mBindGroupEntry = {};
    mBindGroupEntry[0].nextInChain = nullptr;
    mBindGroupEntry[0].binding = 0;
    mBindGroupEntry[0].buffer = Drawable::getUniformBuffer().getBuffer();
    mBindGroupEntry[0].offset = 0;
    mBindGroupEntry[0].size = sizeof(ObjectInfo);

    mBindGroupEntry[1].nextInChain = nullptr;
    mBindGroupEntry[1].buffer = mTransform.mObjectInfo.isAnimated ? mSkiningTransformationBuffer.getBuffer()
                                                                  : app->mDefaultBoneFinalTransformData.getBuffer();
    mBindGroupEntry[1].binding = 1;
    mBindGroupEntry[1].offset = 0;
    mBindGroupEntry[1].size = 100 * sizeof(glm::mat4);

    mBindGroupEntry[2].nextInChain = nullptr;
    mBindGroupEntry[2].buffer = mGlobalMeshTransformationBuffer.getBuffer();
    mBindGroupEntry[2].binding = 2;
    mBindGroupEntry[2].offset = 0;
    mBindGroupEntry[2].size = 20 * sizeof(glm::mat4);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = mBindGroupEntry.data();
    mTrasBindGroupDesc.entryCount = 3;
    mTrasBindGroupDesc.label = WGPUStringView{"translation bind group", WGPU_STRLEN};
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);

    for (auto& [mat_id, mesh] : mFlattenMeshes) {
        mesh.binding_data = bindingData;
        // if (mesh.isTransparent) {
        //     continue;
        // }

        mesh.binding_data[0].nextInChain = nullptr;
        mesh.binding_data[0].binding = 0;
        mesh.binding_data[0].textureView = mesh.mTexture != nullptr && mesh.mTexture->isValid()
                                               ? mesh.mTexture->getTextureView()
                                               : app->mDefaultDiffuse->getTextureView();

        mesh.binding_data[1].nextInChain = nullptr;
        mesh.binding_data[1].binding = 1;
        mesh.binding_data[1].textureView = mesh.mSpecularTexture != nullptr && mesh.mSpecularTexture->isValid()
                                               ? mesh.mSpecularTexture->getTextureView()
                                               : app->mDefaultMetallicRoughness->getTextureView();

        mesh.binding_data[2].nextInChain = nullptr;
        mesh.binding_data[2].binding = 2;
        mesh.binding_data[2].textureView = mesh.mNormalMapTexture != nullptr && mesh.mNormalMapTexture->isValid()
                                               ? mesh.mNormalMapTexture->getTextureView()
                                               : app->mDefaultNormalMap->getTextureView();

        auto& desc = app->mDefaultTextureBindingGroup.getDescriptor();
        desc.entries = mesh.binding_data.data();
        desc.entryCount = mesh.binding_data.size();
        mesh.mTextureBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &desc);

        // Also create and initialize material buffer
        mesh.mMaterialBuffer.setLabel("")
            .setSize(sizeof(Material))
            .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
            .setMappedAtCraetion(false)
            .create(&mApp->getRendererResource());
        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                             sizeof(Material));

        std::array<WGPUBindGroupEntry, 2> material_bg_entries = {};
        material_bg_entries[0].nextInChain = nullptr;
        material_bg_entries[0].buffer = mesh.mMaterialBuffer.getBuffer();
        material_bg_entries[0].binding = 0;
        material_bg_entries[0].offset = 0;
        material_bg_entries[0].size = sizeof(Material);

        // Also create and initialize material buffer
        mesh.mMeshMapIdx.setLabel("mesh map index")
            .setSize(sizeof(uint32_t))
            .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
            .setMappedAtCraetion(false)
            .create(&mApp->getRendererResource());

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMeshMapIdx.getBuffer(), 0, &mesh.meshId,
                             sizeof(uint32_t));

        material_bg_entries[1].nextInChain = nullptr;
        material_bg_entries[1].buffer = mesh.mMeshMapIdx.getBuffer();
        material_bg_entries[1].binding = 1;
        material_bg_entries[1].offset = 0;
        material_bg_entries[1].size = sizeof(uint32_t);

        WGPUBindGroupDescriptor mat_desc = {};
        mat_desc.nextInChain = nullptr;
        mat_desc.entries = material_bg_entries.data();
        mat_desc.entryCount = 2;
        mat_desc.label = WGPUStringView{"mesh material bind group", WGPU_STRLEN};
        mat_desc.layout = app->mBindGroupLayouts[6];

        mesh.mMaterialBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mat_desc);
    }
}

void Model::update(Application* app, float dt, float physicSimulating) {
    auto& queue = app->getRendererResource().queue;

    updateAnimation(dt);

    if (mScene->HasAnimations() && anim->getActiveAction() && anim->getActiveAction()->hasSkining) {
        wgpuQueueWriteBuffer(queue, mSkiningTransformationBuffer.getBuffer(), 0 * sizeof(glm::mat4),
                             anim->mFinalTransformations.data(),
                             anim->mFinalTransformations.size() * sizeof(glm::mat4));
    }

    // Apply position/Rotation changes to the meshes
    if (mPhysicComponent != nullptr && physicSimulating) {
        auto [new_pos, rotation] = physics::getPositionAndRotationyId(mPhysicComponent->bodyId);
        moveTo(new_pos);
        rotate(glm::normalize(rotation));
    }

    // If object is diry, then update its buffer
    if (mTransform.mDirty) {
        wgpuQueueWriteBuffer(queue, Drawable::getUniformBuffer().getBuffer(), 0, &mTransform.mObjectInfo,
                             sizeof(ObjectInfo));
        for (auto& [id, mesh] : mFlattenMeshes) {
            wgpuQueueWriteBuffer(queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial, sizeof(Material));
        }
        mTransform.mDirty = false;
    }
}

void Model::internalDraw(Application* app, WGPURenderPassEncoder encoder, Node* node) {
    for (auto& mid : node->mMeshIndices) {
        auto& mesh = mFlattenMeshes[mid];
        WGPUBindGroup active_bind_group = nullptr;
        // if (!mesh.isTransparent) {
        active_bind_group = app->getBindingGroup().getBindGroup();

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                             wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh.mIndexBuffer.getBuffer(), WGPUIndexFormat_Uint32, 0,
                                            wgpuBufferGetSize(mesh.mIndexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(encoder, 2,
                                          mesh.mTextureBindGroup == nullptr
                                              ? app->mDefaultTextureBindingGroup.getBindGroup()
                                              : mesh.mTextureBindGroup,
                                          0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(encoder, 6, mesh.mMaterialBindGroup, 0, nullptr);
        if (this->instance != nullptr) {
            // wgpuRenderPassEncoderDrawIndexedIndirect(encoder, mIndirectDrawArgsBuffer.getBuffer(), 0);
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(), instance->getInstanceCount(), 0, 0, 0);
        } else {
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(), 1, 0, 0, 0);
        }
    }
}

void Model::drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node) {
    if (node == nullptr) {
        return;
    }
    // std::cout << "DDDDDDrawing child " << node->mName << node->mChildrens.size() << std::endl;

    // Draw the node itself
    internalDraw(app, encoder, node);

    // Draw its childs
    for (auto* child : node->mChildrens) {
        // Drawing functionality
        // std::cout << "Drawing child " << child->mName << child->mChildrens.size() << std::endl;
        drawGraph(app, encoder, child);
    }
}

void Model::drawHirarchy(Application* app, WGPURenderPassEncoder encoder) { drawGraph(app, encoder, mRootNode); }

void Model::draw(Application* app, WGPURenderPassEncoder encoder) {
    auto& render_resource = app->getRendererResource();
    WGPUBindGroup active_bind_group = nullptr;

    for (auto& [mat_id, mesh] : mFlattenMeshes) {
        // if (!mesh.isTransparent) {
        active_bind_group = app->getBindingGroup().getBindGroup();

        wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh.mVertexBuffer.getBuffer(), 0,
                                             wgpuBufferGetSize(mesh.mVertexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh.mIndexBuffer.getBuffer(), WGPUIndexFormat_Uint32, 0,
                                            wgpuBufferGetSize(mesh.mIndexBuffer.getBuffer()));
        wgpuRenderPassEncoderSetBindGroup(encoder, 0, active_bind_group, 0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(encoder, 1, ggg, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(encoder, 2,
                                          mesh.mTextureBindGroup == nullptr
                                              ? app->mDefaultTextureBindingGroup.getBindGroup()
                                              : mesh.mTextureBindGroup,
                                          0, nullptr);

        wgpuRenderPassEncoderSetBindGroup(encoder, 6, mesh.mMaterialBindGroup, 0, nullptr);
        if (this->instance != nullptr) {
            wgpuRenderPassEncoderDrawIndexedIndirect(encoder, mIndirectDrawArgsBuffer.getBuffer(), 0);
        } else {
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(), 1, 0, 0, 0);
        }
    }
}

std::shared_ptr<Texture> ShowTextureRegisteryCombo(Application* app, Model* model, Mesh& mesh) {
    std::shared_ptr<Texture> res = nullptr;
    ImGui::OpenPopup("Texture list");
    if (ImGui::BeginPopupModal("Texture Registery", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginCombo("Textures", "test")) {
            for (const auto [name, texture] : app->mTextureRegistery->list()) {
                ImGui::PushID((void*)texture.get());
                if (ImGui::Selectable(name.c_str(), false)) {
                    res = texture;
                }
                ImGui::PopID();  // Pop the unique ID for this item
            }
            ImGui::EndCombo();
        }
        ImGui::EndPopup();
    }
    return res;
}

#ifdef DEVELOPMENT_BUILD
void Model::userInterface() {
    ImGuiIO& io = ImGui::GetIO();

    // Determine the drag speed based on modifier keys
    float drag_speed = 1.0f;  // Default speed
    bool lock_scale = false;
    if (io.KeyCtrl) {
        drag_speed = 0.1f;  // Finer control when Ctrl is held
    } else if (io.KeyShift) {
        lock_scale = true;
    }

    if (ImGui::CollapsingHeader("Transformations",
                                ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
        bool happend = false;
        ImGui::Text("Coordinate sysmte is :");
        ImGui::SameLine();
        switch (getCoordinateSystem()) {
            case Y_UP: {
                ImGui::TextColored(ImVec4{0.0, 1.0, 0.0, 1.0}, "%s", "Y UP");
                break;
            }
            case Z_UP: {
                ImGui::TextColored(ImVec4{0.0, 0.0, 1.0, 1.0}, "%s", "Z UP");
                break;
            }
            default:

                break;
        }
        ImGui::Text("Path is : %s", mPath.c_str());
        if (ImGui::DragFloat3("Position2", glm::value_ptr(mTransform.mPosition), 0.1)) {
            happend = true;
        }

        if (ImGui::DragFloat("Scale x", &mTransform.mScale.x, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.x};
            }
            happend = true;
        }
        if (ImGui::DragFloat("Scale y", &mTransform.mScale.y, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.y};
            }
            happend = true;
        }
        if (ImGui::DragFloat("Scale z", &mTransform.mScale.z, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.z};
            }
            happend = true;
        }

        // if (ImGui::DragFloat4("Rotation", glm::value_ptr(mTransform.mOrientation), 0.1, 0.0f, 360.0f)) {
        //     glm::vec3 euler_radians =
        //         glm::eulerAngles(mTransform.mOrientation);  // glm::radians(mTransform.mEulerRotation);
        //
        //     if (mTransform.mEulerRotation.x < 0) mTransform.mEulerRotation.x += 360.0f;
        //     if (mTransform.mEulerRotation.y < 0) mTransform.mEulerRotation.y += 360.0f;
        //     if (mTransform.mEulerRotation.z < 0) mTransform.mEulerRotation.z += 360.0f;
        //     if (mTransform.mEulerRotation.x > 360.0) mTransform.mEulerRotation.x -= 360.0f;
        //     if (mTransform.mEulerRotation.y > 360.0) mTransform.mEulerRotation.y -= 360.0f;
        //     if (mTransform.mEulerRotation.z > 360.0) mTransform.mEulerRotation.z -= 360.0f;
        //
        //     // mTransform.mOrientation = glm::quat(euler_radians);
        //
        //     // mTransform.mOrientation = glm::normalize(mTransform.mOrientation);
        //     // mTransform.mRotationMatrix = glm::toMat4(mTransform.mOrientation);
        //     // happend = true;
        // }

        if (happend) {
            moveTo(mTransform.mPosition);
            scale(mTransform.mScale);
            getGlobalTransform();
        }
    }

    bool is_animated = mTransform.mObjectInfo.isAnimated;
    if (ImGui::Checkbox("Animated /Rest Pose", &is_animated)) {
        mTransform.mObjectInfo.isAnimated = is_animated;
        mTransform.mDirty = true;
    }

    if (mTransform.mObjectInfo.isAnimated) {
        if (ImGui::CollapsingHeader("Available Actions:  (press to play)", ImGuiTreeNodeFlags_DefaultOpen)) {
            const std::string active_name =
                anim->activeAction == nullptr ? "Select one##selected" : anim->activeAction->name.c_str();
            if (ImGui::BeginCombo("Clips", active_name.c_str())) {
                for (auto& [name, action] : anim->actions) {
                    ImGui::PushID((void*)&action);
                    bool selected = (name == active_name);
                    if (ImGui::Selectable(name.c_str(), selected)) {
                        mTransform.mObjectInfo.isAnimated = true;
                        mTransform.mDirty = true;
                        anim->playAction(name, true);
                    }
                    ImGui::PopID();  // Pop the unique ID for this item
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("set as default")) {
                mDefaultAction = anim->activeAction;
            }
        }
        ImGui::Text("Default action: %s", mDefaultAction == nullptr ? "None" : mDefaultAction->name.c_str());
    }

    if (ImGui::CollapsingHeader("Mesh transformations")) {
        for (auto& [name, mesh] : mFlattenMeshes) {
            ImGui::Separator();
            if (mesh.mTexture != nullptr && mesh.mTexture->isValid()) {
                ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Texture for Diffuse");
            } else {
                ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "No Texture for Diffuse");
                if (ImGui::BeginCombo("Textures##Diffuse", "test##diffuse")) {
                    for (const auto [name, texture] : mApp->mTextureRegistery->list()) {
                        ImGui::PushID((void*)texture.get());
                        if (ImGui::Selectable(name.c_str(), false)) {
                            mesh.mTexture = texture;
                            createSomeBinding(mApp, mApp->getDefaultTextureBindingData());
                        }
                        ImGui::PopID();  // Pop the unique ID for this item
                    }
                    ImGui::EndCombo();
                }
            }
            if (mesh.mNormalMapTexture != nullptr && mesh.mNormalMapTexture->isValid()) {
                ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Normal for Diffuse");
            } else {
                ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "No Nromal for Diffuse");
                if (ImGui::BeginCombo("Textures##Normal", "test##Normal")) {
                    for (const auto [name, texture] : mApp->mTextureRegistery->list()) {
                        ImGui::PushID((void*)texture.get());
                        if (ImGui::Selectable(name.c_str(), false)) {
                            mesh.mNormalMapTexture = texture;
                            createSomeBinding(mApp, mApp->getDefaultTextureBindingData());
                        }
                        ImGui::PopID();  // Pop the unique ID for this item
                    }
                    ImGui::EndCombo();
                }
            }
            if (mesh.mSpecularTexture != nullptr && mesh.mSpecularTexture->isValid()) {
                ImGui::TextColored(ImVec4(0.0, 1.0, 0.0, 1.0), "Specular for Diffuse");
            } else {
                ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "No Specular for Diffuse");
                if (ImGui::BeginCombo("Textures##specular", "test##specular")) {
                    for (const auto [name, texture] : mApp->mTextureRegistery->list()) {
                        ImGui::PushID((void*)texture.get());
                        if (ImGui::Selectable(name.c_str(), false)) {
                            mesh.mSpecularTexture = texture;
                            createSomeBinding(mApp, mApp->getDefaultTextureBindingData());
                        }
                        ImGui::PopID();  // Pop the unique ID for this item
                    }
                    ImGui::EndCombo();
                }
            }
        }
        // for (auto& [name, node] : mNodeNameMap) {
        //     ImGui::PushID((void*)node);
        //
        //     ImGui::Text("%s->%s", name.c_str(), node->mParent != nullptr ? node->mParent->mName.c_str() :
        //     "world"); auto [pos, sc, rot] = decomposeTransformation(node->mLocalTransform); bool pos_changed =
        //     ImGui::DragFloat3("post", glm::value_ptr(pos)); bool rot_changed = ImGui::DragFloat3("rot",
        //     glm::value_ptr(rot)); bool scale_changed = ImGui::DragFloat3("scale", glm::value_ptr(sc));
        //     // if (pos_changed || rot_changed || scale_changed) {
        //     //     auto new_global =
        //     //         glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f),
        //     sc);
        //     //     glm::mat4 parent_global =
        //     //         (node->mParent != nullptr) ? node->mParent->getGlobalTransform() : glm::mat4(1.0f);
        //     //     glm::mat4 newLocal = glm::inverse(parent_global) * new_global;
        //     //     node->mLocalTransform = newLocal;
        //     //
        //     //     populateGlobalMeshesTransformationBuffer(mApp, mRootNode, mGlobalMeshTransformationData,
        //     //     mFlattenMeshes,
        //     //                                              {});
        //     //
        //     //     auto& databuffer = mGlobalMeshTransformationData;
        //     //     wgpuQueueWriteBuffer(mApp->getRendererResource().queue,
        //     mGlobalMeshTransformationBuffer.getBuffer(),
        //     //     0,
        //     //                          databuffer.data(), sizeof(glm::mat4) * databuffer.size());
        //     // }
        //     ImGui::PopID();  // Pop the unique ID for this item
        // }
    }

    if (ImGui::CollapsingHeader("Instances")) {
        size_t instances_count = instance != nullptr ? instance->mPositions.size() : 0;
        int instances_id = instance != nullptr ? instance->mOffsetID : -1;
        ImGui::LabelText("Instance Id", "id #%d", instances_id);
        if (instance != nullptr) {
            for (size_t i = 0; i < instances_count; ++i) {
                ImGui::PushID((void*)&instance->mPositions[i]);
                ImGui::LabelText("Label", "instance #%ld", i);
                bool pos_changed = ImGui::DragFloat3("pos", glm::value_ptr(instance->mPositions[i]), 0.01);
                bool scale_changed = ImGui::DragFloat3("scale", glm::value_ptr(instance->mScale[i]), 0.01);

                if (pos_changed || scale_changed) {
                    instance->createInstanceWrapper(i).moveTo(instance->mPositions[i]);
                }

                ImGui::PopID();  // Pop the unique ID for this item
            }

            if (ImGui::Button("New instance")) {
                size_t new_idx = instance->duplicateLastInstance(glm::vec3{.01}, min, max);
                wgpuQueueWriteBuffer(
                    mApp->getRendererResource().queue, mApp->mInstanceManager->getInstancingBuffer().getBuffer(),
                    ((InstanceManager::MAX_INSTANCE_COUNT * instance->mOffsetID) + new_idx) * sizeof(InstanceData),
                    // (instance->mOffsetID + new_idx) * sizeof(InstanceData),
                    &instance->mInstanceBuffer[new_idx], sizeof(InstanceData));
            }
            if (ImGui::Button("Export instances info")) {
                instance->dumpJson();
            }
        } else {
            ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "No Instance!");
            ImGui::SameLine();
            if (ImGui::Button("Enable Instancing")) {
                auto* ins =
                    new Instance{{}, glm::vec3{1.0, 0.0, 0.0}, {}, {}, glm::vec4{min, 1.0f}, glm::vec4{max, 1.0f}};
                ins->parent = this;
                // ins->mApp = mApp;
                ins->mManager = mApp->mInstanceManager;
                ins->mPositions = {};
                ins->mScale = {};

                ins->mOffsetID = mApp->mInstanceManager->getNewId();
                mTransform.mObjectInfo.instanceOffsetId = ins->mOffsetID;
                mTransform.mDirty = true;
                setInstanced(ins);

                ins->mManager->getInstancingBuffer().queueWrite(
                    (InstanceManager::MAX_INSTANCE_COUNT * ins->mOffsetID) * sizeof(InstanceData),
                    ins->mInstanceBuffer.data(), sizeof(InstanceData) * (ins->mInstanceBuffer.size()));
                // wgpuQueueWriteBuffer(mApp->getRendererResource().queue,
                //                      mApp->mInstanceManager->getInstancingBuffer().getBuffer(),
                //                      (InstanceManager::MAX_INSTANCE_COUNT * ins->mOffsetID) *
                //                      sizeof(InstanceData), ins->mInstanceBuffer.data(), sizeof(InstanceData) *
                //                      (ins->mInstanceBuffer.size()));

                std::cout << "(((((((((((((((( in mesh " << mFlattenMeshes.size() << std::endl;

                mIndirectDrawArgsBuffer.setLabel(("indirect draw args buffer for " + getName()).c_str())
                    .setUsage(WGPUBufferUsage_Storage | WGPUBufferUsage_Indirect | WGPUBufferUsage_CopySrc |
                              WGPUBufferUsage_CopyDst)
                    .setSize(sizeof(DrawIndexedIndirectArgs))
                    .setMappedAtCraetion()
                    .create(&mApp->getRendererResource());
            }
        }
    }

    // if (mTransform.mObjectInfo.isAnimated && anim->getActiveAction() != nullptr) {
    //     for (auto& [name, bone] : anim->activeAction->Bonemap) {
    //         ImGui::PushID((void*)bone);
    //         if (ImGui::Button(name.c_str())) {
    //             if (anim->activeAction->calculatedTransform.contains(name)) {
    //                 // std::cout << "Bone " << name << " Exists!\n";
    //             }
    //         }
    //         ImGui::PopID();  // Pop the unique ID for this item
    //     }
    // }

    if (ImGui::CollapsingHeader("Materials")) {  // DefaultOpen makes it open initially
        for (auto& [id, mesh] : mFlattenMeshes) {
            ImGui::PushID((void*)&mesh);
            ImGui::Text("%s", mesh.mMaterialName.c_str());
            bool has_normal = mesh.mMaterial.hasFlag(MaterialProps::HasNormalMap);
            if (ImGui::Checkbox("Has Normal Map", &has_normal)) {
                mesh.mMaterial.setFlag(MaterialProps::HasNormalMap, has_normal);
                mTransform.mDirty = true;
            }
            bool has_specular = mesh.mMaterial.hasFlag(MaterialProps::HasRoughnessMap);
            if (ImGui::Checkbox("Has Specular Map", &has_specular)) {
                mesh.mMaterial.setFlag(MaterialProps::HasRoughnessMap, has_specular);
                mTransform.mDirty = true;
            }
            bool has_diffuse = mesh.mMaterial.hasFlag(MaterialProps::HasDiffuseMap);
            if (ImGui::Checkbox("Has Diffuse Map", &has_diffuse)) {
                mesh.mMaterial.setFlag(MaterialProps::HasDiffuseMap, has_diffuse);
                mTransform.mDirty = true;
            }
            if (ImGui::SliderFloat("Diffuse Value", &mesh.mMaterial.roughness, 0.0f, 1.0f)) {
                mTransform.mDirty = true;
            }

            if (ImGui::DragFloat3("Uv Values", glm::value_ptr(mesh.mMaterial.uvMultiplier), drag_speed, 100.0f)) {
                mTransform.mDirty = true;
            }
            ImGui::PopID();  // Pop the unique ID for this item
        }
    }
    if (ImGui::CollapsingHeader("Textures")) {  // DefaultOpen makes it open initially
                                                // if (!mFlattenMeshes.empty()) {
        for (auto& [id, mesh] : mFlattenMeshes) {
            if (ImGui::TreeNode(std::format("Textures {}##texture_{}", id, id).c_str())) {
                if (mesh.mTexture != nullptr && mesh.mTexture->isValid()) {
                    ImGui::Text("Diffuse/Albedo");
                    ImGui::Image((ImTextureID)(intptr_t)mesh.mTexture->getTextureView(),
                                 ImVec2(1920 / 4.0, 1022 / 4.0));
                }
                if (mesh.mSpecularTexture != nullptr && mesh.mSpecularTexture->isValid()) {
                    ImGui::Text("Specular");
                    ImGui::Image((ImTextureID)(intptr_t)mesh.mSpecularTexture->getTextureView(),
                                 ImVec2(1920 / 4.0, 1022 / 4.0));
                }
                if (mesh.mNormalMapTexture != nullptr && mesh.mNormalMapTexture->isValid()) {
                    ImGui::Text("Normal Map");
                    ImGui::Image((ImTextureID)(intptr_t)mesh.mNormalMapTexture->getTextureView(),
                                 ImVec2(1920 / 4.0, 1022 / 4.0));
                }
                ImGui::TreePop();
            }
        }
        // }
    }
    // }
}
#endif  // DEVELOPMENT_BUILD

BaseModel& BaseModel::scale(const glm::vec3& s) {
    mTransform.mScale = s;
    mTransform.mDirty = true;
    mTransform.mScaleMatrix = glm::scale(glm::mat4{1.0}, s);
    getGlobalTransform();
    return *this;
}

BaseModel& BaseModel::rotate(const glm::vec3& around, float degree) {
    (void)degree;
    mTransform.mDirty = true;
    mTransform.mEulerRotation += around;
    glm::vec3 euler_radians = glm::radians(mTransform.mEulerRotation);
    mTransform.mRotationMatrix = glm::toMat4(glm::quat(euler_radians));
    mTransform.mOrientation = glm::quat(euler_radians);
    getGlobalTransform();
    return *this;
}
BaseModel& BaseModel::rotate(const glm::quat& rot) {
    mTransform.mOrientation = rot;
    mTransform.mRotationMatrix = glm::toMat4(mTransform.mOrientation);
    return *this;
}

glm::vec3& Transform::getPosition() { return mPosition; }

glm::vec3& Transform::getScale() { return mScale; }

glm::mat4& Transform::getLocalTransform() {
    mTransformMatrix = mTranslationMatrix * mRotationMatrix * mScaleMatrix;
    mObjectInfo.transformation = mTransformMatrix;
    return mTransformMatrix;
}

glm::mat4 Transform::getGlobalTransform(BaseModel* parent) {
    if (parent == nullptr) {
        return getLocalTransform();
    } else {
        return parent->getGlobalTransform() * getLocalTransform();
    }
}

float AABB::calculateVolume() {
    float dx = std::abs(min.x - max.x);
    float dy = std::abs(min.y - max.y);
    float dz = std::abs(min.z - max.z);

    return dx * dy * dz;
}

glm::vec3 AABB::getAABBSize() {
    float dx = std::abs(min.x - max.x);
    float dy = std::abs(min.y - max.y);
    float dz = std::abs(min.z - max.z);

    return glm::vec3{dx, dy, dz};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldSpaceAABB() {
    glm::vec3 corners[8];
    corners[0] = glm::vec3(min.x, min.y, min.z);
    corners[1] = glm::vec3(max.x, min.y, min.z);
    corners[2] = glm::vec3(min.x, max.y, min.z);
    corners[3] = glm::vec3(min.x, min.y, max.z);
    corners[4] = glm::vec3(max.x, max.y, min.z);
    corners[5] = glm::vec3(max.x, min.y, max.z);
    corners[6] = glm::vec3(min.x, max.y, max.z);
    corners[7] = glm::vec3(max.x, max.y, max.z);

    // 2. Initialize worldMin and worldMax
    glm::vec3 worldMin(std::numeric_limits<float>::max());
    glm::vec3 worldMax(std::numeric_limits<float>::lowest());  // Equivalent to -FLT_MAX

    // 3. Transform each corner and update worldMin/worldMax
    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformedCorner = getGlobalTransform() * glm::vec4(corners[i], 1.0f);

        worldMin.x = glm::min(worldMin.x, transformedCorner.x);
        worldMin.y = glm::min(worldMin.y, transformedCorner.y);
        worldMin.z = glm::min(worldMin.z, transformedCorner.z);

        worldMax.x = glm::max(worldMax.x, transformedCorner.x);
        worldMax.y = glm::max(worldMax.y, transformedCorner.y);
        worldMax.z = glm::max(worldMax.z, transformedCorner.z);
    }

    return {worldMin, worldMax};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getPhysicsAABB() {
    // 1. Get full world transform
    glm::mat4 world = getGlobalTransform();  // includes pos + rot + scale

    // 2. Extract translation and scale, zero out rotation
    glm::vec3 scale =
        glm::vec3(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])), glm::length(glm::vec3(world[2])));

    glm::vec3 translation = glm::vec3(world[3]);

    glm::mat4 scaleTranslateOnly = glm::mat4(1.0f);
    scaleTranslateOnly = glm::translate(scaleTranslateOnly, translation);
    scaleTranslateOnly = glm::scale(scaleTranslateOnly, scale);

    // 3. Transform the 8 local corners with only scale+translation
    glm::vec3 localCorners[8] = {
        glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z),
    };

    glm::vec3 physMin(+FLT_MAX);
    glm::vec3 physMax(-FLT_MAX);

    for (int i = 0; i < 8; ++i) {
        glm::vec4 t = scaleTranslateOnly * glm::vec4(localCorners[i], 1.0f);
        glm::vec3 p(t.x, t.y, t.z);
        physMin = glm::min(physMin, p);
        physMax = glm::max(physMax, p);
    }

    return {physMin, physMax};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldMin() {
    auto min = getGlobalTransform() * glm::vec4(this->min, 1.0);
    auto max = getGlobalTransform() * glm::vec4(this->max, 1.0);
    return {min, max};
}

const std::string& BaseModel::getName() { return mName; }

Texture* BaseModel::getDiffuseTexture() { return mFlattenMeshes[0].mTexture.get(); }

glm::mat4 BaseModel::getGlobalTransform() {
    auto* root = mParent;
    while (root != nullptr) {
        root = mParent->mParent;
    }

    return mTransform.getGlobalTransform(root);
}

void BaseModel::updateHirarchy() {
    mTransform.mDirty = true;
    for (auto* child : mChildrens) {
        child->updateHirarchy();
        child->mTransform.mObjectInfo.transformation = getGlobalTransform() * child->mTransform.mTransformMatrix;
    }
}

WGPUBindGroup Model::getObjectInfoBindGroup() { return ggg; }
