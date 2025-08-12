#include "texture.h"

#include <webgpu/webgpu.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>

#include "glm/exponential.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

WGPUTextureView Texture::createView() {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_All;
        texture_view_desc.baseArrayLayer = 0;
        texture_view_desc.arrayLayerCount = 1;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.format = mDescriptor.format;
        mTextureView = wgpuTextureCreateView(mTexture, &texture_view_desc);
        return mTextureView;
    }
    std::cout << "failed here " << mIsTextureAlive << std::endl;
    return nullptr;
}
WGPUTextureView Texture::createViewDepthStencil(uint32_t base, uint32_t count) {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_All;
        texture_view_desc.baseArrayLayer = base;
        texture_view_desc.arrayLayerCount = count;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.format = mDescriptor.format;

        mTextureView = wgpuTextureCreateView(mTexture, &texture_view_desc);
        return mTextureView;
    }
    std::cout << "failed here " << mIsTextureAlive << std::endl;
    return nullptr;
}

WGPUTextureView Texture::createViewDepthOnly(uint32_t base, uint32_t count) {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_DepthOnly;
        texture_view_desc.baseArrayLayer = base;
        texture_view_desc.arrayLayerCount = count;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.format = mDescriptor.format;
        if (texture_view_desc.format == WGPUTextureFormat_Depth24PlusStencil8) {
            texture_view_desc.format = WGPUTextureFormat_Depth24Plus;  // Correct depth-only format
        }
        return wgpuTextureCreateView(mTexture, &texture_view_desc);
        /*return mTextureView;*/
    }
    std::cout << "failed here " << mIsTextureAlive << std::endl;
    return nullptr;
}
WGPUTextureView Texture::createViewDepthOnly2(uint32_t base, uint32_t count) {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_DepthOnly;
        texture_view_desc.baseArrayLayer = base;
        texture_view_desc.arrayLayerCount = count;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.format = mDescriptor.format;
        return wgpuTextureCreateView(mTexture, &texture_view_desc);
    }
    std::cout << "failed here " << mIsTextureAlive << std::endl;
    return nullptr;
}

WGPUTextureView Texture::createViewArray(uint32_t base, uint32_t count) {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_All;
        texture_view_desc.baseArrayLayer = base;
        texture_view_desc.arrayLayerCount = count;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2DArray;
        texture_view_desc.format = mDescriptor.format;
        mArrayTextureView = wgpuTextureCreateView(mTexture, &texture_view_desc);
        return mArrayTextureView;
    }
    std::cout << "failed here " << mIsTextureAlive << std::endl;
    return nullptr;
}

Texture::Texture(WGPUDevice wgpuDevice, uint32_t width, uint32_t height, TextureDimension dimension,
                 WGPUTextureUsage flags, WGPUTextureFormat textureFormat, uint32_t extent, std::string textureLabel) {
    mLabel = textureLabel;
    mDescriptor = {};
    mDescriptor.label = {mLabel.c_str(), mLabel.size()};
    mDescriptor.nextInChain = nullptr;
    mDescriptor.dimension = static_cast<WGPUTextureDimension>(dimension);
    mDescriptor.format = textureFormat;
    mDescriptor.mipLevelCount = 1;
    mDescriptor.sampleCount = 1;
    mDescriptor.size = {width, height, extent};
    mDescriptor.usage = flags;
    mDescriptor.viewFormatCount = 0;
    mDescriptor.viewFormats = nullptr;
    mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);

    mIsTextureAlive = true;
}

Texture::Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path, WGPUTextureFormat textureFormat) {
    int width, height, channels;
    width = height = channels = 0;
    unsigned char* pixel_data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

    if (nullptr == pixel_data) {
        std::cout << "failed to find file at " << path.string().c_str() << std::endl;
        return;
    };

    mDescriptor = {};
    mDescriptor.dimension = static_cast<WGPUTextureDimension>(TextureDimension::TEX_2D);
    mDescriptor.label = WGPUStringView{path.string().c_str(), path.string().size()};
    // by convention for bmp, png and jpg file. Be careful with other formats.
    mDescriptor.format = textureFormat;
    mDescriptor.mipLevelCount = glm::log2((float)width);
    mDescriptor.sampleCount = 1;
    mDescriptor.size = {(unsigned int)width, (unsigned int)height, 1};
    mDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    mDescriptor.viewFormatCount = 0;
    mDescriptor.viewFormats = nullptr;

    mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);

    size_t image_byte_size = (width * height * 4);
    mBufferData.reserve(image_byte_size);
    mBufferData.resize(image_byte_size);
    mHasAlphaChannel = (channels == 4);
    // Copy pixel data into mBufferData
    for (size_t i = 0, j = 0; i < (size_t)width * height * channels; i += channels, j += 4) {
        mBufferData[j] = pixel_data[i];                                    // Red
        mBufferData[j + 1] = pixel_data[i + 1];                            // Green
        mBufferData[j + 2] = pixel_data[i + 2];                            // Blue
        mBufferData[j + 3] = (channels == 4) ? pixel_data[i + 3] : 255.0;  // Alpha (default to 255 if no alpha channel)
    }

    stbi_image_free(pixel_data);
    mIsTextureAlive = true;
}

