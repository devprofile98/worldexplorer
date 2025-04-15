
#ifndef WGPU_UILIB_H
#define WGPU_UILIB_H

#include "pipeline.h"
class GUI {
    public:
        static void initialize(Application* app, std::vector<WGPUBindGroupLayout> bindGroupLayout);

    private:
        GUI() {};
	static inline GUI* mInstace = nullptr;
        Pipeline *mUIPipeline = nullptr;
};

#endif  // WGPU_UILIB_H
