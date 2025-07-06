
#ifndef WORLD_EXPLORER_EDTIOR_H
#define WORLD_EXPLORER_EDTIOR_H

#include "camera.h"
#include "glm/fwd.hpp"
#include "model.h"

struct GizmoElement {
        static inline BaseModel* center = nullptr;
        static inline BaseModel* z = nullptr;
        static inline BaseModel* x = nullptr;
        static inline BaseModel* y = nullptr;
	static inline bool mFirstTouch = true;

        static inline bool mIsLocked = false;
        static BaseModel* testSelection(Camera& camera, size_t width, size_t height,
                                        std::pair<size_t, size_t> mouseCoord);
        static void moveTo(const glm::vec3 position);
};

struct Editor {
        BaseModel* mSelectedElement = nullptr;
};

#endif  //! WORLD_EXPLORER_EDTIOR_H
