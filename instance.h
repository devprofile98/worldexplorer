
#ifndef WEBGPUTEST_INSTANCE_H
#define WEBGPUTEST_INSTANCE_H

#include <cstdint>
#include <vector>

#include "gpu_buffer.h"

class Application;

class InstanceManager {
    public:
        explicit InstanceManager(Application* app, size_t bufferSize, size_t maxInstancePerModel);

	size_t mBufferSize = 0;
        // Getter
        Buffer& getInstancingBuffer();

    private:
        Buffer mOffsetBuffer;
};

class Instance {
    public:
        explicit Instance(std::vector<glm::mat4>& instanceBuffer);
	
	//Getter
	size_t getInstanceCount();
	uint16_t getInstanceID();

        std::vector<glm::mat4> mInstanceBuffer;
	uint16_t mOffsetID = 0;
};

#endif  // WEBGPUTEST_INSTANCE_H
