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
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "assimp/matrix4x4.h"
#include "assimp/mesh.h"
#include "assimp/quaternion.h"
#include "assimp/vector3.h"
#include "glm/ext/matrix_common.hpp"
#include "mesh.h"
#include "texture.h"
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

const char* model_name = "cube1";

Drawable::Drawable() {}

void Drawable::configure(Application* app) {
    mUniformBuffer.setLabel("Uniform buffer for object info")
        .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform)
        .setSize(sizeof(ObjectInfo))
        .setMappedAtCraetion()
        .create(app);
}

void Drawable::draw(Application* app, WGPURenderPassEncoder encoder) {
    (void)app;
    (void)encoder;
}

Buffer& Drawable::getUniformBuffer() { return mUniformBuffer; }

Model::Model() : BaseModel() {
    mTransform.mScaleMatrix = glm::scale(mTransform.mScaleMatrix, mTransform.mScale);
    mTransform.mTransformMatrix = mTransform.mRotationMatrix * mTransform.mTranslationMatrix * mTransform.mScaleMatrix;
    mTransform.mObjectInfo.transformation = mTransform.mTransformMatrix;
    // mTransform.mObjectInfo.isFlat = 0;
    // mTransform.mObjectInfo.useTexture = 1;
    // mTransform.mObjectInfo.isFoliage = 0;
    // mTransform.mObjectInfo.uvMultiplier = {1.0, 1.0, 1.0};
    mTransform.mObjectInfo.isAnimated = 0;
    mTransform.mDirty = true;
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

void printMeshBoneData(aiMesh* mesh) {
    std::cout << "has bone " << mesh->HasBones() << " and " << mesh->mNumBones << std::endl;
}

void printAnimationInfos(const std::string& name, const aiScene* scene) {
    if (scene->mAnimations != nullptr) {
        std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ There are animations " << name << " " << scene->mNumAnimations
                  << std::endl;
        for (size_t i = 0; i < scene->mNumAnimations; i++) {
            const aiAnimation* anim = scene->mAnimations[i];
            std::cout << "@@@@@@@@@@@@@@@@@@@@@ " << anim->mName.C_Str() << " " << anim->mNumChannels << std::endl;
            for (size_t j = 0; j < anim->mNumChannels; j++) {
                const aiNodeAnim* nodeAnim = anim->mChannels[j];
                for (size_t k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                    std::cout << std::format("at: {}\n", nodeAnim->mPositionKeys[k].mTime / anim->mTicksPerSecond);
                }
            }
        }
    }
}

inline glm::vec3 assimpToGlmVec3(aiVector3D vec) { return glm::vec3(vec.x, vec.y, vec.z); }

inline glm::quat assimpToGlmQuat(aiQuaternion quat) {
    glm::quat q;
    q.x = quat.x;
    q.y = quat.y;
    q.z = quat.z;
    q.w = quat.w;

    return q;
}

size_t findPositionKey(double time, const aiNodeAnim* channel) {
    for (size_t i = 0; i < channel->mNumPositionKeys; i++) {
        if (time < channel->mPositionKeys[i].mTime) {
            return i;
        }
    }
    return channel->mNumPositionKeys - 1;
}

size_t findRotationKey(double time, const aiNodeAnim* channel) {
    for (size_t i = 0; i < channel->mNumRotationKeys; ++i) {
        if (time < channel->mRotationKeys[i].mTime) {
            return i;
        }
    }
    return channel->mNumRotationKeys - 1;
}

glm::vec3 calculateInterpolatedPosition(double time, const aiNodeAnim* channel) {
    if (channel->mNumPositionKeys == 1) {
        return assimpToGlmVec3(channel->mPositionKeys[0].mValue);
    }

    size_t pose_idx = findPositionKey(time, channel);
    if (pose_idx == channel->mNumPositionKeys - 1) {
        return assimpToGlmVec3(channel->mPositionKeys[pose_idx].mValue);
    }

    double deltatime = channel->mPositionKeys[pose_idx + 1].mTime - channel->mPositionKeys[pose_idx].mTime;
    double factor = (time - channel->mPositionKeys[pose_idx].mTime) / deltatime;
    const glm::vec3& start = assimpToGlmVec3(channel->mPositionKeys[pose_idx].mValue);
    const glm::vec3& end = assimpToGlmVec3(channel->mPositionKeys[pose_idx + 1].mValue);
    // std::cout << "the fatcor is " << factor << std::endl;
    return glm::mix(start, end, factor);
}

glm::vec3 calculateInterpolatedScale(double time, const aiNodeAnim* channel) {
    if (channel->mNumScalingKeys == 1) {
        return assimpToGlmVec3(channel->mScalingKeys[0].mValue);
    }

    size_t pose_idx = findPositionKey(time, channel);
    if (pose_idx == channel->mNumScalingKeys - 1) {
        return assimpToGlmVec3(channel->mScalingKeys[pose_idx].mValue);
    }

    double deltatime = channel->mScalingKeys[pose_idx + 1].mTime - channel->mScalingKeys[pose_idx].mTime;
    double factor = (time - channel->mScalingKeys[pose_idx].mTime) / deltatime;
    const glm::vec3& start = assimpToGlmVec3(channel->mScalingKeys[pose_idx].mValue);
    const glm::vec3& end = assimpToGlmVec3(channel->mScalingKeys[pose_idx + 1].mValue);
    // std::cout << "the fatcor is " << factor << std::endl;
    return glm::mix(start, end, factor);
}

glm::quat CalcInterpolatedRotation(double time, const aiNodeAnim* channel) {
    if (channel->mNumRotationKeys == 1) {
        return assimpToGlmQuat(channel->mRotationKeys[0].mValue);
    }

    unsigned int idx = findRotationKey(time, channel);
    if (idx == channel->mNumRotationKeys - 1) {
        return assimpToGlmQuat(channel->mRotationKeys[idx].mValue);
    }

    double deltaTime = channel->mRotationKeys[idx + 1].mTime - channel->mRotationKeys[idx].mTime;
    double factor = (time - channel->mRotationKeys[idx].mTime) / deltaTime;
    auto start = assimpToGlmQuat(channel->mRotationKeys[idx].mValue);
    auto end = assimpToGlmQuat(channel->mRotationKeys[idx + 1].mValue);
    glm::quat out = glm::slerp(start, end, (float)factor);

    return out;
}

void Model::buildNodeCache(aiNode* node) {
    if (node == nullptr) {
        return;
    } else {
        mNodeCache[node->mName.C_Str()] = node;
        for (size_t i = 0; i < node->mNumChildren; ++i) {
            buildNodeCache(node->mChildren[i]);
        }
    }
}

aiMatrix4x4 GetGlobalTransform(aiNode* node) {
    aiMatrix4x4 transform = node->mTransformation;
    aiNode* parent = node->mParent;
    while (parent != nullptr) {
        transform =
            parent->mTransformation * transform;  // Note: Assimp matrices are row-major, multiplication order matters
        parent = parent->mParent;
    }
    return transform;
}

glm::mat4 AiToGlm(const aiMatrix4x4& aiMat) {
    return glm::mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3,
                     aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
}

glm::mat4 GetLocalTransformAtTime(const aiNode* node, double time,
                                  const std::map<std::string, aiNodeAnim*>& channelMap) {
    auto it = channelMap.find(node->mName.C_Str());
    if (it != channelMap.end()) {
        // find a pose and use it
        aiNodeAnim* channel = it->second;

        glm::vec3 pos = calculateInterpolatedPosition(time, channel);
        glm::quat rotation = CalcInterpolatedRotation(time, channel);
        glm::vec3 scale = calculateInterpolatedScale(time, channel);

        glm::mat4 translation_mat = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 rotation_mat = glm::mat4(rotation);
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

        return translation_mat * rotation_mat * scale_mat;
    }
    return AiToGlm(node->mTransformation);
}

void ComputeGlobalTransforms(const aiNode* node, const glm::mat4& parentGlobal, double time,
                             const std::map<std::string, aiNodeAnim*>& channelMap,
                             std::map<std::string, glm::mat4>& outGlobalMap) {
    glm::mat4 local = GetLocalTransformAtTime(node, time, channelMap);
    glm::mat4 global = parentGlobal * local;
    outGlobalMap[node->mName.C_Str()] = global;

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ComputeGlobalTransforms(node->mChildren[i], global, time, channelMap, outGlobalMap);
    }
}

