
#include "world.h"

#include <glm/fwd.hpp>
#include <utility>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "utils.h"

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
