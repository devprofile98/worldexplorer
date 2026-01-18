#include "wgpu_utils.h"

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <cassert>
#include <cstring>
#include <format>
#include <iostream>
#include <vector>

WGPUStringView createStringView(const std::string &str) {
    // std::cout << "String receive is " << str << " with size " << str.size() << std::endl;
    return WGPUStringView{str.c_str(), str.size() + 1};
}

WGPUStringView createStringViewC(const char *str) {
    // std::cout << "String receive is " << str << " with size " << strlen(str) << std::endl;
    return WGPUStringView{str, WGPU_STRLEN};
}
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

    auto on_request_ended_callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
                                        void *userData, void *) {
        auto &user_data = *reinterpret_cast<UserData *>(userData);
        if (status == WGPURequestAdapterStatus_Success) {
            user_data.ended = true;
            user_data.adapter = adapter;
            std::cout << std::format("Successfuly acquire WGPUAdapter!\n");
        } else {
            std::cout << std::format("Failed in requesting adapter, message ({})\n", message.data);
        }
    };

    WGPURequestAdapterCallbackInfo cb_options{};
    cb_options.nextInChain = nullptr;
    cb_options.callback = on_request_ended_callback;
    cb_options.userdata1 = (void *)&user_data;

    wgpuInstanceRequestAdapter(instance, &options, cb_options);
    assert(user_data.ended);
    return user_data.adapter;
}

void func(const WGPUDevice *device, WGPUDeviceLostReason reason, WGPUStringView message, void *, void *) {
    (void)device;
    std::cout << std::format("Device lost, reason is: {}\n", reason == WGPUDeviceLostReason_Destroyed);
    if (message.data != nullptr) {
        std::cout << std::format("({})", message.data);
    }
};

// request webgpu device
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPULimits limits) {
    (void)adapter;
    WGPUDeviceDescriptor descriptor = {};
    std::vector<WGPUFeatureName> features = {WGPUFeatureName_TimestampQuery};
    descriptor.nextInChain = nullptr;
    descriptor.label = {"My Device", sizeof("My Device")};  // anything works here, that's your
    descriptor.requiredFeatures = features.data();
    descriptor.requiredFeatureCount = features.size();  // we do not require any
    /*descriptor.requiredFeatures;*/
    descriptor.requiredLimits = &limits;  // we do not require any
    descriptor.defaultQueue.nextInChain = nullptr;
    descriptor.defaultQueue.label = {"The default queue", 18};
    descriptor.deviceLostCallbackInfo.callback = func;
    // descriptor.deviceLostCallback = [](WGPUDeviceLostReason reason, const char *message, void *) {
    //     std::cout << std::format("Device lost, reason is: {}\n", reason == WGPUDeviceLostReason_Destroyed);
    //     if (message != nullptr) {
    //         std::cout << std::format("({})", message);
    //     }
    // };

    struct UserData {
            bool ended = false;
            WGPUDevice device = nullptr;
    };
    WGPURequestDeviceCallbackInfo cb_info{};

    cb_info.nextInChain = nullptr;
    cb_info.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata,
                          void *) {
        auto &user_data = *reinterpret_cast<UserData *>(userdata);
        if (status == WGPURequestDeviceStatus_Success) {
            user_data.device = device;
            std::cout << "device request completed succesfuly!" << std::endl;
        } else {
            user_data.device = nullptr;
            std::cout << std::format("Wgpu failed to acquire the device, message is {}\n", message.data);
        }
    };
    UserData user_data{};
    cb_info.userdata1 = &user_data;

    wgpuAdapterRequestDevice(adapter, &descriptor, cb_info);
    return user_data.device;
}

WGPUQueue getDeviceQueue(WGPUDevice device) {
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    // auto on_queue_done = [](WGPUQueueWorkDoneStatus status, void *) {
    //     std::cout << "Queue Submitted work done status : " << (status == WGPUQueueWorkDoneStatus_Success) <<
    //     std::endl;
    // };
    // wgpuQueueOnSubmittedWorkDone(queue, on_queue_done, nullptr);
    return queue;
}

void inspectProperties(WGPUAdapter adapter) {
    WGPUAdapterInfo info;
    info.nextInChain = nullptr;
    wgpuAdapterGetInfo(adapter, &info);

    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << info.vendorID << std::endl;
    if (info.vendor.length) {
        std::cout << " - vendorName: " << info.vendor.data << std::endl;
    }
    if (info.architecture.length) {
        std::cout << " - architecture: " << info.architecture.data << std::endl;
    }
    std::cout << " - deviceID: " << info.deviceID << std::endl;
    if (info.description.length) {
        // std::cout << " - driverDescription: " << info.description.data << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << info.adapterType << std::endl;
    std::cout << " - backendType: 0x" << info.backendType << std::endl;
    std::cout << std::dec;  // Restore decimal numbers
}

void inspectFeatures(WGPUAdapter adapter) {
    // std::vector<WGPUFeatureName> features;
    WGPUSupportedFeatures features;
    wgpuAdapterGetFeatures(adapter, &features);
    // features.resize(feature_count);

    // wgpuAdapterEnumerateFeatures(adapter, features.data());
    std::cout << "Adapter features: " << std::endl;
    std::cout << std::hex;
    for (size_t i = 0; i < features.featureCount; i++) {
        std::cout << " - 0x" << features.features[i] << std::endl;
    }
    std::cout << std::dec;
}

void inspectAdapter(WGPUAdapter adapter) {
    WGPULimits supported_limits;
    supported_limits.nextInChain = nullptr;

    bool success = wgpuAdapterGetLimits(adapter, &supported_limits);
    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supported_limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supported_limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supported_limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supported_limits.maxTextureArrayLayers << std::endl;
        std::cout << " - Adapter.MaxVertexAttribute: " << supported_limits.maxVertexAttributes << std::endl;
    }
}

void inspectDevice(WGPUDevice device) {
    WGPULimits supported_limits{};
    supported_limits.nextInChain = nullptr;
    std::cout << "Device limits:" << std::endl;

    bool success = wgpuDeviceGetLimits(device, &supported_limits);
    if (success) {
        std::cout << " - Device.MaxVertexAttribute: " << supported_limits.maxVertexAttributes << std::endl;
        std::cout << " - Device.maxStorageBufferBindingSize: " << supported_limits.maxStorageBufferBindingSize
                  << std::endl;
    }
}
