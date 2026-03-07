#include "mesh.h"

void Mesh::setVisible(bool visibility) { mIsVisible = visibility; }

bool Mesh::getVisible() const { return mIsVisible; }

void Mesh::setMatreial(const std::shared_ptr<Material> mat) {
    mTextureMaterial = mat;
    mNormalMapTexture = mat->mNormalMap;
    mSpecularTexture = mat->mSpecularMap;
    mTexture = mat->mDiffuseMap;
}

std::shared_ptr<Material> Mesh::getMatreial() { return mTextureMaterial; }
