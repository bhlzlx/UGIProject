#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <stb_truetype.h>
#include <utils/singleton.h>

namespace ugi {
    class Device;
    class Texture;
    class GPUAsyncLoadManager;
}

namespace gui {

    /// SDF 字形图集单个 tile 的信息
    struct GlyphInfo {
        uint32_t glyphIndex = 0;
        int32_t  bitmapOffsetX = 0;   // tile 在纹理层内的 X 偏移
        int32_t  bitmapOffsetY = 0;   // tile 在纹理层内的 Y 偏移
        uint32_t bitmapWidth  = 0;
        uint32_t bitmapHeight = 0;
        uint32_t bitmapLayer  = 0;    // 纹理数组层索引

        float bitmapBearingX = 0;     // 渲染时的 X 偏移
        float bitmapBearingY = 0;     // 渲染时的 Y 偏移
        float bitmapAdvance  = 0;     // 水平推进量
        float SDFScale       = 1.0f;  // 缩放系数

        // UV 坐标 (归一化到 [0,1])
        float texU = 0, texV = 0, texW = 0, texH = 0;
    };

    struct GlyphKey {
        uint32_t fontID   : 16;
        uint32_t charCode : 16;

        bool operator==(GlyphKey const& o) const { return fontID == o.fontID && charCode == o.charCode; }
    };

    /// 字体度量信息
    struct FontMetrics {
        int ascent  = 0;    // 基线到顶
        int descent = 0;    // 基线到底
        int lineGap = 0;    // 行间距
        float scale = 1.0f; // 当前字号下的缩放系数
    };

    /// SDF 字体管理器 — 单例
    /// 负责字形 SDF 位图生成、纹理图集管理、GPU 上传
    class FontManager : public comm::Singleton<FontManager> {
        friend class comm::Singleton<FontManager>;
    public:
        struct Config {
            uint32_t sdfSourceSize   = 64;    // SDF 源字体大小 (px)
            uint32_t extraBorder     = 8;     // SDF 额外边距
            uint32_t searchDistance  = 8;     // SDF 搜索距离
            uint32_t tileSize        = 64;    // 每 tile 像素尺寸
            uint32_t atlasSize       = 1024;  // 纹理层边长
            uint32_t maxLayers       = 4;     // 最大纹理数组层数
            float    dpi             = 96.0f; // DPI
        };

    private:
        Config                              _cfg;
        ugi::Device*                        _device = nullptr;
        ugi::GPUAsyncLoadManager*           _asyncLoadMgr = nullptr;
        ugi::Texture*                       _texArray = nullptr;

        // 每字体: stb_truetype 数据
        struct FontData {
            stbtt_fontinfo  info;
            std::vector<uint8_t> ttfBuffer;  // 保留原始 ttf 数据
        };
        std::vector<FontData>               _fonts;

        // 字形缓存
        std::unordered_map<uint32_t, GlyphInfo> _glyphs;  // key = GlyphKey打包

        // 图集 tile 分配
        uint32_t _tilesPerRow = 0;
        uint32_t _tilesPerLayer = 0;
        uint32_t _nextTile = 0;     // 下一个可用 tile (0-based)

        // 待上传 CPU 缓冲
        std::vector<uint8_t>                 _uploadBuffer;
        struct UploadItem {
            uint32_t bufferOffset;
            GlyphInfo* glyph;
        };
        std::vector<UploadItem>              _pendingUploads;

        // 字形 SDF 生成
        GlyphInfo* generateGlyph(int fontID, uint32_t charCode);
        void signedDistanceField(uint8_t* src, int sw, int sh,
                                 uint8_t* dst, int dw, int dh);
        bool uploadPending(ugi::Device* device);

    private:
        FontManager() = default;
    public:
        ~FontManager();

        /// 初始化图集
        bool initialize(ugi::Device* device, ugi::GPUAsyncLoadManager* asyncMgr, Config const& cfg = {});

        /// 注册字体 (ttf 文件数据)，返回 fontID
        int addFont(uint8_t const* ttfData, size_t size);

        /// 获取字形信息 (首次请求时自动生成 SDF)，返回拷贝，调用方用完即弃
        GlyphInfo getGlyph(int fontID, uint32_t charCode);

        /// 上传所有待处理字形到 GPU (每帧调用，无新数据则为空操作)
        void tickUpload(ugi::Device* device);

        /// 获取 SDF 纹理数组
        ugi::Texture* sdfTexture() const { return _texArray; }

        /// 获取配置
        Config const& config() const { return _cfg; }

        /// 获取字体度量 (按指定字号)
        FontMetrics getMetrics(int fontID, float fontSize) const;
    };

}

// std::hash 特化
namespace std {
    template<> struct hash<gui::GlyphKey> {
        size_t operator()(gui::GlyphKey const& k) const noexcept {
            return (uint32_t(k.fontID) << 16) | k.charCode;
        }
    };
}
