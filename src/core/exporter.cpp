

#include <assimp/material.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "assmip/include/assimp/Importer.hpp"
#include "assmip/include/assimp/postprocess.h"
#include "assmip/include/assimp/scene.h"
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

using loadTextureResType = std::vector<std::array<std::string, 3>>;

loadTextureResType loadModelTextures(const std::filesystem::path& path) {
    // load model from disk
    //
    std::string diffuse_path = "";
    std::string normal_path = "";
    std::string specular_path = "";
    loadTextureResType res;

    Assimp::Importer importer;
    const aiScene* const scene = importer.ReadFile(path.string().c_str(), 0);
    if (scene == nullptr) {
        res.push_back({diffuse_path, normal_path, specular_path});
        return res;
    }
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        // texture types you care about
        aiTextureType types[] = {
            aiTextureType_DIFFUSE,            //
            aiTextureType_DIFFUSE_ROUGHNESS,  //
            aiTextureType_HEIGHT,             //
            aiTextureType_NORMALS,            //
            // aiTextureType_METALNESS,          //
            // aiTextureType_EMISSIVE,           //
        };

        for (auto type : types) {
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString path;
                mat->GetTexture(type, i, &path);
                std::string ppath = path.C_Str();

                size_t pos = 0;
                while ((pos = ppath.find("%20", pos)) != std::string::npos) {
                    ppath.replace(pos, 3, " ");  // replace 3 chars ("%20") with 1 space
                    pos += 1;                    // move past the inserted space
                }
                switch (type) {
                    case aiTextureType_DIFFUSE: {
                        diffuse_path = ppath;
                        break;
                    }
                    case aiTextureType_NORMALS:
                    case aiTextureType_HEIGHT: {
                        normal_path = ppath;
                        break;
                    }

                    case aiTextureType_DIFFUSE_ROUGHNESS: {
                        specular_path = ppath;
                        break;
                    }
                    default:
                        break;
                }

                std::cout << "Mesh " << mesh->mName.C_Str() << " uses texture: " << ppath << "\n";
            }
        }
        res.push_back({diffuse_path, normal_path, specular_path});
        // return {diffuse_path, normal_path, specular_path};
    }
    return res;
}

void createExportDir(const fs::path& path) {
    std::cout << "Creating export file at " << path << std::endl;

    fs::create_directories(path);
}

bool exportModels(const fs::path& assetDir, json& objects) {
    std::error_code ec;
    auto models_dir = assetDir / "models";
    fs::create_directories(models_dir, ec);
    if (ec) {
        std::cerr << "Failed to create 'models' direcotry at " << assetDir.string() << ", Quiting!\n";
        return false;
    }

    for (auto& obj : objects) {
        std::string name = obj["name"];
        std::string path = obj["path"];

        if (path.empty()) {
            continue;
        }
        auto entry = fs::path(path);
        fs::create_directories(models_dir / entry.stem(), ec);
        if (ec) {
        }
        auto target_path = models_dir / entry.stem() / entry.filename();

        bool copied = fs::copy_file(entry, target_path, fs::copy_options::update_existing, ec);

        if (copied) {
            std::cout << "Exported: " << target_path << "\n";
        } else {
            std::cout << "Skipped (up to date): " << target_path << "\n";
        }

        // update json file
        obj["path"] = target_path;

        if (entry.has_extension() && entry.extension() == ".gltf") {
            fs::path bin = entry.parent_path() / (entry.stem().string() + ".bin");
            auto target_path = models_dir / entry.stem() / (entry.stem().string() + ".bin");
            bool copied = fs::copy_file(bin, target_path, fs::copy_options::update_existing, ec);
            if (!copied) {
                std::cout << "Failed to copy bin for the glft " << target_path << '\n';
            }
        }

        if (entry.has_extension() && entry.extension() == ".obj") {
            fs::path bin = entry.parent_path() / (entry.stem().string() + ".mtl");
            auto target_path = models_dir / entry.stem() / (entry.stem().string() + ".mtl");
            bool copied = fs::copy_file(bin, target_path, fs::copy_options::update_existing, ec);
            if (!copied) {
                std::cout << "Failed to copy mtl for the obj " << target_path << '\n';
            }
        }

        auto textures_container = loadModelTextures(entry);
        for (const auto& textures : textures_container) {
            for (int type = 0; type < textures.size(); type++) {
                auto texture_path = fs::path(textures[type]);
                if (texture_path.is_relative()) {
                    texture_path = entry.parent_path() / texture_path;
                }
                auto texture_target_path = models_dir / entry.stem() / texture_path.filename();
                if (fs::path(textures[type]).is_relative()) {
                    texture_target_path = models_dir / entry.stem() / fs::path(textures[type]);
                }
                std::cout << "Texture at  " << type << texture_path.string() << '\n';

                if (texture_target_path.has_parent_path()) {
                    fs::create_directories(texture_target_path.parent_path(), ec);
                }
                bool copied = fs::copy_file(texture_path, texture_target_path, fs::copy_options::update_existing, ec);
                if (name == "chair") {
                    std::cout << "HAHAHAHAHAHAHAH\n";
                }
                if (!copied) {
                    std::cout << "Failed :: Texture at  " << textures[type] << " " << texture_path.string() << '\n';
                }
            }
        }
    }
    return true;
}

bool exportMaterials(const fs::path& assetDir, json& materials) {
    std::error_code ec;
    auto mat_dir = assetDir / "materials";
    fs::create_directories(mat_dir, ec);
    std::array<std::string, 3> keys = {"diffuse_map", "normal_map", "specular_map"};

    if (ec) {
        std::cerr << "Failed to create 'materials' direcotry at " << assetDir.string() << ", Quiting!\n";
        return false;
    }

    for (auto& mat : materials) {
        std::error_code ec;
        auto name = mat["name"].get<std::string>();
        auto diffuse = mat["diffuse_map"];
        auto normal = mat["normal_map"];
        auto specular = mat["specular_map"];

        if (name.empty()) {
            std::cerr << "Material has no name, so it will be ignored\n";
            continue;
        }

        fs::create_directories(mat_dir / name, ec);
        if (ec) {
        } else {
            std::array<json, 3> paths = {diffuse, normal, specular};
            int index = 0;
            for (const auto& path : paths) {
                if (path.is_null()) {
                    continue;
                    index++;
                }
                auto entry = fs::path(path.get<std::string>());
                auto target_path = mat_dir / name / entry.filename();
                bool copied = fs::copy_file(entry, target_path, fs::copy_options::update_existing, ec);
                if (copied) {
                    mat[keys[index]] = target_path;
                }
                index++;
            }
        }
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Not enough argument, at least provide the scene file path" << std::endl;
        return 2;
    }

    std::string scene_file_name = argv[1];

    auto target_scene = fs::path(RESOURCE_DIR) / scene_file_name;
    std::cout << "Target Scene is " << target_scene << std::endl;
    std::ifstream world_file(target_scene);

    auto asset_dir = std::filesystem::current_path() / "export" / "assets";
    createExportDir(asset_dir);

    json j;
    json res = j.parse(world_file);

    exportModels(asset_dir, res["objects"]);
    exportMaterials(asset_dir, res["materials"]);

    std::ofstream out;
    out.open(asset_dir / "scene.json", std::ios::trunc);
    auto scene_dump = res.dump(2);
    out.write(scene_dump.c_str(), scene_dump.size());

    return 0;
}
