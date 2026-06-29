#pragma once

#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <glm/glm.hpp>
#include <map>
#include <io/archive.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/asyncload/gpu_asyncload_manager.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_components/mesh.h>

namespace ugi {

struct RectScope2f { float left=0, right=0; void SetLeft(float l){left=l;} void SetRight(float r){right=r;} float GetRight()const{return right;} float GetCenterX()const{return (left+right)*0.5f;} };

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
        glm::vec2 position;
        glm::vec2 uv;
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
        glm::vec3 col1;
        glm::vec3 col2;
        glm::vec2 padding;
        Transform( const glm::vec3& c1, const glm::vec3& c2 )
            : col1(c1)
            , col2(c2)
            , padding(0.0f, 0.0f) {
        }
        //
        bool operator < ( const Transform& other ) const {
            return memcmp( this, &other, sizeof(Transform)) < 0;
        }

        static Transform createTransform( const glm::vec2& offset ) {
            return Transform(
                glm::vec3( 1.0f, 0.0f, offset.x),
                glm::vec3( 0.0f, 1.0f, offset.y)
            );
        }

        static Transform createTransform( const glm::vec2& anchor, const glm::vec2& scale ) {
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            return Transform( 
                glm::vec3(a, 0, -a*x + x),
                glm::vec3(0, b, -b*y + y)
            );
        }

        static Transform createTransform( const glm::vec2& anchor, float radian ){
            float cosValue = cos(radian); float sinValue = sin(radian);
            float x = anchor.x; float y = anchor.y;
            return Transform(
                glm::vec3(cosValue, -sinValue, -cosValue*x + sinValue*y + x),
                glm::vec3(sinValue, cosValue,  -sinValue*x - cosValue*y + y)
            );
        }
        
        static Transform createTransform(const glm::vec2& anchor, const glm::vec2& scale, float rotation ) {
            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            return Transform(
                glm::vec3(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x),
                glm::vec3(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y)
            );
        }


        static Transform createTransform( const glm::vec2& anchor, const glm::vec2& scale, float radian, const glm::vec2& offset ) {
            float cosValue = cos(radian); float sinValue = sin(radian);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            Transform rst = Transform(
                glm::vec3(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x),
                glm::vec3(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y)
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
        Renderable*         _renderable;
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
            , _renderable( nullptr )
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
        GraphicsPipeline*           _pipeline;
        MeshBufferAllocator*        _meshAllocator;
        GPUAsyncLoadManager*        _asyncLoader;
        pipeline_desc_t             _pipelineDescription;
        comm::IArchive*             _archive;
        SDFTextureTileManager*      _texTileManager;
        //
        res_descriptor_t            _descIndices;
        res_descriptor_t            _descEffects;
        res_descriptor_t            _descTransforms;
        res_descriptor_t            _descContext;
        res_descriptor_t            _descSampler;
        res_descriptor_t            _descTexArray;

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
        bool initialize( Device* device, comm::IArchive* archive, GPUAsyncLoadManager* asyncLoader, const SDFRenderParameter& sdfParam );
        void resize( uint32_t width, uint32_t height );
        //==== Build functions
        void beginBuild();
        // 这个接口默认会给分配一个新的 style/transform index位以及数据uint32_t元
        IndexHandle appendText( float x, float y, SDFChar* text,  uint32_t length, const Transform& transform, const Style& style, RectScope2f& rect );
        // 这个接口是用来重用的
        IndexHandle appendTextReuseTransform( 
            float x, float y, SDFChar* text, uint32_t length,
            IndexHandle resuseHandle, const Style& style, RectScope2f& rect
        );
        IndexHandle appendTextReuseStyle ( 
            float x, float y, SDFChar* text, uint32_t length, 
            IndexHandle resuseHandle, const Transform& transform, RectScope2f& rect
        );
        IndexHandle appendTextReuse ( 
            float x, float y, SDFChar* text, uint32_t length, 
            IndexHandle resuseHandle, RectScope2f& rect
        );

        SDFFontDrawData* endBuild();
        //==== Resource functions
        void tickResource( ResourceCommandEncoder* resEncoder );
        void prepareResource( ResourceCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator );
        void draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount );
    };

}