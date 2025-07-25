#include "wgpu_utils.h"

#include <cassert>
#include <format>
#include <iostream>
#include <vector>

#include "webgpu.h"
#include "wgpu.h"

// request webgpu adapter
WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPUSurface surface) {
    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.compatibleSurface = surface;

    struct UserData {
            bool ended = false;
            WGPUAdapter adapter = nullptr;
    };
    UserData user_data;

    auto on_request_ended_callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char *message,
                                        void *userData) {
        auto &user_data = *reinterpret_cast<UserData *>(userData);
        if (status == WGPURequestAdapterStatus_Success) {
            user_data.ended = true;
            user_data.adapter = adapter;
            std::cout << std::format("Successfuly acquire WGPUAdapter!\n");
        } else {
            std::cout << std::format("Failed in requesting adapter, message ({})\n", message);
        }
    };

    wgpuInstanceRequestAdapter(instance, &options, on_request_ended_callback, &user_data);
    assert(user_data.ended);
    return user_data.adapter;
}

// request webgpu device
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPURequiredLimits limits) {
    WGPUDeviceDescriptor descriptor = {};
    /*std::vector<WGPUFeatureName> features = {WGPUNativeFeature_VertexWritableStorage};*/
    descriptor.nextInChain = nullptr;
    descriptor.label = "My Device";       // anything works here, that's your
    descriptor.requiredFeatureCount = 0;  // we do not require any
    /*descriptor.requiredFeatures;*/
    descriptor.requiredLimits = &limits;  // we do not require any
    descriptor.defaultQueue.nextInChain = nullptr;
    descriptor.defaultQueue.label = "The default queue";
    descriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, const char *message, void *) {
        std::cout << std::format("Device lost, reason is: {}\n", reason == WGPUDeviceLostReason_Destroyed);
        if (message != nullptr) {
            std::cout << std::format("({})", message);
        }
    };

    struct UserData {
            bool ended = false;
            WGPUDevice device = nullptr;
    };

    auto on_request_ended_callback = [](WGPURequestDeviceStatus status, WGPUDevice device, char const *message,
                                        void *userdata) {
        auto &user_data = *reinterpret_cast<UserData *>(userdata);
        if (status == WGPURequestDeviceStatus_Success) {
            user_data.device = device;
            std::cout << "device request completed succesfuly!" << std::endl;
        } else {
            user_data.device = nullptr;
            std::cout << std::format("Wgpu failed to acquire the device, message is {}\n", message);
        }
    };
    UserData user_data;
    wgpuAdapterRequestDevice(adapter, &descriptor, on_request_ended_callback, &user_data);
    return user_data.device;
}

WGPUQueue getDeviceQueue(WGPUDevice device) {
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    auto on_queue_done = [](WGPUQueueWorkDoneStatus status, void *) {
        std::cout << "Queue Submitted work done status : " << (status == WGPUQueueWorkDoneStatus_Success) << std::endl;
    };
    wgpuQueueOnSubmittedWorkDone(queue, on_queue_done, nullptr);
    return queue;
}

void inspectProperties(WGPUAdapter adapter) {
    WGPUAdapterInfo info;
    info.nextInChain = nullptr;
    wgpuAdapterGetInfo(adapter, &info);

    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << info.vendorID << std::endl;
    if (info.vendor) {
        std::cout << " - vendorName: " << info.vendor << std::endl;
    }
    if (info.architecture) {
        std::cout << " - architecture: " << info.architecture << std::endl;
    }
    std::cout << " - deviceID: " << info.deviceID << std::endl;
    if (info.description) {
        std::cout << " - driverDescription: " << info.description << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << info.adapterType << std::endl;
    std::cout << " - backendType: 0x" << info.backendType << std::endl;
    std::cout << std::dec;  // Restore decimal numbers
}

void inspectFeatures(WGPUAdapter adapter) {
    std::vector<WGPUFeatureName> features;
    size_t feature_count = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    features.resize(feature_count);

    wgpuAdapterEnumerateFeatures(adapter, features.data());
    std::cout << "Adapter features: " << std::endl;
    std::cout << std::hex;
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;
}

void inspectAdapter(WGPUAdapter adapter) {
    WGPUSupportedLimits supported_limits;
    supported_limits.nextInChain = nullptr;

    bool success = wgpuAdapterGetLimits(adapter, &supported_limits);
    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supported_limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supported_limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supported_limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supported_limits.limits.maxTextureArrayLayers << std::endl;
        std::cout << " - Adapter.MaxVertexAttribute: " << supported_limits.limits.maxVertexAttributes << std::endl;
    }
}

void inspectDevice(WGPUDevice device) {
    WGPUSupportedLimits supported_limits;
    supported_limits.nextInChain = nullptr;
    std::cout << "Device limits:" << std::endl;

    bool success = wgpuDeviceGetLimits(device, &supported_limits);
    if (success) {
        std::cout << " - Device.MaxVertexAttribute: " << supported_limits.limits.maxVertexAttributes << std::endl;
	 std::cout << " - Device.maxStorageBufferBindingSize: " << supported_limits.limits.maxStorageBufferBindingSize << std::endl;
	
    }
}
