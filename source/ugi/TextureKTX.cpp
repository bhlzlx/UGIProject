#include "TextureKTX.h"
#include <ugi/Device.h>
#include <opengl_registry/api/GLES2/gl2ext.h>
#include <opengl_registry/api/GLES3/gl32.h>
#include <ugi/UGITypeMapping.h>

namespace ugi {

    Texture* CreateTextureKTX(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, std::function<void(CommandBuffer*)>&& callback) {
        const uint8_t * ptr = (const uint8_t *)data;
		const uint8_t * end = ptr + dataLen;
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
		desc.mipmapLevel = header->mipLevelCount;
		if (header->faceCount == 6) {
			if (header->arraySize >= 1) {
				desc.type = TextureType::TextureCubeArray;
				desc.depth = header->faceCount * header->arraySize;
			}
			else {
				desc.type = TextureType::TextureCube;
				desc.depth = 6;
			}
		}
		else if( header->pixelDepth >=1 ) {
			desc.type = TextureType::Texture3D;
		}
		else {
			if (header->arraySize >= 1) {
				desc.type = TextureType::Texture2DArray;
				desc.depth = header->arraySize;
			}
			else
			{
				desc.type = TextureType::Texture2D;
				desc.depth = 1;
			}
			
		}
		desc.width = header->pixelWidth;
		desc.height = header->pixelHeight;
		TextureVk* texture = TextureVk::createTexture(_context, VK_NULL_HANDLE, VK_NULL_HANDLE, desc, TextureUsageSampled | TextureUsageTransferDestination);
		//
		ptr += header->bytesOfKeyValueData;
		//
		uint32_t bpp = 8;
		uint32_t pixelsize = bpp / 8;

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

			/*for (uint32_t arrayIndex = 0; arrayIndex < desc.depth; ++arrayIndex) {
				
			}*/
		}
		TextureRegion region;
		region.baseLayer = 0;
		region.mipLevel = 0;
		region.offset.x = region.offset.y = region.offset.z = 0;
		region.size.width = desc.width;
		region.size.height = desc.height;
		region.size.depth = desc.depth;

		BufferImageUpload upload;
		upload.baseMipRegion = region;
		upload.data = content;
		upload.length = pixelContentSize;
		upload.mipCount = (uint32_t)offsets.size();
		for (size_t i = 0; i < offsets.size(); ++i) {
			upload.mipDataOffsets[i] = offsets[i];
		}
		_context->getUploadQueue()->uploadTexture(texture, upload);
		delete[]content;
		return texture;
	}
    }

}