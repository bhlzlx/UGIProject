#include "SDFTextureTileManager.h"
#include <ugi/Device.h>
#include <ugi/Texture.h>
#include <ugi/Buffer.h>
#include <ugi/ResourceManager.h>
#include <hgl/assets/AssetsSource.h>
#include <ugi/commandBuffer/ResourceCommandEncoder.h>
#include <ugi/CommandBuffer.h>

namespace ugi {

    static const int32_t sourceFontSize = 72;
    static const int32_t searchDistance = sourceFontSize / 6;

    void SDFTextureTileManager::signedDistanceFieldImage2D(
        uint8_t* src, int32_t srcWid, int32_t srcHei,
        uint8_t* dst, int32_t dstWid, int32_t dstHei
    ) {
        SDFFlag* flagMap = _SDFFlags.data();
        float maxDistance = (float)searchDistance*sqrt(2.0f);
        // 像素类型划分
        // 如果像素是半透
        for( int32_t i = 0; i<srcHei; ++i) {
            for( int32_t j = 0; j<srcWid; ++j) {
                int32_t index = i*srcWid+j;
                flagMap[index].opaque = 0;  // 默认透明
                flagMap[index].inner = 0;   // 默认为外部
                if( src[index] != 0x0 ) {   // 值不为0证明为内部像素
                    flagMap[index].inner = 1;
                    if(src[index] == 0xff) {
                        flagMap[index].opaque = 1;
                    }
                }
            }
        }
        float ratioX = (float)srcWid / (float)dstWid;
        float ratioY = (float)srcHei / (float)dstHei;
        //
        for( int32_t i = 0; i<dstHei; ++i) {
            for( int32_t j = 0; j<dstWid; ++j) {

                int srcX = j * ratioX, srcY = i * ratioY;
                int srcMinX = srcX - searchDistance >= 0 ? srcX - searchDistance : 0;
                int srcMaxX = srcX + searchDistance >= srcWid ? srcWid-1 : srcX + searchDistance;
                int srcMinY = srcY - searchDistance >= 0 ? srcY - searchDistance : 0;
                int srcMaxY = srcY + searchDistance >= srcHei ? srcHei-1 : srcY + searchDistance;

                int srcIndex = srcX + srcY * srcWid;
                int dstIndex = j + i * dstWid;
                //
                const SDFFlag& flag = flagMap[srcX+srcWid*srcY];
                //
                float distance = maxDistance;
                if(flag.inner) {
                    if( flag.opaque ) { // 不透明的直接算距离
                        for( int xpos = srcMinX; xpos<=srcMaxX; ++xpos ) {
                            for( int ypos = srcMinY; ypos<=srcMaxY; ++ypos) {
                                SDFFlag tempFlag = flagMap[xpos+ypos*srcWid];
                                if(tempFlag.opaque == 0) { // 半透明
                                    float tempDistance = sqrt(pow(xpos-srcX,2) + pow(ypos-srcY,2));
                                    // 两个版本的透明像素距离处理
                                    // tempDistance += (src[xpos+ypos*srcWid] / 255.0f);
                                    tempDistance += ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                                    if(tempDistance<distance) {
                                        distance = tempDistance;
                                    }
                                }
                            }
                        }
                    } else { // 半透明的，这个特别处理一下
                        distance = ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                    }
                    dst[dstIndex] = ((distance / maxDistance)/2.f + 0.5) * 255;
                } else {
                    for( int xpos = srcMinX; xpos<=srcMaxX; ++xpos ) {
                        for( int ypos = srcMinY; ypos<=srcMaxY; ++ypos) {
                            SDFFlag tempFlag = flagMap[xpos+ypos*srcWid];
                            if(tempFlag.inner == 1) { // 半透明
                                float tempDistance = sqrt(pow(xpos-srcX,2) + pow(ypos-srcY,2));
                                // 两个版本的透明像素距离处理
                                // tempDistance += (1.0f - (src[xpos+ypos*srcWid] / 255.0f));
                                tempDistance -= ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                                if(tempDistance<distance) {
                                    distance = tempDistance;
                                }
                            }
                        }
                    }
                    dst[dstIndex] = (0.5f - (distance / maxDistance)/2.f)* 255;
                }
            }
        }
    }


