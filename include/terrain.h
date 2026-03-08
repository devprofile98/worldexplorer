
#ifndef WORLD_EXPLORER_APP_TERRAIN_H
#define WORLD_EXPLORER_APP_TERRAIN_H

#include "linegroup.h"
#include "model.h"
#include "model_registery.h"

class Application;

class TerrainModel : public Model {
    public:
        TerrainModel(Application* app);
        Model& uploadToGPU(Application* app) override;
        Model& load(std::string name, Application* app, const std::filesystem::path& path,
                    WGPUBindGroupLayout layout) override;

        void update(Application* app, float dt, float physicSimulating = true) override;
        void getCustomBindGroup(Application* app, WGPURenderPassEncoder encoder, Mesh& mesh) override;
        Pipeline* getPipeline(Application* app) override;

        void drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node);
        void drawHirarchy(Application* app, WGPURenderPassEncoder encoder) override;

        LineGroup wireFrame;
};

class Cube : public Model {
    public:
        Cube(Application* app);
        Model& uploadToGPU(Application* app) override;
        Model& load(std::string name, Application* app, const std::filesystem::path& path,
                    WGPUBindGroupLayout layout) override;

        void update(Application* app, float dt, float physicSimulating = true) override;

        void drawGraph(Application* app, WGPURenderPassEncoder encoder, Node* node);
        void drawHirarchy(Application* app, WGPURenderPassEncoder encoder) override;
        LineGroup wireFrame;
};

#endif  // !WORLD_EXPLORER_CORE_RENDERERRESOURCE_H
