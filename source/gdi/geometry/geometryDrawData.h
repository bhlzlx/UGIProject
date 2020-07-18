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
        class GeometryTransformArgument;

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
            GeometryBatch( GeometryBatch&& batch );
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
            uint32_t                            _maxVertex;
            GeometryTransformArgument           _globalTransform;
            //
            std::vector<GeometryBatch>          _batches;
            std::vector<ugi::ArgumentGroup*>    _argGroups;
            //
            ugi::ResourceDescriptor             _elementInformationDescriptor;
            ugi::ResourceDescriptor             _globalInformationDescriptor;
            //
            struct GlobalInformationPrototype {
                hgl::Vector4f screenSize;
                GeometryTransformArgument globalTransform;
            };
        public:
            GeometryDrawData(GDIContext* context);
            ///> 准备资源
            void updateElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation );
            void prepareResource( ResourceCommandEncoder* encoder, UniformAllocator* allocator );
            ///> 绘制
            void draw( RenderCommandEncoder* encoder );
            ///> 更新当前数据全局变换
            void updateTransfrom( const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian );

            ~GeometryDrawData();
        };

    }

}