    SDFTextureTileManager::SDFTextureTileManager()
        : _device( nullptr )
        , _texArray( nullptr )
        , _cellSize(1)
        , _position(0)
        , _glyphRecord {}
        , _freeTable {}
        , _fontTable {}
        , _DPI(96.0f)
    {
    }


    bool SDFTextureTileManager::initialize( Device* device, hgl::assets::AssetsSource* assetsSource, uint32_t cellSize, uint32_t texSize, uint32_t arrayLayer ) {
        _device = device;
        _assetsSource = assetsSource;
        if(_texArray) {
            return true; // 重复初始化？
        }
        TextureDescription texDesc;
        texDesc.type = TextureType::Texture2DArray;
        texDesc.arrayLayers = arrayLayer;
        texDesc.format = UGIFormat::R8_UNORM;
        texDesc.width = texDesc.height = texSize;
        texDesc.depth = 1;
        texDesc.mipmapLevel = 1;
        _texArray = _device->createTexture(texDesc);
        if(!_texArray) {
            return false;
        }
        _cellSize = cellSize;
        _col = texSize / cellSize;
        _row = texSize / cellSize;
        _layers = arrayLayer;
        _capacity = _col * _row * _layers;
        // 预分配 buffer 
        _rawBitmapBuffer.resize(256*256);         // 最大支持 96x96  我们按64号字体
        _SDFFlags.resize(256*256);
        _glyphInfoPool.reserve(_capacity);
        //
        _sdfBitmapBuffer.reserve( 32*32*32 );   // 预先准备 32 个字体的空间，防止频繁分配buffer

        _resourceManager = new ResourceManager(device);
        //
        {
            // 初始化一个字体（先写死，做个测试）
            auto inputStream = _assetsSource->Open(hgl::UTF8String("hwzhsong.ttf"));
            _fontTable.emplace_back();
            FontInfo& fontInfo = _fontTable.back();
            auto size = inputStream->GetSize();
            uint8_t* data = new uint8_t[size];
            inputStream->ReadFully(data, size);
            int error = stbtt_InitFont(&fontInfo.stbTtfInfo, data, 0);
            stbtt_GetFontBoundingBox(&fontInfo.stbTtfInfo, &fontInfo.x0, &fontInfo.y0, &fontInfo.x1, &fontInfo.y1);
            fontInfo.maxHeight = fontInfo.y1 - fontInfo.y0;
            stbtt_GetFontVMetrics(&fontInfo.stbTtfInfo, &fontInfo.ascent, &fontInfo.descent, &fontInfo.lineGap);
        }
        return true;
    }

    GlyphInfo* SDFTextureTileManager::allocateGlyph() {
        GlyphInfo* info = nullptr;
        if(_freeTable.size()) {
            info = _freeTable.front();
            _freeTable.pop();
        } else {
            if(_position < _capacity ) {
                _glyphInfoPool.emplace_back();
                info = &_glyphInfoPool.back();
                info->glyphIndex = _position;
                ++_position;
            }
        }
        return info;
    }

