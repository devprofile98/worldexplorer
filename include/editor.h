
#ifndef WORLD_EXPLORER_EDTIOR_H
#define WORLD_EXPLORER_EDTIOR_H

#include <variant>

#include "camera.h"
#include "glm/fwd.hpp"
#include "input_manager.h"
#include "instance.h"
#include "model.h"
#include "physics.h"
#include "point_light.h"
#include "utils.h"

class Application;

enum GizmoMode {
    GizmoModeTranslation = 0,
    GizmoModeScale,
    GizmoModeRotation,
};

enum KeyboardKey {
    KeyboardKey_NONE = 0,
    KeyboardKey_LSHIFT = 1,
    KeyboardKey_RSHIFT = 2,
    KeyboardKey_LCTRL = 3,
    KeyboardKey_RCTRL = 4,
};

struct GizmoElement : public MouseMoveListener, public MouseButtonListener {
        static inline BaseModel* center = nullptr;
        static inline BaseModel* z = nullptr;
        static inline BaseModel* x = nullptr;
        static inline BaseModel* y = nullptr;
        static inline bool mFirstTouch = true;
        static inline glm::vec2 mStartingCoordinate = glm::vec2{0.0f};
        static inline BaseModel* mSelectedPart = nullptr;
        static inline KeyboardKey mKeyModifier = KeyboardKey_NONE;

        static inline bool mIsLocked = false;
        static inline GizmoMode mMode = GizmoModeTranslation;
        static BaseModel* testSelection(Camera& camera, size_t width, size_t height,
                                        std::pair<size_t, size_t> mouseCoord);
        static void moveTo(const glm::vec3 position);
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
};

struct Editor {
        BaseModel* mSelectedElement = nullptr;
        void showBoneAt(const glm::mat4& transformation);
        static inline BaseModel* BoneIndicator = nullptr;
        GizmoElement gizmo;
        std::variant<std::monostate, BaseModel*, physics::BoxCollider*, DebugBox*, Light*, SingleInstance>
            mSelectedObject;

        bool mEditorActive = true;
};

class Screen : public MouseMoveListener,
               public MouseButtonListener,
               public MouseScrollListener,
               public KeyboardListener {
    public:
        static void initialize(Application* app);
        void onMouseMove(MouseEvent event) override;
        void onMouseClick(MouseEvent event) override;
        void onMouseScroll(MouseEvent event) override;
        void onKey(KeyEvent event) override;
        static Screen& instance();

    private:
        static inline Application* mApp;
        // static Screen mInstance;
        Screen();
};

#endif  //! WORLD_EXPLORER_EDTIOR_H
