
#ifndef WORLD_EXPLORER_CORE_PARTICLE_SYSTEM_H
#define WORLD_EXPLORER_CORE_PARTICLE_SYSTEM_H

#include <functional>

#include "binding_group.h"
#include "utils.h"

class Application;
class NewRenderPass;

struct alignas(16) ParticleData {
        glm::mat4 transformation;
        glm::vec4 props;  // vec3 color and float life
        glm::vec2 uvoffset;
};

struct Particle {
        glm::vec3 position;
        glm::vec3 rotate;
        float speed;
        float life;
        float scale;  // uniform scale factor
        glm::vec2 uvoffset = glm::vec2{0.0};
};

class ParticleSystem;
using EmitterCallback = std::function<Particle(ParticleSystem*, const glm::vec3&)>;
using SimulationCallback =
    std::function<void(ParticleSystem*, float, std::vector<Particle>&, std::vector<ParticleData>&)>;

struct ParticleSystemSetting {
        ParticleSystemSetting(const glm::vec3& speedDirVector, EmitterCallback emitterCb, SimulationCallback simCb,
                              const glm::vec2& speedRange = glm::vec2{0.1, 0.2},
                              const glm::vec2& scaleRange = glm::vec2{0.008, 0.009}, float lifeTime = 0.4f,
                              float particleSize = 0.01, int ppf = 10);
        void setSpeedDiretcion(const glm::vec3& speedDir);
        glm::vec3& getSpeedDirection();

        glm::vec3 mExitVector{0.0f, 0.0f, -1.0f};
        glm::vec2 speedRange;
        glm::vec2 scaleRange;
        float mLifeTime;  // in seconds
        float mSize;
        int ParticlePerFrame;

        glm::vec2 uvoffset{0.0f};

        EmitterCallback spawnCallback;
        SimulationCallback simulationCallback;
};

class ParticleSystem {
    public:
        ParticleSystem(Application* app, const std::string& name, const ParticleSystemSetting& settings);

        void updateParticleSystem(float dt, bool simulate);
        void executePass();
        void draw(WGPURenderPassEncoder encoder);
        void setEmitterSocket(BoneSocket* emitterSocket);
        BoneSocket* getEmitterSocket();
        ParticleSystemSetting& getSettings();
        const std::string_view getName() const;
        void setActive(bool active);
        bool isActive() const;
        Application* mApp;

    private:
        void simulateParticles(float dt);
        size_t lastAddedParticle();

    private:
        std::string mName;
        bool mActive = true;

        size_t lastAddedIdx = 0;
        std::vector<ParticleData> mParticlesData;
        std::vector<Particle> mParticles;
        BoneSocket* mEmitterSocket = nullptr;
        ParticleSystemSetting mSettings;

        BindingGroup mBindGroup{};
        Pipeline* mPipeline = nullptr;
        VertexBufferLayout mVertexBufferLayout;
        NewRenderPass* mRenderPass = nullptr;
        std::vector<WGPUBindGroupEntry> mBindingData;
        Buffer mVertexBuffer;
        Buffer mVertexBuffer2;
        Buffer mInstancingBuffer;
};

class ParticleSystemsManager {
    public:
        void userInterface(Application* app);
        bool registerSystem(ParticleSystem* system);
        void run(float dt);

    private:
        std::vector<ParticleSystem*> mParticleSystems;
};

#endif  // !WORLD_EXPLORER_CORE_PARTICLE_SYSTEM_H
