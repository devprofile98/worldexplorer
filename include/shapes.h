#ifndef WEBGPUTEST_SHAPE_H
#define WEBGPUTEST_SHAPE_H

#include "../webgpu/webgpu.h"
#include "binding_group.h"
#include "gpu_buffer.h"
#include "mesh.h"
#include "model.h"
#include "pipeline.h"

class Application;

class Cube : public BaseModel {
    public:
        Cube(Application* app);
        virtual void draw(Application* app, WGPURenderPassEncoder encoder) override;
        void userInterface() override;
        size_t getVertexCount() const override;

    private:
        // vertex indices
        Buffer mIndexDataBuffer = {};
        Buffer offset_buffer = {};
        // ? material
        Application* mApp;
};

class Plane : public Cube {
    public:
        Plane(Application* app);

    private:
        std::map<int, Mesh> mMeshes;
};

class Line : public BaseModel {
    public:
        Line(Application* app, glm::vec3 start, glm::vec3 end, float width, glm::vec3 color);

        virtual void draw(Application* app, WGPURenderPassEncoder encoder) override;

    private:
        Application* mApp;
        Buffer mIndexDataBuffer = {};
        /*float triangleVertexData[33];*/
        uint16_t mIndexData[6] = {0, 1, 2, 1, 2, 3};
        float triangleVertexData[44] = {
            1.0, 0.5, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  //
            6.0, 3.0, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  //
            6.1, 2.8, 4.0, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  //
        };
        /*    std::map<int, Mesh> mMeshes;*/
};
#endif  // WEBGPUTEST_SHAPE_H
