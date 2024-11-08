#include "binding_group.h"

#include "application.h"

BindingGroup::BindingGroup(/* args */) { mEntries.reserve(3); }

BindingGroup::~BindingGroup() {}

void BindingGroup::add(WGPUBindGroupLayoutEntry entry) { mEntries.push_back(entry); }

size_t BindingGroup::getEntryCount() const { return mEntries.size(); };

WGPUBindGroupLayoutEntry* BindingGroup::getEntryData() { return mEntries.data(); };

WGPUBindGroup& BindingGroup::getBindGroup() { return mBindGroup; }

void BindingGroup::create(Application* app, WGPUBindGroupDescriptor desc) {
    mBindGroupDesc = desc;
    mBindGroup = wgpuDeviceCreateBindGroup(app->getRendererResource().device, &desc);
};

WGPUBindGroupDescriptor& BindingGroup::getDescriptor() { return mBindGroupDesc; }