
#ifndef WORLD_EXPLORER_LINE_GROUP_H
#define WORLD_EXPLORER_LINE_GROUP_H

#include <cstdint>
#include <vector>
struct LineEngine;

class LineGroup {
    public:
        uint32_t mId;
        LineGroup& updateLines(const std::vector<glm::vec4>& newPoints);
        LineGroup& updateTransformation(const glm::mat4& trans);
        LineGroup& updateColor(const glm::vec3& color);
        LineGroup& updateVisibility(bool visibility);
        bool remove();
        bool isInitialized() const;

    private:
        bool mInitialized = false;
        LineEngine* mLineEngine;
        friend struct LineEngine;
};

#endif
