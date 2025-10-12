// renderer_resource.h (New file, minimal includes)

#ifndef WORLD_EXPLORER_CORE_RENDERERRESOURCE_H
#define WORLD_EXPLORER_CORE_RENDERERRESOURCE_H

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

/*
 * A struct to gather primitives for our renderer
 * Note: Pipelines and bindgroups are not part of this struct
 *       they could be included in some pipeline struct for each
 *       models that defines its own pipeline and render pass
 */
struct RendererResource {
        WGPUQueue queue;
        WGPUDevice device;
        WGPUSurface surface;
        GLFWwindow* window;
        WGPUCommandEncoder commandEncoder;
};
#endif  // !WORLD_EXPLORER_CORE_RENDERERRESOURCE_H
