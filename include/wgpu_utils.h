#ifndef WEBGPU_UTILS_H
#define WEBGPU_UTILS_H

#include <string>

#include "../webgpu/webgpu.h"
#include "../webgpu/wgpu.h"

// request webgpu adapter
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPUSurface surface);

// request webgpu device
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPULimits limits);

WGPUQueue getDeviceQueue(WGPUDevice device);

void inspectProperties(WGPUAdapter adapter);

void inspectFeatures(WGPUAdapter adapter);

void inspectAdapter(WGPUAdapter adapter);
void inspectDevice(WGPUDevice device);

WGPUStringView createStringView(const std::string& str);

#endif  // WEBGPU_UTILS_H
