#include "TextureDDS.h"
#include <ugi/device.h>
#include <ugi/texture.h>
#include <tinyddsloader/tinyddsloader.h>

namespace ugi {

    Texture* CreateTextureDDS(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, std::function<void(void*, CommandBuffer*)> &&onComplete) {
        tinyddsloader::DDSFile file;
        auto result = file.Load((const uint8_t*)data, dataLen);
        if (result != tinyddsloader::Success) {
            return nullptr;
        }
        tex_desc_t desc = {}; {
            auto dim = file.GetTextureDimension();
            switch (dim)
            {
            case tinyddsloader::DDSFile::TextureDimension::Unknown:
                return nullptr;
            case tinyddsloader::DDSFile::TextureDimension::Texture1D:
                desc.type = TextureType::Texture1D;
                break;
            case tinyddsloader::DDSFile::TextureDimension::Texture2D:
                if (file.IsCubemap()) {
                    desc.type = TextureType::TextureCube;
                    if (file.GetArraySize() != 6 ) 
                    {
                        desc.type = TextureType::TextureCubeArray;
                    }
                    else
                    {
                        desc.type = TextureType::TextureCube;
                    }
                }
                else
                {
                    if (file.GetArraySize() > 1) {
                        desc.type = TextureType::Texture2DArray;
                    }
                    else {
                        desc.type = TextureType::Texture2D;
                    }
                }
                break;
            case tinyddsloader::DDSFile::TextureDimension::Texture3D:
                desc.type = TextureType::Texture3D;
                break;
            default:
                break;
            }
            desc.width = file.GetWidth();
            desc.height = 1;
            desc.depth = 1;
            if (desc.type == TextureType::Texture2D || desc.type == TextureType::TextureCube  || desc.type == TextureType::Texture2DArray || desc.type == TextureType::TextureCubeArray ) {
                desc.height = file.GetHeight();
                desc.depth = file.GetArraySize();
            }
            else if (desc.type == TextureType::Texture3D) {
                desc.height = file.GetHeight();
                desc.depth = file.GetDepth();
            }
            //
            desc.mipmapLevel = file.GetMipCount();
            auto format = file.GetFormat();
            if (format == tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm) {
                desc.format = UGIFormat::BC3_LINEAR_RGBA;
            }
            else if (format == tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm) {
                desc.format = UGIFormat::BC1_LINEAR_RGBA;
            }
            else {
                return nullptr;
            }
        }
        auto texture = device->createTexture(desc, ResourceAccessType::ShaderRead);
        std::vector<VkDeviceSize> offsets;
        char* pixelContent = nullptr;
        VkDeviceSize pixelContentSize = 0;

        for (uint32_t mipIndex = 0; mipIndex < file.GetMipCount(); ++mipIndex)
        {
            uint32_t width = desc.width >> mipIndex;
            uint32_t height = desc.height >> mipIndex;
            if (desc.format == UGIFormat::BC1_LINEAR_RGBA || desc.format == UGIFormat::BC3_LINEAR_RGBA) {
                if (width <= 4) {
                    width = 4;
                }
                if (height <= 4) {
                    height = 4;
                }
            }
            uint32_t dataLength = width * height * file.GetBitsPerPixel(file.GetFormat()) / 8 * desc.depth;
            offsets.push_back(pixelContentSize);
            pixelContentSize += dataLength;
        }
        pixelContent = new char[pixelContentSize];
        //
        VkDeviceSize writeOffset = 0;
        for (uint32_t mipIndex = 0; mipIndex < file.GetMipCount(); ++mipIndex)
        {
            uint32_t width = desc.width >> mipIndex;
            uint32_t height = desc.height >> mipIndex;
            if (desc.format == UGIFormat::BC1_LINEAR_RGBA || desc.format == UGIFormat::BC3_LINEAR_RGBA) {
                if (width <= 4) {
                    width = 4;
                }
                if (height <= 4) {
                    height = 4;
                }
            }
            uint32_t dataLength = width * height * file.GetBitsPerPixel(file.GetFormat()) / 8;
            for (uint32_t arrayIndex = 0; arrayIndex < file.GetArraySize(); ++arrayIndex) {
                auto layerData = file.GetImageData(mipIndex, arrayIndex);
                memcpy( pixelContent + writeOffset, layerData->m_mem, dataLength );
                writeOffset += dataLength;
            }
        }
        std::vector<ImageRegion> regions;
        for(uint32_t i = 0; i<file.GetMipCount(); ++i) {
            ImageRegion region;
            region.mipLevel = 0;
            region.arrayIndex = 0;
            region.arrayCount = desc.depth;
            region.offset = {0, 0, 0};
            region.extent = {desc.width >> i, desc.height >> i, 1};
			regions.push_back(region);
        }
        texture->updateRegions(device, regions.data(), regions.size(), (uint8_t const*)pixelContent, pixelContentSize, offsets.data(), asyncLoadMgr, std::move(onComplete));
        delete[]pixelContent;
		return texture;
    }
}