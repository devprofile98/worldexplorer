
#ifndef WORLD_EXPLORER_CORE_MATERIAL_H
#define WORLD_EXPLORER_CORE_MATERIAL_H

#include <memory>
#include <string>

class Texture;

struct Material {
        std::string mName;
        std::shared_ptr<Texture> mDiffuseMap;
        std::shared_ptr<Texture> mNormalMap;
        std::shared_ptr<Texture> mSpecularMap;
};

#endif  //! WORLD_EXPLORER_APP_TERRAIN_H
