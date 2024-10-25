#ifndef WEBGPUTEST_BINDING_GROUP_H
#define WEBGPUTEST_BINDING_GROUP_H

#include <vector>

#include "webgpu/webgpu.h"

class BindingGroup {
    public:
        BindingGroup(/* args */);
        ~BindingGroup();

        void add(WGPUBindGroupLayoutEntry entry);

        // --- Getter functions
        size_t getEntryCount() const;
        WGPUBindGroupLayoutEntry* getEntryData();

        std::vector<WGPUBindGroupLayoutEntry> mEntries = {};

    private:
};

#endif  // WEBGPUTEST_BINDING_GROUP_H