void Model::updateAnimation() {
    if (mScene->mAnimations) {
        const aiMatrix4x4 rootTransform = mScene->mRootNode->mTransformation;

        // 2. Convert to glm and invert
        // Use a helper function for conversion from Assimp's aiMatrix4x4 to glm::mat4
        glm::mat4 globalInverseMatrix = AiToGlm(rootTransform);
        globalInverseMatrix = glm::inverse(globalInverseMatrix);

        ComputeGlobalTransforms(mScene->mRootNode, globalInverseMatrix, mAnimationSecond, channelMap, globalMap);

    } else {
        return;
    }

    // TODO: fix this rotation, we need a 90 degree rotation to be aligned with z-up coordinate system of the mesh data
    auto rot = glm::rotate(glm::mat4{1.0}, glm::radians(0.0f), glm::vec3{1.0, 0.0, 0.0});

    for (const std::string& boneName : uniqueBones) {
        if (boneToIdx.find(boneName) != boneToIdx.end()) {
            mFinalTransformations[boneToIdx[boneName]] = globalMap[boneName] * mOffsetMatrixCache[boneName] * rot;
        }
    }
}

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
    glm::mat4 nodeTransform = aiMatrix4x4ToGlm(node->mTransformation);
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Process meshes in this node
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        unsigned int mid = node->mMeshes[i];
        aiMesh* mesh = scene->mMeshes[mid];
        processMesh(app, mesh, scene, mid, globalTransform);
    }

    // Recursively process child nodes
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        processNode(app, node->mChildren[i], scene, globalTransform);
    }
}

