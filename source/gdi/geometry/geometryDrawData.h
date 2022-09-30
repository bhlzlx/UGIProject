#pragma once
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <vector>
#include <cstdint>
#include <hgl/math/Vector.h>
#include "geometryDefine.h"

namespace ugi {

    namespace gdi {

        typedef uint32_t GeometryHandle;

        class GDIContext;
        struct GeometryTransformArgument;

        class GeometryBatch {
            friend class GeometryBuilder;
            friend class GeometryDrawData;
        private:
            uint32_t                    _firstVertex;            // 当前Batch的起始顶点位置
            uint32_t                    _firstIndex;             // 当前Batch的起始索引位置
            uint32_t                    _primitiveCount;         // 当前Batch的绘制顶点个数（index）
            GeometryTransformArgument*  _transArgBuffer;         // 变换的buffer
            uint32_t                    _transArgCount;          //
            uint32_t                    _transArgCapacity;       //

            GeometryBatch( const GeometryBatch& ) = default; 
        public:
            GeometryBatch()
                :_firstVertex(0)     
                ,_firstIndex(0) 
                ,_primitiveCount(0)
                ,_transArgBuffer(nullptr)  
                ,_transArgCount(0)
                ,_transArgCapacity(0)
            {
            }

            GeometryBatch( uint32_t firstVertex, uint32_t firstIndex, uint32_t primitiveCount, GeometryTransformArgument* argBuff, uint32_t argCapacity );
            GeometryBatch( GeometryBatch&& batch ) noexcept;
            void reset();
        };

        class GeometryDrawData {
            friend class GeometryBuilder;
        private:
            GDIContext*                          _context;
            /// <summary>
            ///  GPU Resource                    
            /// </summary>
            ugi::Buffer*                        _vertexBuffer;
            ugi::Buffer*                        _indexBuffer;
            ugi::Drawable*                      _drawable;
            //
            std::vector<GeometryBatch>          _batches;
            std::vector<ugi::DescriptorBinder*>    _argGroups;
            //
            ugi::ResourceDescriptor             _elementInformationDescriptor;
            ugi::ResourceDescriptor             _globalInformationDescriptor;

            ContextInformation                  _contextInformation;
            GeometryTransformArgument           _transform;
            hgl::Vector2f                       _translation;
            uint32_t                            _colorMask;
            uint32_t                            _extraFlags;
        public:
            GeometryDrawData(GDIContext* context);
            ///> 准备资源
            void setElementTransform( GeometryHandle handle, const hgl::Vector2f& offset );
            void setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale );
            void setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, float rotation );
            void setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation );
            void setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation, const hgl::Vector2f& offset );
            void setElementTransform( GeometryHandle handle, const hgl::Vector3f(&)[2] );
            void setElementColorMask( GeometryHandle handle, uint32_t colorMask );
            void setElementExtraFlags( GeometryHandle handle, uint32_t flags );
            //
            void prepareResource( ResourceCommandEncoder* encoder, UniformAllocator* allocator );
            ///> 绘制
            void draw( RenderCommandEncoder* encoder );
            ///> 更新当前数据全局变换
            void setTransform( const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian );
            void setTranslation( const hgl::Vector2f& translate );
            void setColorMask( uint32_t colorMask );
            void setExtraFlags( uint32_t flags);
            void setScissor( float left, float right, float top, float bottom );
            ///> 析构
            ~GeometryDrawData();
        };

    }

}