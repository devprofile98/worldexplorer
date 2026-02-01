#include "texture.h"

#include <webgpu/webgpu.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <string>

#include "glm/exponential.hpp"
#include "profiling.h"

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

WGPUTextureView Texture::createViewCube(uint32_t base, uint32_t count) {
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_descriptor = {};
        texture_view_descriptor.nextInChain = nullptr;
        texture_view_descriptor.format = WGPUTextureFormat_RGBA8Unorm;
        texture_view_descriptor.dimension = WGPUTextureViewDimension_Cube;
        texture_view_descriptor.baseMipLevel = 0;
        texture_view_descriptor.mipLevelCount = 1;
        texture_view_descriptor.baseArrayLayer = base;
        texture_view_descriptor.arrayLayerCount = count;
        mTextureView = wgpuTextureCreateView(mTexture, &texture_view_descriptor);
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
    mBufferData.reserve(extent);
    mBufferData.resize(extent);
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

void Texture::writeBaseTexture(const std::filesystem::path& path, uint32_t extent) {
    int width, height, channels;
    width = height = channels = 0;
    unsigned char* pixel_data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (nullptr == pixel_data) {
        std::cout << "failed to find file at " << path.string().c_str() << std::endl;
        return;
    };
    size_t image_byte_size = (width * height * 4);
    mBufferData.reserve(extent);
    mBufferData.resize(extent);

    mBufferData[0].reserve(image_byte_size);
    mBufferData[0].resize(image_byte_size);

    mHasAlphaChannel = (channels == 4);
    // Copy pixel data into mBufferData
    for (size_t i = 0, j = 0; i < (size_t)width * height * channels; i += channels, j += 4) {
        mBufferData[0][j] = pixel_data[i];          // Red
        mBufferData[0][j + 1] = pixel_data[i + 1];  // Green
        mBufferData[0][j + 2] = pixel_data[i + 2];  // Blue
        mBufferData[0][j + 3] =
            (channels == 4) ? pixel_data[i + 3] : 255.0;  // Alpha (default to 255 if no alpha channel)
    }

    stbi_image_free(pixel_data);
}

Texture::Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path, WGPUTextureFormat textureFormat,
                 uint32_t extent, size_t mipLevels) {
    int width, height, channels;
    auto ret = stbi_info(path.string().c_str(), &width, &height, &channels);

    if (ret == 0) {
        std::cout << "Failure to load texture at" << path << std::endl;
        mIsTextureAlive = false;
        return;
    }

    mDescriptor = {};
    mDescriptor.dimension = static_cast<WGPUTextureDimension>(TextureDimension::TEX_2D);
    std::string path_str = path.string();
    mDescriptor.label = WGPUStringView{path_str.c_str(), path_str.size()};
    mDescriptor.format = textureFormat;
    mDescriptor.mipLevelCount = mipLevels == 0 ? glm::log2((float)width) : mipLevels;
    mDescriptor.sampleCount = 1;
    mDescriptor.size = {(uint32_t)width, (uint32_t)height, extent};
    mDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    mDescriptor.viewFormatCount = 0;
    mDescriptor.viewFormats = nullptr;

    std::cout << "11111111111111111111111111\n";
    mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);
    std::cout << "22222222222222222222222\n";
    mIsTextureAlive = true;
}

Texture::Texture(WGPUDevice wgpuDevice, std::vector<std::filesystem::path> paths, WGPUTextureFormat textureFormat,
                 uint32_t extent) {
    mBufferData.reserve(extent);
    mBufferData.resize(extent);

    ZoneScopedNC("Loading base texture for ", 0xFF00FF);

    for (size_t k = 0; k < extent; ++k) {
        auto& path = paths[k];
        int width, height, channels;
        width = height = channels = 0;
        unsigned char* pixel_data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);

        if (nullptr == pixel_data) {
            std::cout << "failed to find file at " << path.string().c_str() << std::endl;
            return;
        };

        mDescriptor = {};
        mDescriptor.dimension = static_cast<WGPUTextureDimension>(TextureDimension::TEX_2D);
        std::string path_str = path.string();
        mDescriptor.label = WGPUStringView{path_str.c_str(), path_str.size()};
        mDescriptor.format = textureFormat;
        mDescriptor.mipLevelCount = glm::log2((float)width);
        mDescriptor.sampleCount = 1;
        mDescriptor.size = {(uint32_t)width, (uint32_t)height, extent};
        mDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        mDescriptor.viewFormatCount = 0;
        mDescriptor.viewFormats = nullptr;

        mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);

        size_t image_byte_size = (width * height * 4);

        mBufferData[k].reserve(image_byte_size);
        mBufferData[k].resize(image_byte_size);

        mHasAlphaChannel = (channels == 4);
        // Copy pixel data into mBufferData
        for (size_t i = 0, j = 0; i < (size_t)width * height * channels; i += channels, j += 4) {
            // std::cout << image_byte_size << " " << i << " " << j << '\n';
            mBufferData[k][j] = pixel_data[i];          // Red
            mBufferData[k][j + 1] = pixel_data[i + 1];  // Green
            mBufferData[k][j + 2] = pixel_data[i + 2];  // Blue
            mBufferData[k][j + 3] =
                mHasAlphaChannel ? pixel_data[i + 3] : 255.0;  // Alpha (default to 255 if no alpha channel)
        }

        stbi_image_free(pixel_data);
        mIsTextureAlive = true;
    }
}

Texture::~Texture() {
    // if (mTexture != nullptr) {
    //     wgpuTextureDestroy(mTexture);
    // }
    wgpuTextureRelease(mTexture);
}