// Modified processMesh to accept global transformation
void Model::processMesh(Application* app, aiMesh* mesh, const aiScene* scene, unsigned int meshId,
                        const glm::mat4& globalTransform) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    std::map<size_t, std::vector<std::pair<size_t, float>>> bonemap;

    size_t index_offset = mMeshes[mesh->mMaterialIndex].mVertexData.size();
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ " << meshId << " " << mesh->mMaterialIndex << '\n';

    if (mesh->HasBones()) {
        std::cout << " ))))))))))))))))) has bone " << mesh->HasBones() << " and " << mesh->mNumBones << std::endl;
        for (uint32_t i = 0; i < mesh->mNumBones; ++i) {
            auto bone = mesh->mBones[i];
            for (size_t j = 0; j < bone->mNumWeights; ++j) {
                if (boneToIdx.count(bone->mName.C_Str()) == 0) {
                    boneToIdx[bone->mName.C_Str()] = boneToIdx.size();
                }
                if (bonemap.count(bone->mWeights[j].mVertexId) == 0) {
                    bonemap[bone->mWeights[j].mVertexId] = {};
                }
                bonemap[bone->mWeights[j].mVertexId].push_back(
                    {boneToIdx[bone->mName.C_Str()], bone->mWeights[j].mWeight});
            }
        }
    }

    // Compute normal transformation (inverse transpose for normals)
    glm::mat4 normalTransform = glm::transpose(glm::inverse(globalTransform));

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        VertexAttributes vertex;

        // Transform vertex position
        glm::vec4 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        pos = globalTransform * pos;
        glm::vec3 vector(pos.x, pos.y, pos.z);

        // Adjust coordinate system (as in your original code)
        vertex.position.x = vector.x;
        vertex.position.z = vector.y;
        vertex.position.y = -vector.z;

        size_t bid_cnt = 0;
        if (mesh->HasBones()) {
            for (const auto& [bid, bwg] : bonemap[i]) {
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
            normal = glm::mat3(normalTransform) * normal;
            vertex.normal.x = normal.x;
            vertex.normal.y = normal.z;
            vertex.normal.z = normal.y;
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
                tangent = glm::mat3(globalTransform) * tangent;
                vertex.tangent.x = tangent.x;
                vertex.tangent.y = tangent.z;
                vertex.tangent.z = tangent.y;

                // Transform bitangent
                glm::vec3 bitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                bitangent = glm::mat3(globalTransform) * bitangent;
                vertex.biTangent.x = bitangent.x;
                vertex.biTangent.y = bitangent.z;
                vertex.biTangent.z = bitangent.y;
            }
        } else {
            vertex.uv = glm::vec2{0.0f, 0.0f};
        }

        mMeshes[mesh->mMaterialIndex].mVertexData.push_back(vertex);
    }

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            mMeshes[mesh->mMaterialIndex].mIndexData.push_back((uint32_t)face.mIndices[j] + index_offset);
        }
    }

    auto& render_resource = app->getRendererResource();
    auto& mmesh = mMeshes[mesh->mMaterialIndex];

    auto load_texture = [&](aiTextureType type, MaterialProps flag, Texture** target) {
        for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
            mmesh.mMaterial.setFlag(flag, true);
            aiString str;
            material->GetTexture(type, i, &str);
            std::string texture_path = RESOURCE_DIR;
            texture_path += "/";
            texture_path += str.C_Str();
            *target = new Texture{render_resource.device, texture_path};
            if ((*target)->createView() == nullptr) {
                std::cout << std::format("Failed to create diffuse Texture view for {} at {}\n", mName, texture_path);
            } else {
                std::cout << std::format("succesfully create view for {} at {}\n", mName, texture_path);
            }
            (*target)->uploadToGPU(render_resource.queue);
            mmesh.isTransparent = (*target)->isTransparent();
        }
    };

    if (mmesh.mTexture == nullptr) {
        load_texture(aiTextureType_DIFFUSE, MaterialProps::HasDiffuseMap, &mmesh.mTexture);
    }
    if (mmesh.mSpecularTexture == nullptr) {
        load_texture(aiTextureType_DIFFUSE_ROUGHNESS, MaterialProps::HasRoughnessMap, &mmesh.mSpecularTexture);
        // aiTextureType_SPECULAR
    }
    if (mmesh.mNormalMapTexture == nullptr) {
        load_texture(aiTextureType_HEIGHT, MaterialProps::HasNormalMap, &mmesh.mNormalMapTexture);
    }
    if (mmesh.mNormalMapTexture == nullptr) {
        load_texture(aiTextureType_NORMALS, MaterialProps::HasNormalMap, &mmesh.mNormalMapTexture);
    }
}