    GlyphInfo* SDFTextureTileManager::registGlyph( GlyphKey glyph ) {
        if(glyph.fontID >= _fontTable.size()) {
            return nullptr;
        }
        //
        float scale = 1.0f;
        float fontSize = sourceFontSize* _DPI / 72.0f;
        //
        FontInfo& fontInfo = _fontTable[glyph.fontID];

        scale = stbtt_ScaleForMappingEmToPixels(&fontInfo.stbTtfInfo, fontSize);
        //int baseline = (int)(fontInfo.ascent * scale);
        int advance, lsb, x0, y0, x1, y1;
        float shiftX = 0.0f; float shiftY = 0.0f;
        stbtt_GetCodepointHMetrics(&fontInfo.stbTtfInfo, glyph.charCode, &advance, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&fontInfo.stbTtfInfo, glyph.charCode, scale, scale, shiftX, shiftY, &x0, &y0, &x1, &y1);
        stbtt_MakeCodepointBitmapSubpixel(
            &fontInfo.stbTtfInfo,
            _rawBitmapBuffer.data(),
            x1 - x0,
            y1 - y0,
            x1 - x0,
            scale, scale,
            shiftX, shiftY,
            glyph.charCode
        );
        
        int bearingX = x0;
        int bearingY = -y0;
        int bitmapWidth = x1 - x0;
        int bitmapHeight = y1 - y0;
        int bitmapAdvance = advance * scale;
        //
        float ratio = (float)_cellSize / ((bitmapWidth>bitmapHeight) ? bitmapWidth: bitmapHeight);

        GlyphInfo* glyphInfo = allocateGlyph();
        glyphInfo->bitmapBearingX = bearingX * ratio;
        glyphInfo->bitmapBearingY = bearingY * ratio;
        glyphInfo->bitmapWidth = bitmapWidth * ratio;
        glyphInfo->bitmapHeight = bitmapHeight * ratio;
        glyphInfo->bitmapAdvance = bitmapAdvance * ratio;
        glyphInfo->SDFScale = ratio;
        /// 
        uint32_t row = ((uint32_t)glyphInfo->glyphIndex % (_row*_col)) / _row;
        uint32_t col = ((uint32_t)glyphInfo->glyphIndex % (_row*_col)) % _row;

        glyphInfo->bitmapOffsetX = (col * _cellSize);
        glyphInfo->bitmapOffsetY = (row * _cellSize);
        glyphInfo->texIndex = glyphInfo->bitmapLayerIndex;
        glyphInfo->bitmapLayerIndex = glyphInfo->bitmapLayerIndex / (_row*_col);

        float textureSize = _texArray->desc().width;
        glyphInfo->texWidth = (float)glyphInfo->bitmapWidth / textureSize;
        glyphInfo->texHeight = (float)glyphInfo->bitmapHeight / textureSize;
        //
        glyphInfo->texU = (col * _cellSize) / textureSize;
        glyphInfo->texV = (row * _cellSize) / textureSize;
        glyphInfo->texWidth = glyphInfo->bitmapWidth / textureSize;
        glyphInfo->texHeight = glyphInfo->bitmapHeight / textureSize;

        
        uint32_t destinationPixelCount = glyphInfo->bitmapWidth * glyphInfo->bitmapHeight;
        auto outputPosition = _sdfBitmapBuffer.size();
        _sdfBitmapBuffer.resize(outputPosition+destinationPixelCount);

        signedDistanceFieldImage2D( _rawBitmapBuffer.data(), bitmapWidth, bitmapHeight, &_sdfBitmapBuffer[outputPosition], glyphInfo->bitmapWidth, glyphInfo->bitmapHeight );

        TileItem tileItem;
        tileItem.bufferOffset = outputPosition;
        tileItem.bufferSize = destinationPixelCount;
        tileItem.glyphInfo = glyphInfo;
        _cachedTileItems.push_back(tileItem);

        return glyphInfo;
    }

    void SDFTextureTileManager::unregistGlyph( GlyphKey glyph ) {
        // do nothing
    }

    void SDFTextureTileManager::resourceTick( ResourceCommandEncoder* encoder ) {
        _resourceManager->tick();
        if(_cachedTileItems.size()) {
            size_t stagingBufferSize = this->_sdfBitmapBuffer.size();
            auto stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, stagingBufferSize );
            void* ptr = stagingBuffer->map(_device);
            memcpy(ptr, _sdfBitmapBuffer.data(), _sdfBitmapBuffer.size());
            stagingBuffer->unmap(_device);

            std::vector<uint32_t> offsets;
            std::vector<ImageRegion> regions;
            for( const auto& item : _cachedTileItems ) {
                ImageRegion region;
                region.mipLevel = 0;
                region.arrayIndex = item.glyphInfo->texIndex;
                region.offset = { item.glyphInfo->bitmapOffsetX, item.glyphInfo->bitmapOffsetY, 0 };
                region.extent = { item.glyphInfo->bitmapWidth, item.glyphInfo->bitmapHeight, 1 };
                regions.push_back(region);
                offsets.push_back(item.bufferOffset);
            }
            encoder->updateImage( _texArray, stagingBuffer, regions.data(), offsets.data(), regions.size() );
            //
            _resourceManager->trackResource(stagingBuffer);
            _cachedTileItems.clear();
        }
    }

    Texture* SDFTextureTileManager::texture() {
        return _texArray;
    }
}