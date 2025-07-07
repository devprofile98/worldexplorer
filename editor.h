
#ifndef WORLD_EXPLORER_EDTIOR_H
#define WORLD_EXPLORER_EDTIOR_H

#include "camera.h"
#include "glm/fwd.hpp"
#include "input_manager.h"
#include "model.h"

class Application;

struct GizmoElement : public MouseMoveListener, public MouseButtonListener {
        static inline BaseModel* center = nullptr;
        static inline BaseModel* z = nullptr;
        static inline BaseModel* x = nullptr;
        static inline BaseModel* y = nullptr;
        static inline bool mFirstTouch = true;
        static inline BaseModel* mSelectedPart = nullptr;

        static inline bool mIsLocked = false;
        static BaseModel* testSelection(Camera& camera, size_t width, size_t height,
                                        std::pair<size_t, size_t> mouseCoord);
        static void moveTo(const glm::vec3 position);
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
};

struct Editor {
        BaseModel* mSelectedElement = nullptr;
        GizmoElement gizmo;
};

class Screen : public MouseMoveListener, public MouseButtonListener {
    public:
        static void initialize(Application* app);
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
        static Screen& instance();

    private:
        static inline Application* mApp;
        // static Screen mInstance;
        Screen();
};

#endif  //! WORLD_EXPLORER_EDTIOR_H
