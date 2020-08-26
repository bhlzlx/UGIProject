#pragma once

#include <hgl/math/Vector.h>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <map>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    struct SDFRenderParameter {
        uint32_t texArraySize : 16;
        uint32_t texLayer : 8;
        uint32_t destinationSDFSize : 8;
        uint32_t sourceFontSize : 8;
        uint32_t extraSourceBorder : 8;
        uint32_t searchDistance : 8;
    };

    /*
layout( set = 0, binding = 0 ) uniform Indices {
    uint effectIndices[8192];
    uint transformIndices[8192];
};

// 一个批次最多支持512种样式
layout( set = 0, binding = 1) uniform Effects {
    EffectImp effects[512];
};

// 一个批次最多支持512种变换
layout( set = 0, binding = 2) uniform Transforms {
    TransformImp transforms[512];
};

layout( set = 0, binding = 3) uniform Context {
    float contextWidth;
    float contextHeight;
};
    */

    class SDFTextureTileManager;

    struct FontMeshVertex {
        hgl::Vector2f position;
        hgl::Vector2f uv;
    };

    struct IndexHandle {
        union {
            uint32_t handle;
            struct {
                uint32_t styleIndex:16;        // 样式
                uint32_t transformIndex:16;     // 变换
            };
        };
    };

    struct Style {
        uint32_t color = 0xffffffff;
        uint32_t effectColor = 0xffffffff;
        union {
            uint32_t composedFlags = 0x0;
            struct {
                uint32_t    type:8;        // 类型 : 加粗、描边、阴影、内发光、无
                uint32_t    arrayIndex:8;  // 纹理数组索引
                uint32_t    gray:8;        // 灰度
                uint32_t    padding1:8;
            };
        };
        uint32_t    reserved;
        bool operator < ( const Style& other ) const {
            return memcmp( this, &other, sizeof(Style)) < 0;
        }
    };

    struct Transform {
        hgl::Vector3f col1;
        hgl::Vector3f col2;
        hgl::Vector2f padding;
        bool operator < ( const Transform& other ) const {
            return memcmp( this, &other, sizeof(Transform)) < 0;
        }
    };

    class SDFFontDrawData {
        friend class SDFFontRenderer;
    private:
        ArgumentGroup*      _argumentGroup;
        //
        Buffer*             _vertexBuffer;
        Buffer*             _indexBuffer;
        Drawable*           _drawable;
        uint32_t            _indexCount;
        //
        std::vector<IndexHandle>        _indices;
        std::vector<Style>              _effects;
        std::vector<Transform>          _transforms;
        std::map<Style, uint32_t>       _effectRecord;
        std::map<Transform, uint32_t>   _transformRecord;
    public:
        SDFFontDrawData()
            : _argumentGroup( nullptr )
            , _vertexBuffer( nullptr )
            , _indexBuffer( nullptr )
            , _drawable( nullptr )
        {
        }

    };

    struct SDFChar {
        uint32_t fontID     : 8;
        uint32_t fontSize   : 8;
        uint32_t charCode   : 16;
        uint32_t type       : 8;
        uint32_t renderID   : 16;
        uint32_t color;
        uint32_t effectColor;
        SDFChar()
            : fontID(0)
            , fontSize(18)
            , charCode(u'A')
            , type(0)
            , renderID(0)
            , color( 0xffffffff )
            , effectColor( 0xffffffff )
        {
        }
        SDFChar( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g)
            : fontID(a)
            , fontSize(b)
            , charCode(c)
            , type(d)
            , renderID(e)
            , color(f)
            , effectColor(g)
        {
        }
    };

    class SDFFontRenderer {
    private:
        Device*                     _device;
        Pipeline*                   _pipeline;
        PipelineDescription         _pipelineDescription;
        hgl::assets::AssetsSource*  _assetsSource;
        SDFTextureTileManager*      _texTileManager;
        ResourceManager*            _resourceManager;
        //
        uint32_t                    _indicesHandle;         // "Indices"
        uint32_t                    _effectsHandle;         // "Effects"
        uint32_t                    _transformsHandle;      // "Transforms"
        uint32_t                    _contextHandle;         // "Context"
        //
        uint32_t                    _texArraySamplerHandle; // "TexArraySampler"
        uint32_t                    _texArrayHandle;        // "TexArray"

        uint32_t                    _width;
        uint32_t                    _height;

        SDFRenderParameter          _sdfParameter;

        std::vector<FontMeshVertex> _verticesCache;
        std::vector<uint16_t>       _indicesCache;

        std::vector<IndexHandle>        _indices;
        std::vector<Style>              _effects;
        std::vector<Transform>          _transforms;
        std::map<Style, uint32_t>       _effectRecord;
        std::map<Transform, uint32_t>   _transformRecord;
        //
        struct BufferUpdateItem {
            Buffer* vertexBuffer;
            Buffer* indexBuffer;
            Buffer* stagingBuffer;
        };
        std::vector<BufferUpdateItem> _updateItems;
    public:
        SDFFontRenderer();
        bool initialize( Device* device, hgl::assets::AssetsSource* assetsSource, const SDFRenderParameter& sdfParam );
        void resize( uint32_t width, uint32_t height );
        //==== Build functions
        void beginBuild();
        void appendText( float x, float y, const std::vector<SDFChar>& text, const hgl::Vector3f (& transform )[2] );
        SDFFontDrawData* endBuild();
        //==== Resource functions
        void tickResource( ResourceCommandEncoder* resEncoder );
        void prepareResource( ResourceCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator );
        void draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount );
    };

}