Texture::~Texture() {
    // if (mTexture != nullptr) {
    //     wgpuTextureDestroy(mTexture);
    // }
    wgpuTextureRelease(mTexture);
}

WGPUTexture Texture::getTexture() { return mTexture; }

Texture& Texture::setBufferData(std::vector<uint8_t>& data) {
    mBufferData = data;
    return *this;
}

bool Texture::isTransparent() { return mHasAlphaChannel; }

void Texture::uploadToGPU(WGPUQueue deviceQueue) {
    WGPUTexelCopyTextureInfo destination;
    destination.texture = mTexture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};              // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All;  // only relevant for depth/Stencil textures

    // Arguments telling how the C++ side pixel memory is laid out
    WGPUTexelCopyBufferLayout source{};
    source.offset = 0;

    // upload the level zero: aka original texture
    source.bytesPerRow = 4 * mDescriptor.size.width;

    source.rowsPerImage = mDescriptor.size.height;

    wgpuQueueWriteTexture(deviceQueue, &destination, mBufferData.data(), mBufferData.size(), &source,
                          &mDescriptor.size);

    // generate and upload other levels
    std::cout << mDescriptor.size.width << "*****************" << mDescriptor.size.height << std::endl;
    WGPUExtent3D mip_level_size = mDescriptor.size;
    // WGPUExtent3D prev_mip_level_size;
    std::vector<uint8_t> previous_level_pixels = mBufferData;
    WGPUExtent3D prev_mip_level_size = mDescriptor.size;
    for (uint32_t level = 1; level < mDescriptor.mipLevelCount; ++level) {
        prev_mip_level_size = mip_level_size;
        mip_level_size.height /= 2;
        mip_level_size.width /= 2;
        std::vector<uint8_t> pixels(4 * mip_level_size.width * mip_level_size.height);

        for (uint32_t i = 0; i < mip_level_size.width; ++i) {
            for (uint32_t j = 0; j < mip_level_size.height; ++j) {
                uint8_t* p = &pixels[4 * (j * mip_level_size.width + i)];

                // Get the corresponding 4 pixels from the previous level
                uint8_t* p00 = &previous_level_pixels[4 * ((2 * j + 0) * prev_mip_level_size.width + (2 * i + 0))];
                uint8_t* p01 = &previous_level_pixels[4 * ((2 * j + 0) * prev_mip_level_size.width + (2 * i + 1))];
                uint8_t* p10 = &previous_level_pixels[4 * ((2 * j + 1) * prev_mip_level_size.width + (2 * i + 0))];
                uint8_t* p11 = &previous_level_pixels[4 * ((2 * j + 1) * prev_mip_level_size.width + (2 * i + 1))];
                // Average
                p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                /*p[3] = 1.0;*/
            }
        }

        destination.mipLevel = level;
        source.bytesPerRow = 4 * mip_level_size.width;
        source.rowsPerImage = mip_level_size.height;
        wgpuQueueWriteTexture(deviceQueue, &destination, pixels.data(), pixels.size(), &source, &mip_level_size);
        previous_level_pixels = std::move(pixels);
    }
}

void Texture::Destroy() {
    // If the Texture is removed from VRAM before, then ignore
    if (mTexture != nullptr && mIsTextureAlive == true) {
        wgpuTextureDestroy(mTexture);
        mIsTextureAlive = false;
    }
}

WGPUTextureView Texture::getTextureView() { return mTextureView; }
WGPUTextureView Texture::getTextureViewArray() { return mArrayTextureView; };

std::vector<uint8_t> Texture::expandToRGBA(uint8_t* data, size_t height, size_t width) {
    if (!data) {
        std::cout << "expandToRGBA: Data for Texture provided is null\n";
    }
    size_t image_byte_size = height * width * 4;
    std::vector<uint8_t> buffer_data;
    buffer_data.reserve(image_byte_size);
    buffer_data.resize(image_byte_size);
    // memset(mBufferData.data() + (width * height * 3), 100, width * height * 1 - 2000000);
    for (size_t cnt = 0, dst_cnt = 0; cnt < (size_t)width * height * 3 || dst_cnt < (size_t)width * height * 4;
         cnt += 3, dst_cnt += 4) {
        buffer_data[dst_cnt] = data[cnt];
        buffer_data[dst_cnt + 1] = data[cnt + 1];
        buffer_data[dst_cnt + 2] = data[cnt + 2];
        buffer_data[dst_cnt + 3] = 1;
    }

    return buffer_data;
}
