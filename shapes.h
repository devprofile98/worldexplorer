#ifndef WEBGPUTEST_SHAPE_H
#define WEBGPUTEST_SHAPE_H

#include "binding_group.h"
#include "gpu_buffer.h"
#include "model.h"
#include "pipeline.h"
#include "webgpu/webgpu.h"
#include "mesh.h"

class Application;

class Cube: public BaseModel{
    public:
        Cube(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder, std::vector<WGPUBindGroupEntry>& bindingData) override;
	void userInterface() override;
	size_t getVertexCount() const override;

    private:
        // vertex indices
        Buffer mIndexDataBuffer = {};
	Buffer offset_buffer = {};
        // ? material
	Application* mApp;
};

class Plane: public Cube{
    public:
        Plane(Application* app);
    private:
	std::map<int, Mesh> mMeshes;

};

#endif  // WEBGPUTEST_SHAPE_H
