#ifndef WEBGPUTEST_BINDING_GROUP_H
#define WEBGPUTEST_BINDING_GROUP_H

#include <vector>

#include "webgpu/webgpu.h"

class Application;

class BindingGroup {
    public:
        BindingGroup(/* args */);
        ~BindingGroup();

        void add(WGPUBindGroupLayoutEntry entry);

        // --- Getter functions
        size_t getEntryCount() const;
        WGPUBindGroupLayoutEntry* getEntryData();
        WGPUBindGroup& getBindGroup();
        WGPUBindGroupDescriptor& getDescriptor();

        std::vector<WGPUBindGroupLayoutEntry> mEntries = {};
        void create(Application* app, WGPUBindGroupDescriptor desc);

    private:
        WGPUBindGroup mBindGroup;
        WGPUBindGroupDescriptor mBindGroupDesc;
};

#endif  // WEBGPUTEST_BINDING_GROUP_H
