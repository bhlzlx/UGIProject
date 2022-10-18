#include "TextureUtility.h"
#include "VulkanDeclare.h"
#include "UGITypeMapping.h"
#include "UGITypes.h"
#include "UGIDeclare.h"
#include "Device.h"
#include "CommandQueue.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include <stb_image.h>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

namespace ugi {

	struct alignas(4) KtxHeader {
		alignas(4) uint8_t  idendifier[12];
		alignas(4) uint32_t	endianness;
		alignas(4) uint32_t	type;
		alignas(4) uint32_t	typeSize;
		alignas(4) uint32_t	format;
		alignas(4) uint32_t	internalFormat;
		alignas(4) uint32_t	baseInternalFormat;
		alignas(4) uint32_t	pixelWidth;
		alignas(4) uint32_t	pixelHeight;
		alignas(4) uint32_t	pixelDepth;
		alignas(4) uint32_t	arraySize;
		alignas(4) uint32_t	faceCount;
		alignas(4) uint32_t	mipLevelCount;
		alignas(4) uint32_t	bytesOfKeyValueData;
	};

	void TextureUtility::replaceTexture( Texture* texture, const ImageRegion* regions, const void* data, uint32_t dataLength, uint32_t* offsets, uint32_t regionCount  ) const {	
        auto commandBuffer = _transferQueue->createCommandBuffer(_device); 
        auto stagingBuffer = _device->createBuffer( ugi::BufferType::StagingBuffer, dataLength );
        commandBuffer->beginEncode(); {
            auto resCmdEncoder = commandBuffer->resourceCommandEncoder();
            void* mappingPtr = stagingBuffer->map(_device);
            memcpy( mappingPtr, data, dataLength);
            stagingBuffer->unmap(_device);
            resCmdEncoder->replaceImage( texture, stagingBuffer, regions, offsets, regionCount);
            resCmdEncoder->endEncode();
        }
        commandBuffer->endEncode();
		QueueSubmitInfo submitInfo {
			&commandBuffer,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, nullptr );
        _transferQueue->submitCommandBuffers(submitBatch);
        _transferQueue->waitIdle();
        //
        stagingBuffer->release(_device);
		_transferQueue->destroyCommandBuffer( _device, commandBuffer );
    }

	void TextureUtility::replaceTexture( 
		Texture* texture, 
		const ImageRegion* regions,
		Buffer* stagingBuffer,
		uint32_t* offsets,
		uint32_t regionCount
    ) const {
		auto commandBuffer = _transferQueue->createCommandBuffer(_device); 
        commandBuffer->beginEncode(); {
            auto resCmdEncoder = commandBuffer->resourceCommandEncoder();
            resCmdEncoder->replaceImage( texture, stagingBuffer, regions, offsets, regionCount);
            resCmdEncoder->endEncode();
        }
        commandBuffer->endEncode();
		QueueSubmitInfo submitInfo {
			&commandBuffer,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, nullptr );
        _transferQueue->submitCommandBuffers(submitBatch);
        _transferQueue->waitIdle();
		_transferQueue->destroyCommandBuffer( _device, commandBuffer );
	}