Model& Model::load(std::string name, Application* app, const std::filesystem::path& path, WGPUBindGroupLayout layout) {
    // load model from disk
    (void)layout;
    mName = name;

    Drawable::configure(app);

    std::string warn;
    std::string err;
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    reader_config.mtl_search_path = "/home/ahmad/Documents/project/cpp/wgputest/resources";

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

    mNodeCache.clear();
    mFinalTransformations.reserve(100);
    mFinalTransformations.resize(100);
    buildNodeCache(mScene->mRootNode);

    for (unsigned int m = 0; m < mScene->mNumMeshes; ++m) {
        aiMesh* mesh = mScene->mMeshes[m];
        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();

            if (uniqueBones.find(boneName) != uniqueBones.end()) continue;
            uniqueBones.insert(boneName);

            if (mOffsetMatrixCache.find(boneName) != mOffsetMatrixCache.end()) continue;
            mOffsetMatrixCache[boneName] = AiToGlm(bone->mOffsetMatrix);
        }
    }

    /* Create a mapping from [name] -> [channel data]*/
    if (mScene->HasAnimations()) {
        aiAnimation* anim = mScene->mAnimations[0];
        std::cout << " -----**************************************************** " << getName() << " "
                  << mScene->mNumAnimations << std::endl;
        mAnimationDuration = anim->mDuration / anim->mTicksPerSecond;
        for (size_t i = 0; i < anim->mNumChannels; ++i) {
            std::cout << " ----- " << anim->mChannels[i]->mNumPositionKeys << std::endl;
            aiNodeAnim* channel = anim->mChannels[i];
            channelMap[channel->mNodeName.C_Str()] = channel;
        }
        mTransform.mObjectInfo.isAnimated = true;
    }
    // craeting the bindgropu for skining data

    updateAnimation();

    // Default to non textured
    // mTransform.mObjectInfo.setFlag(MaterialProps::HasNormalMap, false);
    // mTransform.mObjectInfo.setFlag(MaterialProps::HasRoughnessMap, false);
    // mTransform.mObjectInfo.setFlag(MaterialProps::HasDiffuseMap, false);
    processNode(app, scene->mRootNode, scene, glm::mat4{1.0});

    return *this;
}

