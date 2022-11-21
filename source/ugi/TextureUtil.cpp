#include "TextureUtil.h"
#ifdef _WIN32
#include <Windows.h>
#define GL_APIENTRY APIENTRY
#endif
#include <ugi/Device.h>
#include <opengl_registry/api/GLES3/gl32.h>
#include <opengl_registry/api/GLES2/gl2ext.h>
#include <ugi/UGITypeMapping.h>
#include <ugi/Texture.h>

#include <stb/stb_image.h>

namespace ugi {

#pragma pack( push, 1 )
	struct KtxHeader {
		uint8_t		idendifier[12];
		uint32_t	endianness;
		uint32_t	type;
		uint32_t	typeSize;
		uint32_t	format;
		uint32_t	internalFormat;
		uint32_t	baseInternalFormat;
		uint32_t	pixelWidth;
		uint32_t	pixelHeight;
		uint32_t	pixelDepth;
		uint32_t	arraySize;
		uint32_t	faceCount;
		uint32_t	mipLevelCount;
		uint32_t	bytesOfKeyValueData;
	};
#pragma pack( pop )

    Texture* CreateTextureKTX(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, std::function<void(void* res, CommandBuffer*)>&& callback) {
        const uint8_t * ptr = (const uint8_t *)data;
		// const uint8_t * end = ptr + dataLen;
		//
		KtxHeader* header = (KtxHeader*)ptr;
		ptr += sizeof(KtxHeader);
		tex_desc_t desc = {};
		uint8_t FileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};
		// validate identifier
		if (memcmp(header->idendifier, FileIdentifier, sizeof(FileIdentifier)) != 0) {
			return nullptr;
		}
		// For compressed textures, glType must equal 0.
		if (header->type != 0) {
			return nullptr;
		}
		//glTypeSize specifies the data type size that should be used whenendianness cjonversion is required for the texture data stored in thefile. If glType is not 0, this should be the size in bytes correspondingto glType. For texture data which does not depend on platform endianness,including compressed texture data, glTypeSize must equal 1.
		if (header->typeSize != 1) {
			return nullptr;
		}
		// For compressed textures, glFormat must equal 0.
		if (header->format != 0) {
			return nullptr;
		}
		// GL_COMPRESSED_RGBA8_ETC2_EAC GL_COMPRESSED_RG11_EAC
		if (header->internalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA8_ETC2_EAC
			desc.format = VkFormatToUGI(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
			// 
		} else if (header->internalFormat == GL_COMPRESSED_RG11_EAC && header->baseInternalFormat == GL_RG) {
			// GL_COMPRESSED_RG11_EAC
			// VK_FORMAT_EAC_R11G11_UNORM_BLOCK
			desc.format = VkFormatToUGI(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
		}
		else if (header->internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
			// VK_FORMAT_BC1_RGBA_UNORM_BLOCK
			desc.format = VkFormatToUGI(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
		}
		else if (header->internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT && header->baseInternalFormat == GL_RGBA) {
			// GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
			// VK_FORMAT_BC3_UNORM_BLOCK
			desc.format = VkFormatToUGI(VK_FORMAT_BC3_UNORM_BLOCK);
		}
		else {
			// unsupported texture format
			return nullptr;
		}
		desc.arrayLayers = 1;
		desc.depth = 1;
		desc.mipmapLevel = header->mipLevelCount;
		if (header->faceCount == 6) {
			if (header->arraySize >= 1) {
				desc.type = TextureType::TextureCubeArray;
				desc.arrayLayers = header->faceCount * header->arraySize;
			}
			else {
				desc.type = TextureType::TextureCube;
				desc.arrayLayers = 6;
			}
		}
		else if( header->pixelDepth >=1 ) {
			desc.type = TextureType::Texture3D;
			desc.depth = header->pixelDepth;
		}
		else {
			if (header->arraySize >= 1) {
				desc.type = TextureType::Texture2DArray;
				desc.arrayLayers = header->arraySize;
			}
			else
			{
				desc.type = TextureType::Texture2D;
			}
			
		}
		desc.width = header->pixelWidth;
		desc.height = header->pixelHeight;
		auto texture = device->createTexture(desc, ResourceAccessType::ShaderRead);
		ptr += header->bytesOfKeyValueData;
		//
		// uint32_t bpp = 8;
		// uint32_t pixelsize = bpp / 8;

		// should care about the alignment of the cube slice & mip slice
		// but reference to the ETC2 & EAC & `KTX format reference`, for 4x4 block compression type, the alignment should be zero
		// so we can ignore the alignment
		// read all mip level var loop!
		auto* contentStart = ptr;
		std::vector< VkDeviceSize > offsets;
		uint32_t pixelContentSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			offsets.push_back(pixelContentSize);
			uint32_t mipBytes = *(uint32_t*)(ptr);
			pixelContentSize += mipBytes;// *desc.depth;
			ptr += sizeof(mipBytes);
			ptr += mipBytes;// *desc.depth;
		}
		char * content = new char[pixelContentSize];
		ptr = contentStart;
		char * contentR = content;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			uint32_t mipBytes = *(uint32_t*)(ptr);
			ptr += sizeof(mipBytes);
			memcpy(contentR, ptr, mipBytes);
			ptr += mipBytes;
			contentR += mipBytes;
		}
		std::vector<ImageRegion> regions;
		for(uint32_t i = 0; i<header->mipLevelCount; ++i) {
			ImageRegion region;
			region.arrayIndex = 0;
			region.arrayCount = header->arraySize;
			region.mipLevel = i;
			region.offset = {};
			region.extent = { desc.width >> i, desc.height >> i, desc.depth};
			regions.push_back(region);
		}
		texture->updateRegions(device, regions.data(), regions.size(), (uint8_t const*)content, pixelContentSize, offsets.data(), asyncLoadMgr, std::move(callback));
		delete[]content;
		return texture;
    }

    Texture* CreateTexturePNG(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, AsyncLoadCallback&& callback) {
        int x; int y; int channels;
        auto pixel = stbi_load_from_memory( (stbi_uc*)data, dataLen,&x, &y, &channels, 4 );
        tex_desc_t textureDescription;
		//
		uint32_t mipmapLevel = 1; {
			int mipmapRefSize = x > y ? x : y;
			while(mipmapRefSize!=1) {
				++mipmapLevel;
				mipmapRefSize = mipmapRefSize >> 1;
			}
		}
        textureDescription.format = ugi::UGIFormat::RGBA8888_UNORM;
        textureDescription.width = x;
        textureDescription.height = y;
        textureDescription.depth = 1;
        textureDescription.mipmapLevel = mipmapLevel;
        textureDescription.arrayLayers = 1;
        textureDescription.type = TextureType::Texture2D;
        auto texture = device->createTexture(textureDescription);
		//
		ImageRegion region;
		region.arrayIndex = 0;
		region.arrayCount = 1;
		region.extent = { (uint32_t)x, (uint32_t)y, (uint32_t)1 };
		region.mipLevel = 0;
		region.offset = {};
		uint64_t offset = 0;
		texture->updateRegions(device, &region, 1, pixel, x*y*4, &offset, asyncLoadMgr, std::move(callback));
		stbi_image_free(pixel); // cleanup!!!
		return texture;
	}

}