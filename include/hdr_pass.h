
#ifndef WORLD_EXPLORER_CORE_HDR_PASS_H
#define WORLD_EXPLORER_CORE_HDR_PASS_H

#include "binding_group.h"
#include "utils.h"

class Application;
class Pipeline;
class NewRenderPass;

struct alignas(16) HDRSettings {
        uint32_t isActive = 1;
        float exposure = 1.0;
};

class HDRPipeline {
    public:
        HDRPipeline(Application* app, const std::string& name);

        void executePass();
        void onResize();
        void userInterface();
        const std::string_view getName() const;

        Application* mApp;

    private:
        std::string mName;
        bool mActive = true;
        HDRSettings mHDRSettings;
        BindingGroup mBindGroup{};
        BindingGroup mSettingsBG{};
        Pipeline* mPipeline = nullptr;
        VertexBufferLayout mVertexBufferLayout;
        NewRenderPass* mRenderPass = nullptr;
        std::vector<WGPUBindGroupEntry> mBindingData;
        std::vector<WGPUBindGroupEntry> mSettingsBGData;
        Buffer mVertexBuffer;
        Buffer mSettingsBuffer;
};

#endif  //! WORLD_EXPLORER_CORE_HDR_PASS_H