WGPUTexture Texture::getTexture() { return mTexture; }

Texture& Texture::setBufferData(std::vector<uint8_t>& data) {
    mBufferData[0] = data;
    return *this;
}

bool Texture::isTransparent() { return mHasAlphaChannel; }

void generateMipMaps(WGPUTexelCopyBufferLayout& source, WGPUTexelCopyTextureInfo& destination,
                     WGPUTextureDescriptor& descriptor, std::vector<std::vector<uint8_t>>& buffer, uint32_t layer,
                     WGPUQueue queue) {
    // generate and upload other levels
    WGPUExtent3D mip_level_size = descriptor.size;
    // WGPUExtent3D prev_mip_level_size;
    std::vector<uint8_t> previous_level_pixels = buffer[layer];
    WGPUExtent3D prev_mip_level_size = descriptor.size;
    prev_mip_level_size.depthOrArrayLayers = 1;
    for (uint32_t level = 1; level < descriptor.mipLevelCount; ++level) {
        prev_mip_level_size = mip_level_size;
        prev_mip_level_size.depthOrArrayLayers = 1;
        mip_level_size.height /= 2;
        mip_level_size.width /= 2;
        mip_level_size.depthOrArrayLayers = 1;
        std::vector<uint8_t> pixels(4 * mip_level_size.width * mip_level_size.height);

        for (uint32_t i = 0; i < mip_level_size.width; ++i) {
            for (uint32_t j = 0; j < mip_level_size.height; ++j) {
                uint8_t* p = &pixels[4 * (j * mip_level_size.width + i)];
                // std::cout << "2Pixel buffer size: " << pixels.size() << " " << 4 * (j * mip_level_size.width + i)
                //           << std::endl;  // forces materialization
                // Get the corresponding 4 pixels from the previous level
                uint8_t* p00 = &previous_level_pixels[4 * ((2 * j + 0) * prev_mip_level_size.width + (2 * i + 0))];
                uint8_t* p01 = &previous_level_pixels[4 * ((2 * j + 0) * prev_mip_level_size.width + (2 * i + 1))];
                uint8_t* p10 = &previous_level_pixels[4 * ((2 * j + 1) * prev_mip_level_size.width + (2 * i + 0))];
                uint8_t* p11 = &previous_level_pixels[4 * ((2 * j + 1) * prev_mip_level_size.width + (2 * i + 1))];
                // Average
                for (int c = 0; c < 4; ++c) {
                    p[c] = (p00[c] + p01[c] + p10[c] + p11[c] + 2) >> 2;
                }
                // p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                // p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                // p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                // p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
            }
        }

        destination.mipLevel = level;
        source.bytesPerRow = 4 * mip_level_size.width;
        source.rowsPerImage = mip_level_size.height;
        wgpuQueueWriteTexture(queue, &destination, pixels.data(), pixels.size(), &source, &mip_level_size);
        previous_level_pixels = std::move(pixels);
    }
}
bool Texture::isValid() const { return mIsTextureAlive; }

void Texture::uploadToGPU(WGPUQueue deviceQueue) {
    if (!mIsTextureAlive) return;
    for (uint32_t layer = 0; layer < mDescriptor.size.depthOrArrayLayers; ++layer) {
        WGPUTexelCopyTextureInfo destination;
        destination.texture = mTexture;
        destination.mipLevel = 0;
        destination.origin = {0, 0, layer};          // equivalent of the offset argument of Queue::writeBuffer
        destination.aspect = WGPUTextureAspect_All;  // only relevant for depth/Stencil textures

        WGPUTexelCopyBufferLayout source{};
        source.offset = 0;
        // upload the level zero: aka original texture
        source.bytesPerRow = 4 * mDescriptor.size.width;
        source.rowsPerImage = mDescriptor.size.height;
        auto tmp_size = WGPUExtent3D{mDescriptor.size.width, mDescriptor.size.height, 1};

        wgpuQueueWriteTexture(deviceQueue, &destination, mBufferData[layer].data(), mBufferData[layer].size(), &source,
                              &tmp_size);

        generateMipMaps(source, destination, mDescriptor, mBufferData, layer, deviceQueue);
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

TextureLoader::TextureLoader(size_t numThreads) {
    std::cout << "Number of hardware concurrency is " << numThreads << std::endl;
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                LoadRequest request;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    cv.wait(lock, [this] { return !requests.empty() || shouldTerminate; });
                    if (shouldTerminate && requests.empty()) return;
                    request = std::move(requests.front());
                    requests.pop();
                }

                // Heavy work: read file + generate mipmaps on CPU
                // request.baseTexture->writeBaseTexture(request.path);
                //
                // request.baseTexture->uploadToGPU(request.queue);
                //
                // request.promise.set_value(request.baseTexture);
                request.callback(&request);
            }
        });
    }
}

TextureLoader::~TextureLoader() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        shouldTerminate = true;
    }
    cv.notify_all();
    for (auto& w : workers) w.join();
}

std::future<std::shared_ptr<Texture>> TextureLoader::loadAsync(const std::string& path, WGPUQueue queue,
                                                               std::shared_ptr<Texture> baseTexture,
                                                               std::function<void(LoadRequest*)> cb, void* userData) {
    LoadRequest req{path, {}, queue, baseTexture, cb, userData};
    std::future<std::shared_ptr<Texture>> future = req.promise.get_future();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        requests.push(std::move(req));
    }
    cv.notify_one();

    return future;
}
