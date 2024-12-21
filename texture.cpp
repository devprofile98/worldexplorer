#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

WGPUTextureView Texture::createView() {
    WGPUTextureView tmp = nullptr;
    if (mIsTextureAlive) {
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.aspect = WGPUTextureAspect_All;
        texture_view_desc.baseArrayLayer = 0;
        texture_view_desc.arrayLayerCount = 1;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = mDescriptor.mipLevelCount;
        texture_view_desc.dimension = WGPUTextureViewDimension_2D;
        texture_view_desc.format = mDescriptor.format;

        tmp = wgpuTextureCreateView(mTexture, &texture_view_desc);
    }
    return tmp;
}

Texture::Texture(WGPUDevice wgpuDevice, uint32_t width, uint32_t height, TextureDimension dimension) {
    mDescriptor = {};
    mDescriptor.nextInChain = nullptr;
    mDescriptor.dimension = static_cast<WGPUTextureDimension>(dimension);
    mDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    mDescriptor.mipLevelCount = 8;
    mDescriptor.sampleCount = 1;
    mDescriptor.size = {width, height, 1};
    mDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    mDescriptor.viewFormatCount = 0;
    mDescriptor.viewFormats = nullptr;
    mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);

    // createView(mDescriptor);

    mIsTextureAlive = true;
}

Texture::Texture(WGPUDevice wgpuDevice, const std::filesystem::path& path) {
    std::cout << "Loading texture " << path.c_str() << std::endl;
    int width, height, channels;
    unsigned char* pixel_data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    // If data is null, loading failed.
    if (nullptr == pixel_data) return;

    mDescriptor = {};
    mDescriptor.dimension = static_cast<WGPUTextureDimension>(TextureDimension::TEX_2D);
    // by convention for bmp, png and jpg file. Be careful with other formats.
    mDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
    mDescriptor.mipLevelCount = 11;
    mDescriptor.sampleCount = 1;
    mDescriptor.size = {(unsigned int)width, (unsigned int)height, 1};
    mDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    mDescriptor.viewFormatCount = 0;
    mDescriptor.viewFormats = nullptr;
    mTexture = wgpuDeviceCreateTexture(wgpuDevice, &mDescriptor);

    size_t image_byte_size = (width * height * std::max(channels + 1, 1));
    mBufferData.reserve(image_byte_size);
    mBufferData.resize(image_byte_size);
    memset(mBufferData.data() + (width * height * 3), 100, width * height * 1 - 2000000);
    for (size_t cnt = 0, dst_cnt = 0;
         cnt < (size_t)width * height * channels || dst_cnt < (size_t)width * height * (channels + 1);
         cnt += 3, dst_cnt += 4) {
        mBufferData[dst_cnt] = pixel_data[cnt];
        mBufferData[dst_cnt + 1] = pixel_data[cnt + 1];
        mBufferData[dst_cnt + 2] = pixel_data[cnt + 2];
        mBufferData[dst_cnt + 3] = 1;
    }
    // memcpy(mBufferData.data(), pixel_data, width * height * 3);
    std::cout << "buffer size is _______________________ " << channels << " " << mBufferData.size() << std::endl;
    // mBufferData = std::vector<uint8_t>{pixel_data, pixel_data + image_byte_size};

    // Upload data to the GPU texture (to be implemented!)
    // writeMipMaps(device, texture, textureDesc.size, textureDesc.mipLevelCount, pixelData);

    // Use the width, height, channels and data variables here
    // [...]

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

void Texture::uploadToGPU(WGPUQueue deviceQueue) {
    WGPUImageCopyTexture destination;
    destination.texture = mTexture;
    destination.mipLevel = 0;
    destination.origin = {0, 0, 0};              // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All;  // only relevant for depth/Stencil textures

    // Arguments telling how the C++ side pixel memory is laid out
    WGPUTextureDataLayout source;
    source.offset = 0;

    // upload the level zero: aka original texture
    source.bytesPerRow = 4 * mDescriptor.size.width;

    source.rowsPerImage = mDescriptor.size.height;
    std::cout << "Data is .... " << mDescriptor.size.width << " " << mBufferData.size() << std::endl;
    wgpuQueueWriteTexture(deviceQueue, &destination, mBufferData.data(), mBufferData.size(), &source,
                          &mDescriptor.size);

    // generate and upload other levels
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
                // p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                p[3] = 1.0;
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