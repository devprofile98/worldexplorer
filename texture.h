#ifndef WEBGPUTEST_TEXTURE_H
#define WEBGPUTEST_TEXTURE_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "webgpu/webgpu.h"

enum class TextureDimension { TEX_1D, TEX_2D, TEX_3D };

class Texture {
    public:
        Texture(WGPUDevice wgpuDevice, uint32_t width, uint32_t height, TextureDimension dimension,
                WGPUTextureUsageFlags flags = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                WGPUTextureFormat textureFormat = WGPUTextureFormat_RGBA8Unorm, uint32_t extent = 1);
        Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path,
                WGPUTextureFormat textureFormat = WGPUTextureFormat_RGBA8Unorm);
        ~Texture();

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
        void uploadToGPU(WGPUQueue deviceQueue);
        bool isTransparent();

        // Remove the texture from the VRAM
        void Destroy();

        static std::vector<uint8_t> expandToRGBA(uint8_t* data, size_t height, size_t width);

    private:
        WGPUTexture mTexture;
        WGPUTextureView mTextureView = nullptr;
        WGPUTextureView mArrayTextureView = nullptr;
        WGPUTextureDescriptor mDescriptor;
        std::vector<uint8_t> mBufferData;
        bool mIsTextureAlive = false;  // Indicate whether the texure is still valid on VRAM or not
        bool mHasAlphaChannel = false;
};

#endif  // WEBGPUTEST_TEXTURE_H
