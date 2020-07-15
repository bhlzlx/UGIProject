#pragma once
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <vector>
#include <cstdint>
#include <hgl/math/Vector.h>

namespace ugi {

    namespace gdi {

        typedef uint32_t GeometryHandle;

        class GDIContext;

         /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
            *  uniform 里主要是存共用的值，比如旋转，缩放，
            *  一般画一个矩形或者多个复杂的形状缩放和角度都是相同的，这个值是可以复用的
            *  基本上等同于一个 3x3 矩阵
            *  变换过程，先 加一个负的 anchor 偏移，再缩放，再旋转，再加一个 anchor 偏移
            * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
            /*
                1 0 -x  |  cos -sin 0  |  a 0 0  |  1 0 x
                0 1 -y  |  sin cos  0  |  0 b 0  |  0 1 y
                0 0  1  |  0   0    1  |  0 0 1  |  0 0 1
                从左到右变换，从右往左乘
        */
            /*
                a*cos | -a*sin  | -a*cos*x+a*sin*y+x
                b*sin | b*cos   | -b*sin*x-b*cos*y+y
                0     | 0       | 1
            */
            struct GeometryTransformArgument {
                // 基本上等同于一个 3x3 矩阵
                // 变换过程，先 加一个负的 anchor 偏移，再绽放，再旋转，再加一个 anchor 偏移
                /*  我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了，GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
                    a*cos -a*sin -a*cos*x+a*sin*y+x
                    b*sin b*cos  -b*sin*x-b*cos*y+y
                */
                hgl::Vector4f data[2];
                GeometryTransformArgument();
                GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor);
            };

        class GeometryBatch {
            friend class GeometryBuilder;
            friend class GeometryGPUDrawData;
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

            GeometryBatch( uint32_t firstVertex, uint32_t firstIndex, uint32_t primitiveCount, GeometryTransformArgument* argBuff, uint32_t argCapacity )
                :_firstVertex(firstVertex)
                ,_firstIndex(firstIndex) 
                ,_primitiveCount(primitiveCount)
                ,_transArgBuffer(argBuff)  
                ,_transArgCount(1) // 第一个默认单位矩阵
                ,_transArgCapacity(argCapacity)
            {
                argBuff[0] = GeometryTransformArgument();
            }

            GeometryBatch( GeometryBatch&& batch )
                :_firstVertex(batch._firstVertex)     
                ,_firstIndex(batch._firstIndex) 
                ,_primitiveCount(batch._primitiveCount)
                ,_transArgBuffer(nullptr)
                ,_transArgCount(batch._transArgCount)
                ,_transArgCapacity(batch._transArgCapacity)
            {
                free(_transArgBuffer );
                _transArgBuffer = batch._transArgBuffer;
                batch.reset();
            }

            void reset() {
                _firstVertex = 0;
                _firstIndex = 0;
                _primitiveCount = 0;
                _transArgBuffer = nullptr;
                _transArgCount = 0;
                _transArgCapacity = 0;
            }
        };

        class GeometryGPUDrawData {
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
            //
            std::vector<GeometryBatch>          _batches;
            std::vector<ugi::ArgumentGroup*>    _argGroups;
            //
            ugi::ResourceDescriptor             _transformDescriptor; 
        public:
            GeometryGPUDrawData(GDIContext* context);
            ///> 准备资源
            void updateGeometryTranslation( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation );
            void prepareResource( ResourceCommandEncoder* encoder, UniformAllocator* allocator );
            ///> 绘制
            void draw( RenderCommandEncoder* encoder );

            ~GeometryGPUDrawData();
        };

    }

}