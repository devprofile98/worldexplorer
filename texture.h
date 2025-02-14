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
        Texture(WGPUDevice wgpuDevice, uint32_t width, uint32_t height, TextureDimension dimension);
        Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path);
        ~Texture();

        // access function
        WGPUTexture getTexture();
        WGPUTextureView getTextureView();

        Texture& setBufferData(std::vector<uint8_t>& data);
        WGPUTextureView createView();
        void uploadToGPU(WGPUQueue deviceQueue);
	bool isTransparent();

        // Remove the texture from the VRAM
        void Destroy();

        static std::vector<uint8_t> expandToRGBA(uint8_t* data, size_t height, size_t width);

    private:
        WGPUTexture mTexture;
        WGPUTextureView mTextureView = nullptr;
        WGPUTextureDescriptor mDescriptor;
        std::vector<uint8_t> mBufferData;
        bool mIsTextureAlive = false;  // Indicate whether the texure is still valid on VRAM or not
	bool mHasAlphaChannel = false;
};

#endif  // WEBGPUTEST_TEXTURE_H
