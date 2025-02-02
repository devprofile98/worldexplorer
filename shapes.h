#ifndef WEBGPUTEST_SHAPE_H
#define WEBGPUTEST_SHAPE_H

#include "binding_group.h"
#include "gpu_buffer.h"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"

class Application;

class Cube: public Transform, public Drawable{
    public:
        Cube(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData);

    private:
	ObjectInfo mObjectInfo;
        // vertex data
        Buffer mVertexDataBuffer = {};

        // vertex indices
        Buffer mIndexDataBuffer = {};

	/*Buffer mUniformBuffer = {};*/

        // ? material
	
	Application* mApp;
};

#endif  // WEBGPUTEST_SHAPE_H