void BaseModel::setInstanced(Instance* instance) { this->instance = instance; }

void BaseModel::addChildren(BaseModel* child) {
    auto new_local_transform = glm::inverse(getGlobalTransform()) * child->mTransform.mTransformMatrix;

    glm::vec3 position = glm::vec3(new_local_transform[3]);
    // child->mTransform.mPosition = position;
    child->mTransform.moveTo(position);

    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(new_local_transform[0]));
    scale.y = glm::length(glm::vec3(new_local_transform[1]));
    scale.z = glm::length(glm::vec3(new_local_transform[2]));
    // child->mTransform.mScale = scale;
    child->mParent = this;
    child->mTransform.scale(scale);
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

Transform& Transform::moveBy(const glm::vec3& translationVec) {
    // justify position by the factor `m`
    // (void)translationVec;
    mDirty = true;
    mPosition += translationVec;
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    getLocalTransform();
    return *this;
}

Transform& Transform::moveTo(const glm::vec3& moveVec) {
    mDirty = true;
    mPosition = moveVec;
    mTranslationMatrix = glm::translate(glm::mat4{1.0}, mPosition);
    getLocalTransform();
    return *this;
}

Model& Model::uploadToGPU(Application* app) {
    // upload vertex attribute data to GPU
    for (auto& [_mat_id, mesh] : mMeshes) {
        std::cout << getName() << " mesh has " << mesh.mVertexData.size() << '\n';
        mesh.mVertexBuffer.setLabel("Uniform buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex)
            .setSize((mesh.mVertexData.size() + 1) * sizeof(VertexAttributes))
            .setMappedAtCraetion()
            .create(app);

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mVertexBuffer.getBuffer(), 0,
                             mesh.mVertexData.data(), mesh.mVertexData.size() * sizeof(VertexAttributes));

        /*if (getName() == "steampunk") {*/
        mesh.mIndexBuffer.setLabel("index buffer for object info")
            .setUsage(WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex | WGPUBufferUsage_Index)
            .setSize((mesh.mIndexData.size()) * sizeof(uint32_t))
            .setMappedAtCraetion()
            .create(app);

        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mIndexBuffer.getBuffer(), 0, mesh.mIndexData.data(),
                             mesh.mIndexData.size() * sizeof(uint32_t));
        /*}*/
    }
    /*createSomeBinding(app);*/
    return *this;
};

size_t BaseModel::getVertexCount() const { return mMeshes.at(0).mVertexData.size(); }

/*Buffer BaseModel::getVertexBuffer() { return mVertexBuffer; }*/

Buffer BaseModel::getIndexBuffer() { return mIndexBuffer; }

