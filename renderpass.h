
#ifndef WEBGPUTEST_ABSTRACT_RENDER_PASS_H
#define WEBGPUTEST_ABSTRACT_RENDER_PASS_H

#include <vector>

#include "binding_group.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"
#include "webgpu/wgpu.h"

class Application;

struct Scene {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        float farZ;
        float padding[3]{};
};

enum class LoadOp {
    Undefined = 0x0,
    Clear = 0x01,
    Load = 0x02,
};

enum class StoreOp {
    Undefined = 0x0,
    Store = 0x01,
    Discard = 0x02,
};

class ColorAttachment {
    public:
        ColorAttachment() {}
        ColorAttachment(WGPUTextureView target, WGPUTextureView resolve, WGPUColor clearColor,
                        StoreOp storeOp = StoreOp::Store, LoadOp loadOp = LoadOp::Load);

        // ifdef WEBGPU_BACKEND
        WGPURenderPassColorAttachment* get();

    private:
        WGPUTextureView mTarget = nullptr;
        WGPUTextureView mResolveTarget = nullptr;
        WGPUColor mClearColor;
        StoreOp mStoreOp = StoreOp::Store;
        LoadOp mLoadOp = LoadOp::Load;
        WGPURenderPassColorAttachment mAttachment;
};

class DepthStencilAttachment {
    public:
        DepthStencilAttachment() {}
        DepthStencilAttachment(WGPUTextureView target, StoreOp depthStoreOp = StoreOp::Store,
                               LoadOp depthLoadOp = LoadOp::Load, bool depthReadOnly = false,
                               StoreOp stencilStoreOp = StoreOp::Store, LoadOp stencilLoadOp = LoadOp::Load,
                               bool stencilReadOnly = true, float c = 1.0);
        WGPURenderPassDepthStencilAttachment* get();

    private:
        WGPURenderPassDepthStencilAttachment mAttachment;
};

class RenderPass {
    public:
        RenderPass();
        WGPURenderPassDescriptor* getRenderPassDescriptor();
        void setRenderPassDescriptor(WGPURenderPassDescriptor desc);
        Pipeline* getPipeline();
        RenderPass& setColorAttachment(const ColorAttachment& attachment);
        RenderPass& setDepthStencilAttachment(const DepthStencilAttachment& attachment);
        WGPURenderPassDescriptor* init();

        ColorAttachment mColorAttachment;
        DepthStencilAttachment mDepthStencilAttachment;

    private:
        // render pass
        WGPURenderPassDescriptor mRenderPassDesc;

        // bindings
        BindingGroup mBindingGroup;
        std::vector<WGPUBindGroupEntry> mBindingData;

        Texture* mRenderTarget;
        // buffers
        Buffer mSceneUniformBuffer;

        // scene
        std::vector<Scene> mScenes;

    protected:
        Pipeline* mRenderPipeline;
        virtual void createRenderPass(WGPUTextureFormat textureFormat) = 0;
};

#endif  // WEBGPUTEST_ABSTRACT_RENDER_PASS_H
