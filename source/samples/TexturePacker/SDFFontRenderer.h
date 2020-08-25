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
        hgl::Vector4f       _transform[2];
        //
        struct {
            float               smoothStepFactor[2];
            float               layerIndex;
            uint32_t            colorMask;
        }                   _sdfArgument;

        struct Index {
            uint32_t index;
        };
        struct Effect {
            uint32_t    colorMask;     // 颜色参数
            uint32_t    effectColor;   // 效果颜色
            uint32_t    type:8;        // 类型 : 加粗、描边、阴影、内发光、无
            uint32_t    arrayIndex:8;  // 纹理数组索引
            uint32_t    gray:8;        // 灰度
            uint32_t    padding:8;  
            bool operator < ( const Effect& other ) const {
                return memcmp( this, &other, sizeof(Effect)) < 0;
            }
        };
        struct Transform {
            hgl::Vector3f col1;
            hgl::Vector3f col2;
            bool operator < ( const Transform& other ) const {
                return memcmp( this, &other, sizeof(Transform)) < 0;
            }
        };
        //
        std::vector<Index>              _indices;
        std::vector<Effect>             _effects;
        std::vector<Transform>          _transforms;
        std::map<Effect, uint32_t>      _effectRecord;
        std::map<Transform, uint32_t>   _transformRecord;
    public:
        SDFFontDrawData()
            : _argumentGroup( nullptr )
            , _vertexBuffer( nullptr )
            , _indexBuffer( nullptr )
            , _drawable( nullptr )
            , _transform { { 1.0f, 0.0f, 0.0f, 1.0f } , { 0.0f, 1.0f, 0.0f, 0.0f } }
            , _sdfArgument { { 0.49f, 0.50f }, 0.0f, 0xffffffff }
        {
        }

        // 这个函数不应该放在这里，但为了测试方便就在这里加了
        void setScreenSize( float width, float height ) {
            _transform[0].w = width;
            _transform[1].w = height;
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
        uint32_t                    _indicesHandle; // "Indices"
        uint32_t                    _effectsHandle; // "Effects"
        uint32_t                    _transformsHandle; // "Transforms"
        uint32_t                    _contextHandle; // "Context"
        //
        uint32_t                    _texArraySamplerHandle; // "TexArraySampler"
        uint32_t                    _texArrayHandle; // "TexArray"

        uint32_t                    _width;
        uint32_t                    _height;

        SDFRenderParameter          _sdfParameter;

        std::vector<FontMeshVertex> _verticesCache;
        std::vector<uint16_t>       _indicesCache;

        std::vector<SDFFontDrawData::Index>             _indices;
        std::vector<SDFFontDrawData::Effect>            _effects;
        std::vector<SDFFontDrawData::Transform>         _transforms;
        std::map<SDFFontDrawData::Effect, uint32_t>     _effectRecord;
        std::map<SDFFontDrawData::Transform, uint32_t>  _transformRecord;
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