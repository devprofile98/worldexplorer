
#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/fwd.hpp>
#include <limits>
#include <utility>
#include <vector>

#include "application.h"
#include "binding_group.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "mesh.h"
#include "pipeline.h"
#include "shapes.h"
#include "utils.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <webgpu/webgpu.h>

#include "glm/detail/qualifier.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "linegroup.h"
#include "model.h"
#include "rendererResource.h"
#include "renderpass.h"
#include "wgpu_utils.h"

LineGroup& LineGroup::updateLines(const std::vector<glm::vec4>& newPoints) {
    if (!isInitialized()) {
        std::cout << "Must be initialized first!" << mId << '\n';
        return *this;
    }
    mLineEngine->updateLines(mId, newPoints);
    return *this;
}

LineGroup& LineGroup::updateTransformation(const glm::mat4& trans) {
    if (!isInitialized()) {
        std::cout << "Must be initialized first!" << mId << '\n';
        return *this;
    }
    auto it = mLineEngine->mLineGroups.find(mId);
    if (it == mLineEngine->mLineGroups.end()) {
        std::cout << "Failed to find the line group with id " << mId << '\n';
        return *this;
    }
    LineEngine::LineGroup& group = it->second;
    group.transformation = trans;
    group.dirty = true;
    return *this;
}

LineGroup& LineGroup::updateColor(const glm::vec3& color) {
    if (!isInitialized()) {
        std::cout << "Must be initialized first!" << mId << '\n';
        return *this;
    }
    std::cout << "O rafte o man mande am! " << mId << "\n";
    auto it = mLineEngine->mLineGroups.find(mId);
    std::cout << ": " << mId << std::endl;
    if (it == mLineEngine->mLineGroups.end()) return *this;

    LineEngine::LineGroup& group = it->second;
    for (auto& p : group.segment) {
        p.color = color;
    }
    group.dirty = true;
    return *this;
}

LineGroup& LineGroup::updateVisibility(bool visibility) {
    if (!isInitialized()) {
        std::cout << "Must be initialized first!" << mId << '\n';
        return *this;
    }
    auto it = mLineEngine->mLineGroups.find(mId);
    if (it == mLineEngine->mLineGroups.end()) return *this;

    LineEngine::LineGroup& group = it->second;
    for (auto& p : group.segment) {
        p.isActive = visibility;
    }
    group.dirty = true;
    return *this;
}

bool LineGroup::isInitialized() const { return mInitialized; }

bool LineGroup::remove() {
    if (mLineEngine->mLineGroups.erase(mId)) {
        mLineEngine->mGlobalDirty = true;
        return true;
    }
    return false;
}
