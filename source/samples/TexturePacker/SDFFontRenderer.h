#pragma once

#include <hgl/math/Vector.h>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <hgl/type/RectScope.h>
#include <map>

/*
    因为变换这种数据重复率太高了，一般不会有
*/

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

    class SDFTextureTileManager;

    struct FontMeshVertex {
        hgl::Vector2f position;
        hgl::Vector2f uv;
    };

    struct IndexHandle {
        union {
            uint32_t handles[2];
            struct {
                uint32_t styleIndex         :16;    // 样式
                uint32_t transformIndex     :16;    // 变换
                uint32_t layerIndex         :8;     // 纹理LayerIndex
            };
        };
    };

    struct Style {
        uint32_t color = 0xffffffff;
        uint32_t styleColor = 0xffffffff;
        union {
            uint32_t composedFlags = 0x0;
            struct {
                uint32_t    type:8;        // 类型 : 加粗、描边、阴影、内发光、无
                uint32_t    arrayIndex:8;  // 纹理数组索引 ( 这个不能再放在这里了，需要单独给一个ｂｕｆｆｅｒ)
                uint32_t    gray:8;        // 灰度
                uint32_t    padding1:8;
            };
        };
        uint32_t    reserved;
        Style( uint32_t c, uint32_t s, uint32_t f)
            : color(c)
            , styleColor(s)
            , composedFlags(f) {
        }

        bool operator < ( const Style& other ) const {
            return memcmp( this, &other, sizeof(Style)) < 0;
        }
    };

    struct Transform {
        hgl::Vector3f col1;
        hgl::Vector3f col2;
        hgl::Vector2f padding;
        Transform( const hgl::Vector3f& c1, const hgl::Vector3f& c2 )
            : col1(c1)
            , col2(c2) {
        }
        //
        bool operator < ( const Transform& other ) const {
            return memcmp( this, &other, sizeof(Transform)) < 0;
        }

        static Transform createTransform( const hgl::Vector2f& offset ) {
            return Transform(
                hgl::Vector3f( 1.0f, 0.0f, offset.x),
                hgl::Vector3f( 0.0f, 1.0f, offset.y)
            );
        }

        static Transform createTransform( const hgl::Vector2f& anchor, const hgl::Vector2f& scale ) {
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            return Transform( 
                hgl::Vector3f(a, 0, -a*x + x),
                hgl::Vector3f(0, b, -b*y + y)
            );
        }

        static Transform createTransform( const hgl::Vector2f& anchor, float radian ){
            float cosValue = cos(radian); float sinValue = sin(radian);
            float x = anchor.x; float y = anchor.y;
            return Transform(
                hgl::Vector3f(cosValue, -sinValue, -cosValue*x + sinValue*y + x),
                hgl::Vector3f(sinValue, cosValue,  -sinValue*x - cosValue*y + y)
            );
        }
        
        static Transform createTransform(const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation ) {
            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            return Transform(
                hgl::Vector3f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x),
                hgl::Vector3f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y)
            );
        }


        static Transform createTransform( const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian, const hgl::Vector2f& offset ) {
            float cosValue = cos(radian); float sinValue = sin(radian);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            Transform rst = Transform(
                hgl::Vector3f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x),
                hgl::Vector3f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y)
            );
            rst.col1.z += offset.x;
            rst.col2.z += offset.y;
        }
    };

    class SDFFontDrawData {
        friend class SDFFontRenderer;
    private:
        DescriptorBinder*      _argumentGroup;
        //
        Buffer*             _vertexBuffer;
        Buffer*             _indexBuffer;
        Buffer*             _texArrIndexBuffer;
        Drawable*           _drawable;
        uint32_t            _indexCount;
        //
        std::vector<IndexHandle>        _indices;
        std::vector<Style>              _styles;
        std::vector<Transform>          _transforms;
    public:
        SDFFontDrawData()
            : _argumentGroup( nullptr )
            , _vertexBuffer( nullptr )
            , _indexBuffer( nullptr )
            , _texArrIndexBuffer( nullptr )
            , _drawable( nullptr )
            , _indexCount(0)
        {
        }

        //==== Update Parameters
        void updateTransform( IndexHandle handle, Transform transform );
        void updateStyle( IndexHandle handle, Style style );

    };

    struct SDFChar {
        uint32_t fontID     : 8;
        uint32_t fontSize   : 8;
        uint32_t charCode   : 16;
        SDFChar()
            : fontID(0)
            , fontSize(18)
            , charCode(u'A')
        {
        }
        SDFChar( uint32_t a, uint32_t b, uint32_t c )
            : fontID(a)
            , fontSize(b)
            , charCode(c)
        {
        }
    };

    class SDFFontRenderer {
    private:
        Device*                     _device;
        GraphicsPipeline*                   _pipeline;
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

        std::vector<FontMeshVertex> _meshVBO;
        std::vector<uint16_t>       _meshIBO;

        std::vector<IndexHandle>        _indices;
        std::vector<Style>              _styles;
        std::vector<Transform>          _transforms;
        // std::map<Style, uint32_t>       _effectRecord;
        // std::map<Transform, uint32_t>   _transformRecord;
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
        // 这个接口默认会给分配一个新的 style/transform index位以及数据元
        IndexHandle appendText( float x, float y, SDFChar* text, uint32_t length, const Transform& transform, const Style& style, hgl::RectScope2f& rect );
        // 这个接口是用来重用的
        IndexHandle appendTextReuseTransform( 
            float x, float y, SDFChar* text, uint32_t length,
            IndexHandle resuseHandle, const Style& style, hgl::RectScope2f& rect
        );
        IndexHandle appendTextReuseStyle ( 
            float x, float y, SDFChar* text, uint32_t length, 
            IndexHandle resuseHandle, const Transform& transform, hgl::RectScope2f& rect
        );
        IndexHandle appendTextReuse ( 
            float x, float y, SDFChar* text, uint32_t length, 
            IndexHandle resuseHandle, hgl::RectScope2f& rect
        );

        SDFFontDrawData* endBuild();
        //==== Resource functions
        void tickResource( ResourceCommandEncoder* resEncoder );
        void prepareResource( ResourceCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator );
        void draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount );
    };

}