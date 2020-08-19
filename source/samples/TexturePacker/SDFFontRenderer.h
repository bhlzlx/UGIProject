#pragma once

#include <hgl/math/Vector.h>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    class SDFTextureTileManager;

    struct FontMeshVertex {
        hgl::Vector2f position;
        hgl::Vector2f uv;
    };

    struct CharactorUniform {
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
    public:
        SDFFontDrawData()
            : _argumentGroup( nullptr )
            , _vertexBuffer( nullptr )
            , _indexBuffer( nullptr )
            , _drawable( nullptr )
            , _transform { { 1.0f, 0.0f, 0.0f, 1.0f } , { 0.0f, 1.0f, 0.0f, 0.0f } }
            , _sdfArgument { { 0.50f, 0.52f }, 0.0f, 0xffffffff }
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
        uint32_t                    _transformHandle;
        uint32_t                    _sampler2DArrayHandle;
        uint32_t                    _texture2DArrayHandle;
        uint32_t                    _sdfArgumentHandle;

        std::vector<FontMeshVertex> _verticesCache;
        std::vector<uint16_t>       _indicesCache;
        //
        struct BufferUpdateItem {
            Buffer* vertexBuffer;
            Buffer* indexBuffer;
            Buffer* stagingBuffer;
        };
        std::vector<BufferUpdateItem> _updateItems;
    public:
        SDFFontRenderer();
        bool initialize( Device* device, hgl::assets::AssetsSource* assetsSource );
        SDFFontDrawData* buildDrawData( float x, float y, const std::vector<SDFChar>& text );
        void tickResource( ResourceCommandEncoder* resEncoder );
        void prepareResource( ResourceCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator );
        void draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount );
    };

}