void BaseModel::setTransparent(bool value) {
    for (auto& [mat_id, mesh] : mMeshes) {
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
    std::array<WGPUBindGroupEntry, 2> mBindGroupEntry = {};
    mBindGroupEntry[0].nextInChain = nullptr;
    mBindGroupEntry[0].binding = 0;
    mBindGroupEntry[0].buffer = Drawable::getUniformBuffer().getBuffer();
    mBindGroupEntry[0].offset = 0;
    mBindGroupEntry[0].size = sizeof(ObjectInfo);

    mBindGroupEntry[1].nextInChain = nullptr;
    mBindGroupEntry[1].buffer = mScene->HasAnimations() ? mSkiningTransformationBuffer.getBuffer()
                                                        : app->mDefaultBoneFinalTransformData.getBuffer();
    mBindGroupEntry[1].binding = 1;
    mBindGroupEntry[1].offset = 0;
    mBindGroupEntry[1].size = 100 * sizeof(glm::mat4);

    WGPUBindGroupDescriptor mTrasBindGroupDesc = {};
    mTrasBindGroupDesc.nextInChain = nullptr;
    mTrasBindGroupDesc.entries = mBindGroupEntry.data();
    mTrasBindGroupDesc.entryCount = 2;
    mTrasBindGroupDesc.label = createStringView("translation bind group");
    mTrasBindGroupDesc.layout = app->mBindGroupLayouts[1];

    ggg = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mTrasBindGroupDesc);

    for (auto& [mat_id, mesh] : mMeshes) {
        mesh.binding_data = bindingData;
        // if (mesh.isTransparent) {
        //     continue;
        // }
        //
        //

        if (mesh.mTexture != nullptr) {
            if (mesh.mTexture->getTextureView() != nullptr) {
                mesh.binding_data[0].nextInChain = nullptr;
                mesh.binding_data[0].binding = 0;
                mesh.binding_data[0].textureView = mesh.mTexture->getTextureView();
            }
            if (mesh.mSpecularTexture != nullptr && mesh.mSpecularTexture->getTextureView() != nullptr) {
                mesh.binding_data[1].nextInChain = nullptr;
                mesh.binding_data[1].binding = 1;
                mesh.binding_data[1].textureView = mesh.mSpecularTexture->getTextureView();
            }
            if (mesh.mNormalMapTexture != nullptr && mesh.mNormalMapTexture->getTextureView() != nullptr) {
                mesh.binding_data[2].nextInChain = nullptr;
                mesh.binding_data[2].binding = 2;
                mesh.binding_data[2].textureView = mesh.mNormalMapTexture->getTextureView();
            }
            auto& desc = app->mDefaultTextureBindingGroup.getDescriptor();
            desc.entries = mesh.binding_data.data();
            desc.entryCount = mesh.binding_data.size();
            mesh.mTextureBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &desc);
        }

        // Also create and initialize material buffer
        mesh.mMaterialBuffer.setLabel("")
            .setSize(sizeof(Material))
            .setUsage(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst)
            .setMappedAtCraetion(false)
            .create(app);
        wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                             sizeof(Material));

        std::array<WGPUBindGroupEntry, 1> material_bg_entries = {};
        material_bg_entries[0].nextInChain = nullptr;
        material_bg_entries[0].buffer = mesh.mMaterialBuffer.getBuffer();
        material_bg_entries[0].binding = 0;
        material_bg_entries[0].offset = 0;
        material_bg_entries[0].size = sizeof(Material);

        WGPUBindGroupDescriptor mat_desc = {};
        mat_desc.nextInChain = nullptr;
        mat_desc.entries = material_bg_entries.data();
        mat_desc.entryCount = 1;
        mat_desc.label = createStringView("mesh material bind group");
        mat_desc.layout = app->mBindGroupLayouts[6];

        mesh.mMaterialBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &mat_desc);
    }
}

void Model::update(Application* app, float dt) {
    if (mScene->HasAnimations()) {
        wgpuQueueWriteBuffer(app->getRendererResource().queue, mSkiningTransformationBuffer.getBuffer(),
                             0 * sizeof(glm::mat4), mFinalTransformations.data(),
                             mFinalTransformations.size() * sizeof(glm::mat4));
    }
    if (mTransform.mDirty) {
        wgpuQueueWriteBuffer(app->getRendererResource().queue, Drawable::getUniformBuffer().getBuffer(), 0,
                             &mTransform.mObjectInfo, sizeof(ObjectInfo));
        for (auto& [id, mesh] : mMeshes) {
            wgpuQueueWriteBuffer(app->getRendererResource().queue, mesh.mMaterialBuffer.getBuffer(), 0, &mesh.mMaterial,
                                 sizeof(Material));
        }
        mTransform.mDirty = false;
    }
}

void Model::draw(Application* app, WGPURenderPassEncoder encoder) {
    auto& render_resource = app->getRendererResource();
    WGPUBindGroup active_bind_group = nullptr;

    for (auto& [mat_id, mesh] : mMeshes) {
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
            wgpuRenderPassEncoderDrawIndexedIndirect(encoder, mesh.mIndirectDrawArgsBuffer.getBuffer(), 0);
        } else {
            wgpuRenderPassEncoderDrawIndexed(encoder, mesh.mIndexData.size(), 1, 0, 0, 0);
        }

        // continue;
        // }
    }
}

