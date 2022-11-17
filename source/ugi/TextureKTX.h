#pragma once

#include <ugi/UGIDeclare.h>
#include <cstdint>
#include <functional>

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

	struct KtxLoader {
	private:
        uint32_t m_format;
		// VkFormat m_format;
		uint32_t m_width;
		uint32_t m_heigth;
		uint32_t m_depth;
		uint32_t m_mipLevelCount;
		uint32_t m_arrayLength;
		//
		Texture* m_texture;
	public:
        static Texture* CreateTexture(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, std::function<void(CommandBuffer*)> &&callback);
	};


    Texture* CreateTextureKTX(Device* device, uint8_t const* data, uint32_t dataLen);

}