    Texture* TextureUtility::createTextureKTX( const void* data, uint32_t length ) const {
        const uint8_t * ptr = (const uint8_t *)data;
		const uint8_t * end = ptr + length;
        tex_desc_t desc;
        // read file header
		KtxHeader* header = (KtxHeader*)ptr;
		ptr += sizeof(KtxHeader);
		uint8_t FileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};
		{ // validate identifier 
            if (memcmp(header->idendifier, FileIdentifier, sizeof(FileIdentifier)) != 0) {
                return nullptr;
            }
            // For compressed textures, glType must equal 0.
            if (header->type != 0) {
                return nullptr;
            }
            //glTypeSize specifies the data type size that should be used whenendianness conversion is required for the texture data stored in thefile. If glType is not 0, this should be the size in bytes correspondingto glType. For texture data which does not depend on platform endianness,including compressed texture data, glTypeSize must equal 1.
            if (header->typeSize != 1) {
                return nullptr;
            }
            // For compressed textures, glFormat must equal 0.
            if (header->format != 0) {
                return nullptr;
            }

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
				desc.arrayLayers = header->faceCount * header->arraySize;
			}
			else {
				desc.type = TextureType::TextureCube;
				desc.arrayLayers = 1;
			}
			desc.depth = header->pixelDepth;
		}
		else if( header->pixelDepth >=1 ) {
			desc.type = TextureType::Texture3D;
		} else {
			if (header->arraySize >= 1) {
				desc.type = TextureType::Texture2DArray;
				desc.arrayLayers = header->arraySize;
				desc.depth = header->pixelDepth;
			} else {
				desc.type = TextureType::Texture2D;
				desc.arrayLayers = 1;
				desc.depth = 1;
			}
		}
		desc.width = header->pixelWidth;
		desc.height = header->pixelHeight;
        Texture* texture = _device->createTexture( desc, ugi::ResourceAccessType::ShaderRead );
		ptr += header->bytesOfKeyValueData;
		uint32_t bpp = 8;
		uint32_t pixelsize = bpp / 8;
		// should care about the alignment of the cube slice & mip slice
		// but reference to the ETC2 & EAC & `KTX format reference`, for 4x4 block compression type, the alignment should be zero
		// so we can ignore the alignment
		// read all mip level var loop!
		auto* contentStart = ptr;
		std::vector<uint32_t> offsets;
		std::vector<ImageRegion> regions;
		uint32_t pixelContentSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			offsets.push_back(pixelContentSize);
			uint32_t mipBytes = *(uint32_t*)(ptr);
			pixelContentSize += mipBytes;// *desc.depth;
			ptr += sizeof(mipBytes);
			ptr += mipBytes;// *desc.depth;
			//
			ImageRegion region;
			region.arrayCount = desc.arrayLayers;
			region.arrayIndex = 0;
			region.extent.depth = desc.depth;
			region.extent.width = desc.width >> mipLevel;
			region.extent.height = desc.height >> mipLevel;
			region.mipLevel = mipLevel;
			region.offset = {0, 0, 0};
			regions.push_back(region);
		}
		auto stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, pixelContentSize);
		auto mappedPtr = stagingBuffer->map(_device);
		ptr = contentStart;
		char * contentR = (char*)mappedPtr;
		for (uint32_t mipLevel = 0; mipLevel < header->mipLevelCount; ++mipLevel) {
			uint32_t mipBytes = *(uint32_t*)(ptr);
			ptr += sizeof(mipBytes);
			memcpy(contentR, ptr, mipBytes);
			ptr += mipBytes;
			contentR += mipBytes;
		}
		replaceTexture( texture, regions.data(), stagingBuffer, offsets.data(), (uint32_t)offsets.size() );
		_device->destroyBuffer(stagingBuffer);
		return texture;
    }

	Texture* TextureUtility::createTexturePNG( const void* data, uint32_t length ) const {
        int x; int y; int channels;
        auto pixel = stbi_load_from_memory( (stbi_uc*)data, length,&x, &y, &channels, 4 );
        tex_desc_t textureDescription;
        textureDescription.format = ugi::UGIFormat::RGBA8888_UNORM;
        textureDescription.width = x;
        textureDescription.height = y;
        textureDescription.depth = 1;
        textureDescription.mipmapLevel = 1;
        textureDescription.arrayLayers = 1;
        textureDescription.type = TextureType::Texture2D;
        auto texture = _device->createTexture(textureDescription);
		//
		ImageRegion region;
		region.arrayIndex = 0;
		region.arrayCount = 1;
		region.extent = { (uint32_t)x, (uint32_t)y, (uint32_t)1 };
		region.mipLevel = 0;
		region.offset = {};
		uint32_t offset = 0;
		//
		replaceTexture( texture, &region, pixel, x*y*4, &offset, 1 );
		stbi_image_free(pixel); // cleanup!!!
		//
		return texture;
	}

}