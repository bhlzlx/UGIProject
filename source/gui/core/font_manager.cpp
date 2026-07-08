#include "font_manager.h"
#include <ugi/device.h>
#include <ugi/texture.h>
#include <ugi/buffer.h>
#include <ugi/command_queue.h>
#include <ugi/command_buffer.h>
#include <ugi/command_encoder/resource_cmd_encoder.h>
#include <ugi/asyncload/gpu_asyncload_manager.h>
#include <ugi/flight_cycle_invoker.h>
#include <cstring>
#include <cmath>
#include <cassert>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace gui {

    FontManager::~FontManager() {
        // _texArray 由 UGI 管理，不在此释放
    }

    bool FontManager::initialize(ugi::Device* device, ugi::GPUAsyncLoadManager* asyncMgr, Config const& cfg) {
        _device = device;
        _asyncLoadMgr = asyncMgr;
        _cfg = cfg;

        _tilesPerRow    = _cfg.atlasSize / _cfg.tileSize;
        _tilesPerLayer  = _tilesPerRow * _tilesPerRow;

        // 创建纹理数组
        ugi::tex_desc_t desc;
        desc.type         = ugi::TextureType::Texture2DArray;
        desc.format       = ugi::UGIFormat::R8_UNORM;
        desc.width        = _cfg.atlasSize;
        desc.height       = _cfg.atlasSize;
        desc.depth        = 1;
        desc.mipmapLevel  = 1;
        desc.layerCount   = _cfg.maxLayers;
        _texArray = device->createTexture(desc);
        return _texArray != nullptr;
    }

    int FontManager::addFont(uint8_t const* ttfData, size_t size) {
        FontData fd;
        fd.ttfBuffer.assign(ttfData, ttfData + size);
        int offset = stbtt_GetFontOffsetForIndex(fd.ttfBuffer.data(), 0);
        if (offset < 0) return -1;

        int ok = stbtt_InitFont(&fd.info, fd.ttfBuffer.data(), offset);
        if (!ok) return -1;

        int id = (int)_fonts.size();
        _fonts.push_back(std::move(fd));
        return id;
    }

    GlyphInfo FontManager::getGlyph(int fontID, uint32_t charCode) {
        GlyphKey key = { (uint16_t)fontID, (uint16_t)charCode };
        uint32_t k = (uint32_t(key.fontID) << 16) | key.charCode;
        auto it = _glyphs.find(k);
        if (it != _glyphs.end()) {
            return it->second;
        }
        auto* info = generateGlyph(fontID, charCode);
        if (info) {
            _glyphs[k] = *info;
            GlyphInfo result = *info;
            delete info;
            return result;
        }
        return GlyphInfo{};
    }

    GlyphInfo* FontManager::generateGlyph(int fontID, uint32_t charCode) {
        if (fontID < 0 || fontID >= (int)_fonts.size()) return nullptr;

        auto& font = _fonts[fontID];
        float fontSize = (float)_cfg.sdfSourceSize * _cfg.dpi / 72.0f;
        float scale = stbtt_ScaleForMappingEmToPixels(&font.info, fontSize);

        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font.info, charCode, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font.info, charCode, scale, scale, &x0, &y0, &x1, &y1);

        int bw = x1 - x0, bh = y1 - y0;
        int bmpW = 0, bmpH = 0, offX = 0, offY = 0;
        float rs = 1.0f, ratio = 1.0f;

        int maxDim = bw > bh ? bw : bh;
        int cellInner = (int)_cfg.tileSize - (int)_cfg.extraBorder * 2;
        if (cellInner < maxDim) {
            rs = (float)cellInner / maxDim;
            ratio = (float)_cfg.tileSize / (maxDim + _cfg.extraBorder * 2);
        }

        auto sdfBuf = stbtt_GetCodepointSDF(&font.info, scale * rs, charCode,
            (int)_cfg.extraBorder, 128, (float)_cfg.searchDistance / rs,
            &bmpW, &bmpH, &offX, &offY);
        if (!sdfBuf) return nullptr;

        // 分配 tile
        uint32_t tileIdx = _nextTile++;
        uint32_t layer  = tileIdx / _tilesPerLayer;
        uint32_t local  = tileIdx % _tilesPerLayer;
        uint32_t row    = local / _tilesPerRow;
        uint32_t col    = local % _tilesPerRow;

        if (layer >= _cfg.maxLayers) {
            stbtt_FreeSDF(sdfBuf, nullptr);
            return nullptr;
        }

        // CPU 缓冲区暂存，记录给 tickUpload 用
        uint32_t bufOff = (uint32_t)_uploadBuffer.size();
        size_t copySize = (size_t)bmpW * bmpH;
        _uploadBuffer.resize(bufOff + copySize);
        memcpy(_uploadBuffer.data() + bufOff, sdfBuf, copySize);
        stbtt_FreeSDF(sdfBuf, nullptr);

        auto* gi = new GlyphInfo{};
        gi->glyphIndex     = tileIdx;
        gi->bitmapWidth    = bmpW;
        gi->bitmapHeight   = bmpH;
        gi->bitmapLayer    = layer;
        gi->bitmapOffsetX  = (int32_t)(col * _cfg.tileSize);
        gi->bitmapOffsetY  = (int32_t)(row * _cfg.tileSize);

        gi->bitmapBearingX = (float)offX;
        gi->bitmapBearingY = (float)offY;
        gi->bitmapAdvance  = (float)(advance + (int)_cfg.extraBorder) * scale * ratio;
        gi->SDFScale       = ratio;

        float texSize = (float)_cfg.atlasSize;
        gi->texU = (col * _cfg.tileSize) / texSize;
        gi->texV = (row * _cfg.tileSize) / texSize;
        gi->texW = (float)bmpW / texSize;
        gi->texH = (float)bmpH / texSize;

        // 记下给 tickUpload，等上传完再 delete
        _pendingUploads.push_back({bufOff, gi});

        return gi;
    }

    void FontManager::tickUpload(ugi::Device* device) {
        if (_pendingUploads.empty()) return;

        size_t bufSize = _uploadBuffer.size();
        auto staging = device->createBuffer(ugi::BufferType::StagingBuffer, (uint32_t)bufSize);
        void* ptr = staging->map(device);
        memcpy(ptr, _uploadBuffer.data(), bufSize);
        staging->unmap(device);

        // 构建上传区域
        std::vector<uint32_t> offsets;
        std::vector<ugi::image_region_t> regions;
        for (auto& up : _pendingUploads) {
            auto* gi = up.glyph;
            ugi::image_region_t r;
            r.mipLevel    = 0;
            r.arrayIndex  = gi->bitmapLayer;
            r.arrayCount  = 1;
            r.offset      = { gi->bitmapOffsetX, gi->bitmapOffsetY, 0 };
            r.extent      = { gi->bitmapWidth, gi->bitmapHeight, 1 };
            regions.push_back(r);
            offsets.push_back(up.bufferOffset);
            delete gi;  // GlyphInfo 已拷贝到 _glyphs map，临时对象释放
        }

        // 用 transfer queue 执行上传
        auto& transferQueues = device->transferQueues();
        auto queue = transferQueues[0];
        auto cb = queue->createCommandBuffer(device, ugi::CmdbufType::Resetable);
        cb->beginEncode(); {
            auto enc = cb->resourceCommandEncoder();
            enc->updateImage(_texArray, staging, regions.data(), offsets.data(), (uint32_t)regions.size());
            enc->endEncode();
        }
        cb->endEncode();

        auto fence = device->createFence();
        {
            ugi::QueueSubmitInfo si(&cb, 1, nullptr, 0, nullptr, 0);
            ugi::QueueSubmitBatchInfo sb(&si, 1, fence);
            queue->submitCommandBuffers(sb);
        }

        auto onComplete = [](ugi::Device* dev, ugi::Buffer* stg, ugi::CommandBuffer* tcb,
                              ugi::CommandQueue* q) {
            q->destroyCommandBuffer(dev, tcb);
            dev->destroyBuffer(stg);
        };
        using namespace std::placeholders;
        auto binder = std::bind(onComplete, device, staging, cb, queue);
        ugi::GPUAsyncLoadItem item(device, fence, binder);
        _asyncLoadMgr->registerAsyncLoad(std::move(item));

        _pendingUploads.clear();
        _uploadBuffer.clear();
    }

    FontMetrics FontManager::getMetrics(int fontID, float fontSize) const {
        FontMetrics m;
        if (fontID < 0 || fontID >= (int)_fonts.size()) return m;

        auto& font = _fonts[fontID];
        float pxSize = fontSize * _cfg.dpi / 72.0f;
        m.scale = stbtt_ScaleForMappingEmToPixels(&font.info, pxSize);

        stbtt_GetFontVMetrics(&font.info, &m.ascent, &m.descent, &m.lineGap);
        return m;
    }

}
