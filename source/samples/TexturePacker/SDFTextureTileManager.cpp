﻿#include "SDFTextureTileManager.h"
#include <ugi/device.h>
#include <ugi/texture.h>
#include <ugi/buffer.h>
#include <ugi/flight_cycle_invoker.h>    
#include <ugi/command_encoder/resource_cmd_encoder.h>
#include <ugi/command_buffer.h>

namespace ugi {

    void SDFTextureTileManager::signedDistanceFieldImage2D(
        uint8_t* src, int32_t srcWid, int32_t srcHei,
        uint8_t* dst, int32_t dstWid, int32_t dstHei
    ) {
        SDFFlag* flagMap = _SDFFlags.data();
        float maxDistance = (float)_searchDistance*sqrt(2.0f);
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
                int srcMinX = srcX - _searchDistance >= 0 ? srcX - _searchDistance : 0;
                int srcMaxX = srcX + _searchDistance >= srcWid ? srcWid-1 : srcX + _searchDistance;
                int srcMinY = srcY - _searchDistance >= 0 ? srcY - _searchDistance : 0;
                int srcMaxY = srcY + _searchDistance >= srcHei ? srcHei-1 : srcY + _searchDistance;

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


    bool SDFTextureTileManager::initialize( Device* device, hgl::assets::AssetsSource* assetsSource, uint32_t cellSize, uint32_t texSize, uint32_t arrayLayer, uint32_t sourceFontSize, uint32_t extraBorder, uint32_t searchDistance ) {
        _device = device;
        _assetsSource = assetsSource;
        _sourceFontSize = sourceFontSize;
        _extraBorder = extraBorder;
        _searchDistance = searchDistance;
        //
        if(_texArray) {
            return true; // 重复初始化？
        }
        tex_desc_t texDesc;
        texDesc.type = TextureType::Texture2DArray;
        texDesc.layerCount = arrayLayer;
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
        _sdfBitmapBuffer.reserve( cellSize*cellSize*8 );   // 预先准备 32 个字体的空间，防止频繁分配buffer
        //
        {
            // 初始化一个字体（先写死，做个测试）
            // auto inputStream = _assetsSource->Open(hgl::UTF8String("hwzhsong.ttf"));
            auto inputStream = _assetsSource->Open(hgl::UTF8String("msyahei.ttf"));
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

    GlyphInfo* SDFTextureTileManager::getGlyph( GlyphKey glyph ) {
        auto it = _glyphRecord.find(glyph);
        if( it != _glyphRecord.end()) {
            return it->second;
        }
        GlyphInfo* glyphInfo = registGlyphSDF(glyph);
        _glyphRecord[glyph] = glyphInfo;
        return glyphInfo;
    }

    GlyphInfo* SDFTextureTileManager::registGlyphSDF( GlyphKey glyph ) {
        if(glyph.fontID >= _fontTable.size()) {
            return nullptr;
        }
        float scale = 1.0f;
        float fontSize = _sourceFontSize* _DPI / 72.0f;
        //
        FontInfo& fontInfo = _fontTable[glyph.fontID];
        //
        scale = stbtt_ScaleForMappingEmToPixels(&fontInfo.stbTtfInfo, fontSize);
        //int baseline = (int)(fontInfo.ascent * scale);
        int advance, lsb, left, top, right, bottom;

        stbtt_GetCodepointHMetrics(&fontInfo.stbTtfInfo, glyph.charCode, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&fontInfo.stbTtfInfo, glyph.charCode, scale, scale, &left, &top, &right, &bottom);
        int bitmapWidth = right - left, bitmapHeight = bottom - top, bitmapOffsetX, bitmapOffsetY;
        int bitmapSize = bitmapWidth>bitmapHeight?bitmapWidth:bitmapHeight;
        float renderScale = 1.0f, ratio = 1.0f;
        if((int)_cellSize-_extraBorder*2 < bitmapSize ) {
            renderScale = (float)(_cellSize-_extraBorder*2) / bitmapSize;
            ratio = (float)_cellSize / (bitmapSize+_extraBorder*2);
        }
        //
        auto sdfBuffer = stbtt_GetCodepointSDF(&fontInfo.stbTtfInfo, scale*renderScale, glyph.charCode, _extraBorder, 128, (float)8/renderScale,&bitmapWidth, &bitmapHeight, &bitmapOffsetX, &bitmapOffsetY );

        size_t destBufferPosition = _sdfBitmapBuffer.size();
        size_t srcBufferPosition = 0; // bitmapWidth + bitmapOffsetX;
        size_t copySize = bitmapWidth*bitmapHeight;
        _sdfBitmapBuffer.resize(destBufferPosition + copySize); 
        memcpy( _sdfBitmapBuffer.data()+destBufferPosition, sdfBuffer, copySize);
        int bitmapAdvance = (advance + _extraBorder) * scale * ratio;

        GlyphInfo* glyphInfo = allocateGlyph();
        glyphInfo->bitmapBearingX =  bitmapOffsetX;
        glyphInfo->bitmapBearingY = bitmapOffsetY;
        glyphInfo->bitmapWidth = bitmapWidth;
        glyphInfo->bitmapHeight = bitmapHeight;
        glyphInfo->bitmapAdvance = bitmapAdvance;

        glyphInfo->SDFScale = ratio;
        /// 分配纹理位置
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

        TileItem tileItem;
        tileItem.bufferOffset = destBufferPosition;
        tileItem.bufferSize = copySize;
        tileItem.glyphInfo = glyphInfo;
        _cachedTileItems.push_back(tileItem);

        return glyphInfo;
    }

    GlyphInfo* SDFTextureTileManager::registGlyph( GlyphKey glyph ) {
        if(glyph.fontID >= _fontTable.size()) {
            return nullptr;
        }
        //
        float scale = 1.0f;
        float fontSize = _sourceFontSize* _DPI / 72.0f;
        //
        FontInfo& fontInfo = _fontTable[glyph.fontID];

        scale = stbtt_ScaleForMappingEmToPixels(&fontInfo.stbTtfInfo, fontSize);
        //int baseline = (int)(fontInfo.ascent * scale);
        int advance, lsb, x0, y0, x1, y1;
        float shiftX = (float)_extraBorder; float shiftY = (float)_extraBorder;
        stbtt_GetCodepointHMetrics(&fontInfo.stbTtfInfo, glyph.charCode, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&fontInfo.stbTtfInfo, glyph.charCode, scale, scale, &x0, &y0, &x1, &y1);
        // stbtt_GetCodepointBitmapBoxSubpixel(&fontInfo.stbTtfInfo, glyph.charCode, scale, scale, 0, 0, &x0, &y0, &x1, &y1);
        //stbtt_GetCodepointSDF( &fontInfo.stbTtfInfo, scale, glyph.charCode, )
        //
		uint8_t* rawBitmapPtr = _rawBitmapBuffer.data();
        memset(rawBitmapPtr, 0, (x1 - x0 + shiftX*2) *(y1 - y0+shiftY*2) );
        //
        stbtt_MakeCodepointBitmapSubpixel(
            &fontInfo.stbTtfInfo,
			rawBitmapPtr,
            x1 - x0,
            y1 - y0,
            x1 - x0 + shiftX*2,
            scale, scale,
            0, 0,
            // shiftX, shiftY,
            // shiftX*2, shiftY*2,
            glyph.charCode
        );
        
        int bearingX = (x0 - shiftX);
        int bearingY = y0 - shiftY;
        int bitmapWidth = x1 - x0 + shiftX * 2;
        int bitmapHeight = y1 - y0 + shiftY * 2;
        int bitmapAdvance = (advance + shiftX) * scale;
        //
        float diffRatio = (float)_cellSize / (fontSize*0.75f);
        float ratio = (float)_cellSize / ((bitmapWidth>bitmapHeight) ? bitmapWidth: bitmapHeight);
        ratio = ratio > diffRatio ? diffRatio : ratio;

        GlyphInfo* glyphInfo = allocateGlyph();
        glyphInfo->bitmapBearingX = bearingX * ratio;
        glyphInfo->bitmapBearingY = bearingY * ratio;
        glyphInfo->bitmapWidth = bitmapWidth * ratio;
        glyphInfo->bitmapHeight = bitmapHeight * ratio;
        glyphInfo->bitmapAdvance = bitmapAdvance * ratio;
        glyphInfo->SDFScale = ratio;
        /// 分配纹理位置
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

        signedDistanceFieldImage2D( rawBitmapPtr, bitmapWidth, bitmapHeight, &_sdfBitmapBuffer[outputPosition], glyphInfo->bitmapWidth, glyphInfo->bitmapHeight);

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

    void SDFTextureTileManager::tickResource( ResourceCommandEncoder* encoder, ResourceManager* resourceManager ) {
        resourceManager->tick();
        if(_cachedTileItems.size()) {
            size_t stagingBufferSize = this->_sdfBitmapBuffer.size();
            auto stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, stagingBufferSize );
            void* ptr = stagingBuffer->map(_device);
            memcpy(ptr, _sdfBitmapBuffer.data(), _sdfBitmapBuffer.size());
            stagingBuffer->unmap(_device);

            std::vector<uint32_t> offsets;
            std::vector<image_region_t> regions;
            for( const auto& item : _cachedTileItems ) {
                image_region_t region;
                region.mipLevel = 0;
                region.arrayIndex = item.glyphInfo->texIndex;
                region.offset = { item.glyphInfo->bitmapOffsetX, item.glyphInfo->bitmapOffsetY, 0 };
                region.extent = { item.glyphInfo->bitmapWidth, item.glyphInfo->bitmapHeight, 1 };
                regions.push_back(region);
                offsets.push_back(item.bufferOffset);
            }
            encoder->updateImage( _texArray, stagingBuffer, regions.data(), offsets.data(), (uint32_t)regions.size() );
            //
            resourceManager->trackResource(stagingBuffer);
            _cachedTileItems.clear();
        }
    }

    Texture* SDFTextureTileManager::texture() {
        return _texArray;
    }
}