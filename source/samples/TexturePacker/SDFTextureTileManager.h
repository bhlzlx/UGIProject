#pragma once
#include <ugi/UGIDeclare.h>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <stb_truetype.h>
#include <algorithm>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {
    struct GlyphInfo {
        // ====================Tile管理信息===============
        uint32_t    glyphIndex : 32;
        int32_t     bitmapOffsetX: 16;
        int32_t     bitmapOffsetY: 16;
        uint32_t    bitmapLayerIndex : 16;
        // 位图宽高
        uint32_t    bitmapWidth;
        uint32_t    bitmapHeight;
        // ====================渲染信息===================
        // 基线偏移
        float       bitmapBearingX;
        float       bitmapBearingY;
        // 字宽跨越
        float       bitmapAdvance;
        // 位图大小是原图缩放多少后的
        float       SDFScale;
        // textureArray[w].uv 起点
        float       texU, texV, texWidth, texHeight, texIndex;
    };
    
    struct GlyphKey {
        union {
            uint32_t handle;
            struct {
                uint32_t fontID     : 16;       // 字体索引
                uint32_t charCode   : 16;       // 字符编码 utf16
            };
        };
    };
}

namespace std {
    template<>
    class hash<ugi::GlyphKey>  {
        size_t operator()(const ugi::GlyphKey& key) const {
            return key.handle;
		}
	};
}

namespace ugi {

    struct FontInfo {
        stbtt_fontinfo stbTtfInfo;
        int x0, y0;
        int x1, y1;
        int maxHeight;
        int ascent; int descent; int lineGap;
    };

    //
    class SDFTextureTileManager {
        struct TileItem {
            uint32_t    bufferOffset;
            uint32_t    bufferSize;
            GlyphInfo*  glyphInfo;
        };
        struct SDFFlag {
            uint8_t inner : 1;
            uint8_t opaque : 1;
        };
    private:
        Device*                                         _device;
        ResourceManager*                                _resourceManager;
        hgl::assets::AssetsSource*                      _assetsSource;
        Texture*                                        _texArray;
        uint32_t                                        _cellSize;
        uint32_t                                        _position;
        uint32_t                                        _capacity;
        uint32_t                                        _row;
        uint32_t                                        _col;
        uint32_t                                        _layers;
        //
        std::unordered_map< GlyphKey, GlyphInfo* >      _glyphRecord;
        std::vector<GlyphInfo>                          _glyphInfoPool;
        std::queue< GlyphInfo* >                        _freeTable;
        std::vector< FontInfo >                         _fontTable;
        //
        float                                           _DPI;
        std::vector<uint8_t>                            _rawBitmapBuffer;
        std::vector<uint8_t>                            _sdfBitmapBuffer;
        std::vector<SDFFlag>                            _SDFFlags;
        std::vector<TileItem>                           _cachedTileItems;
    public:
        SDFTextureTileManager();
        // === 
        bool initialize( Device* device,  hgl::assets::AssetsSource* assetsSource, uint32_t cellSize, uint32_t textureWidth, uint32_t arrayLayer );
        GlyphInfo* allocateGlyph();
        GlyphInfo* registGlyph( GlyphKey glyph );
        void unregistGlyph( GlyphKey glyph );

        void signedDistanceFieldImage2D(
            uint8_t* src, int32_t srcWid, int32_t srcHei,
            uint8_t* dst, int32_t dstWid, int32_t dstHei
        );

        void resourceTick( ResourceCommandEncoder* encoder );

        Texture* texture();
    };

}