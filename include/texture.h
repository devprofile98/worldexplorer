#ifndef WEBGPUTEST_TEXTURE_H
#define WEBGPUTEST_TEXTURE_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../webgpu/webgpu.h"

enum class TextureDimension { TEX_UNDEFINED, TEX_1D, TEX_2D, TEX_3D };

class Texture {
    public:
        Texture(WGPUDevice wgpuDevice, uint32_t width, uint32_t height, TextureDimension dimension,
                WGPUTextureUsage flags = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                WGPUTextureFormat textureFormat = WGPUTextureFormat_RGBA8Unorm, uint32_t extent = 1,
                std::string textureLabel = "texture label");

        Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path,
                WGPUTextureFormat textureFormat = WGPUTextureFormat_RGBA8Unorm, uint32_t extent = 1,
                size_t mipLevels = 0);

        Texture(WGPUDevice wgpuDevice, std::vector<std::filesystem::path> paths,
                WGPUTextureFormat textureFormat = WGPUTextureFormat_RGBA8Unorm, uint32_t extent = 1);
        ~Texture();
        float ff;

        // access function
        WGPUTexture getTexture();
        WGPUTextureView getTextureView();
        WGPUTextureView getTextureViewArray();

        Texture& setBufferData(std::vector<uint8_t>& data);
        WGPUTextureView createView();
        WGPUTextureView createViewDepthOnly(uint32_t base = 0, uint32_t count = 1);
        WGPUTextureView createViewDepthStencil(uint32_t base = 0, uint32_t count = 1);
        WGPUTextureView createViewDepthOnly2(uint32_t base = 0, uint32_t count = 1);
        WGPUTextureView createViewArray(uint32_t base, uint32_t count);
        WGPUTextureView createViewCube(uint32_t base, uint32_t count);
        void writeBaseTexture(const std::filesystem::path& path, uint32_t extent = 1);
        void uploadToGPU(WGPUQueue deviceQueue);
        bool isTransparent();
        bool isValid() const;

        // Remove the texture from the VRAM
        void Destroy();

        static std::vector<uint8_t> expandToRGBA(uint8_t* data, size_t height, size_t width);

    private:
        std::string mLabel;
        WGPUTexture mTexture;
        WGPUTextureView mTextureView = nullptr;
        WGPUTextureView mArrayTextureView = nullptr;
        WGPUTextureDescriptor mDescriptor;
        std::vector<std::vector<uint8_t>> mBufferData;
        bool mIsTextureAlive = false;  // Indicate whether the texure is still valid on VRAM or not
        bool mHasAlphaChannel = false;
};

class TextureLoader {
    public:
        struct LoadRequest {
                std::string path;
                std::promise<std::shared_ptr<Texture>> promise;
                WGPUQueue queue;  // for GPU upload after CPU work
                std::shared_ptr<Texture> baseTexture;
                std::function<void(LoadRequest* req)> callback;
                void* userData = nullptr;
        };

        TextureLoader(size_t numThreads = std::thread::hardware_concurrency() - 1);
        ~TextureLoader();
        std::future<std::shared_ptr<Texture>> loadAsync(const std::string& path, WGPUQueue queue,
                                                        std::shared_ptr<Texture> baseTexture,
                                                        std::function<void(LoadRequest*)> cb, void* userData = nullptr);
        WGPUDevice device;

    private:
        std::vector<std::thread> workers;
        std::queue<LoadRequest> requests;
        std::mutex queueMutex;
        std::condition_variable cv;
        bool shouldTerminate = false;
};

template <typename K, typename V>
class Registery {
    public:
        Registery() {}
        void addToRegistery(const K& key, std::shared_ptr<V> value) { mRegistery[key] = value; }
        std::shared_ptr<V> get(const std::string& key) {
            if (mRegistery.contains(key)) {
                return mRegistery[key];
            }
            return nullptr;
        }
        std::unordered_map<K, std::shared_ptr<V>> list() const { return mRegistery; }

        TextureLoader mLoader;

    private:
        std::unordered_map<K, std::shared_ptr<V>> mRegistery;
};

#endif  // WEBGPUTEST_TEXTURE_H