// size_t Model::getInstaceCount() { return this->instances; }

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
        ImGui::Text("Position:");
        ImGui::DragFloat3("Position", glm::value_ptr(mTransform.mPosition), drag_speed);

        if (ImGui::DragFloat("Scale x", &mTransform.mScale.x, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.x};
            }
        }
        if (ImGui::DragFloat("Scale y", &mTransform.mScale.y, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.y};
            }
        }
        if (ImGui::DragFloat("Scale z", &mTransform.mScale.z, 0.01)) {
            if (lock_scale) {
                mTransform.mScale = glm::vec3{mTransform.mScale.z};
            }
        }

        if (ImGui::DragFloat3("Rotation", glm::value_ptr(mTransform.mEulerRotation), drag_speed, 360.0f)) {
            glm::vec3 euler_radians = glm::radians(mTransform.mEulerRotation);
            mTransform.mRotationMatrix = glm::toMat4(glm::quat(euler_radians));

            if (mTransform.mEulerRotation.x < 0) mTransform.mEulerRotation.x += 360.0f;
            if (mTransform.mEulerRotation.y < 0) mTransform.mEulerRotation.y += 360.0f;
            if (mTransform.mEulerRotation.z < 0) mTransform.mEulerRotation.z += 360.0f;
            if (mTransform.mEulerRotation.x > 360.0) mTransform.mEulerRotation.x -= 360.0f;
            if (mTransform.mEulerRotation.y > 360.0) mTransform.mEulerRotation.y -= 360.0f;
            if (mTransform.mEulerRotation.z > 360.0) mTransform.mEulerRotation.z -= 360.0f;
        }

        mTransform.moveTo(mTransform.mPosition);
        mTransform.scale(mTransform.mScale);
        mTransform.getLocalTransform();
    }

    bool is_animated = mTransform.mObjectInfo.isAnimated;
    if (ImGui::Checkbox("Is Animated", &is_animated)) {
        mTransform.mObjectInfo.isAnimated = is_animated;
        mTransform.mDirty = true;
    }
    bool dirty = false;
    for (auto& [id, mesh] : mMeshes) {
        ImGui::PushID((void*)&mesh);
        if (ImGui::CollapsingHeader("Materials",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {  // DefaultOpen makes it open initially
                                                                        //
            // bool is_animated = mesh.mMaterial.hasFlag(MaterialProps::IsAnimated);
            // if (ImGui::Checkbox("Is Animated", &is_animated)) {
            //     mesh.mMaterial.setFlag(MaterialProps::IsAnimated, is_animated);
            //     // mTransform.mDirty = true;
            // }
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
        }

        ImGui::PopID();  // Pop the unique ID for this item
    }
    // }
}
#endif  // DEVELOPMENT_BUILD

Transform& Transform::scale(const glm::vec3& s) {
    mScale = s;
    mDirty = true;
    mScaleMatrix = glm::scale(glm::mat4{1.0}, s);
    getLocalTransform();
    return *this;
}

Transform& Transform::rotate(const glm::vec3& around, float degree) {
    (void)degree;
    mDirty = true;
    mEulerRotation += around;
    glm::vec3 euler_radians = glm::radians(mEulerRotation);
    this->mRotationMatrix = glm::toMat4(glm::quat(euler_radians));
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
        glm::vec4 transformedCorner = mTransform.getLocalTransform() * glm::vec4(corners[i], 1.0f);

        worldMin.x = glm::min(worldMin.x, transformedCorner.x);
        worldMin.y = glm::min(worldMin.y, transformedCorner.y);
        worldMin.z = glm::min(worldMin.z, transformedCorner.z);

        worldMax.x = glm::max(worldMax.x, transformedCorner.x);
        worldMax.y = glm::max(worldMax.y, transformedCorner.y);
        worldMax.z = glm::max(worldMax.z, transformedCorner.z);
    }

    return {worldMin, worldMax};
}

std::pair<glm::vec3, glm::vec3> BaseModel::getWorldMin() {
    auto min = mTransform.getLocalTransform() * glm::vec4(this->min, 1.0);
    auto max = mTransform.getLocalTransform() * glm::vec4(this->max, 1.0);
    return {min, max};
}

const std::string& BaseModel::getName() { return mName; }

Texture* BaseModel::getDiffuseTexture() { return mMeshes[0].mTexture; }

glm::mat4 BaseModel::getGlobalTransform() { return mTransform.getGlobalTransform(mParent); }

void BaseModel::update() {
    mTransform.mDirty = true;
    for (auto* child : mChildrens) {
        child->update();
        child->mTransform.mObjectInfo.transformation = getGlobalTransform() * child->mTransform.mTransformMatrix;
    }
}

WGPUBindGroup Model::getObjectInfoBindGroup() { return ggg; }
