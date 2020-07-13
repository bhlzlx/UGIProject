#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>
#include <utility>

#include <ugi/buffer.h>
#include <ugi/Device.h>
#include <ugi/Drawable.h>
#include <hgl/assets/AssetsSource.h>

namespace ugi {

    namespace gdi {

        typedef uint32_t GeometryHandle;

        // 存储 GDI 设备上下文的状态
        // 先 使用GeometryBuilder 在CPU端组装顶点以及索引数据
        class GDIContext {
        private:
            ugi::Device*                _device;
            hgl::assets::AssetsSource*  _assetsSource;
            ugi::PipelineDescription    _pipelineDesc;
            ugi::Pipeline*              _pipeline;
            ugi::Buffer*                _contextInfo; // uniform buffer with the context info
            //
            hgl::Vector2f               _size;        // context size( width,height )
        public:
            GDIContext( ugi::Device* device, hgl::assets::AssetsSource* assetsSource )
                : _device( device )
                , _assetsSource( assetsSource )
                , _pipeline(nullptr)
                , _contextInfo(nullptr)
            {
            }

            bool initialize();

            ugi::Pipeline* pipeline() const noexcept;
            const ugi::PipelineDescription& pipelineDescription() const noexcept;
            ugi::Buffer* contextUniform() const noexcept;
            //
            void setSize(const hgl::Vector2f& size);
            //
            ugi::Device* device() { return _device;  };
            const hgl::Vector2f& size() { return _size; }
        };
        
        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        *  1. uniformElement 如果是0表明不旋转也不缩放
        *  2. color 这个属性实际上可以在shader里算最终颜色也可以提供一个颜色表来做映射（可能更快，目前先不这么做）
        *  3. position 的值如果想省CPU则可以放到GPU里作浮点运算（先放CPU里算）
        * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        struct GeometryVertex {
            hgl::Vector2f   position;           // 位置
            uint32_t        color;              // 颜色索引 R(16) G(16) B(16) A(16) 每个8位
            uint32_t        uniformElement;     // uniform 元素的索引
        };

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
        protected:
            std::vector<GeometryVertex>             _vertices;          // 顶点数据，上限为 4096，即大约1024个矩形
            std::vector<uint16_t>                   _indices;           // 顶点索引上限为 4096 和顶点数量上限一致
            std::vector<GeometryTransformArgument>  _uniformElements;   // UBO块 1024
        public:
            GeometryBatch()
                : _vertices {}
                , _indices {}
                , _uniformElements {}
            {
                _vertices.reserve(4096);
                _indices.reserve(4096);
                // 第一个uniform默认就是不作任务变换
                _uniformElements.reserve(1024);
                _uniformElements.resize(1);
                _uniformElements[0].data[0] = hgl::Vector4f(1,0,0,0);
                _uniformElements[0].data[1] = hgl::Vector4f(0,1,0,0);
            }

            const GeometryVertex* vertexData() const noexcept;
            size_t vertexDataLength() const noexcept;
            const uint16_t* indexData() const noexcept;
            size_t indexDataLength() const noexcept;
            const void* uniformData() const noexcept;
            size_t uniformDataLength() const noexcept;
            size_t uniformElementCount() const  noexcept;
        };

        class GeometryGPUDrawData;

        class GeometryMemoryData {
            class GeometryBatchPrivate : public GeometryBatch {

                bool checkVertex(size_t vertexCount);
                bool checkUniform();
            public:
                bool appendVertices(
                    GeometryVertex const* vertices,
                    size_t                              vertexCount,
                    uint16_t const* indices,
                    size_t                              indexCount,
                    const GeometryTransformArgument* uniform
                );
            };
        private:
            std::vector<GeometryBatchPrivate>      _geometryBatches;
        public:
            GeometryMemoryData()
                : _geometryBatches(1)
            {
            }

            GeometryHandle addGeometry( 
                GeometryVertex const*   vertices, 
                size_t                  vertexCount, 
                uint16_t const*         indices, 
                size_t                  indexCount
            );

            GeometryHandle addGeometry( 
                GeometryVertex const*   vertices, 
                size_t                  vertexCount, 
                uint16_t const*         indices, 
                size_t                  indexCount, 
                float                   angle, 
                const hgl::Vector2f&    scale,
                const hgl::Vector2f&    anchor
            );

            GeometryGPUDrawData* createGeometryDrawData( GDIContext* context );
        };

        class GeometryBuilder {
        private:
            GDIContext*                     _context;
            std::vector<GeometryVertex>     _vertexBuffer;
            std::vector<uint16_t>           _indicesBuffer;
        public:
            GeometryBuilder(GDIContext* context)
                : _context(context)
                , _vertexBuffer{}
                , _indicesBuffer{}
            {
                _vertexBuffer.reserve(2048);
                _indicesBuffer.reserve(2048);
            }
            //
            void drawLine( GeometryMemoryData* geomData, const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd, float width, uint32_t color );
            GeometryHandle drawRect( GeometryMemoryData* geomData, float x, float y, float width, float height, uint32_t color, bool dynamic = false );
        };

        class GeometryGPUDrawData {
            friend class GeometryMemoryData;
            struct DrawBatch {
                size_t              indexOffset;             // index offset
                size_t              indexCount;              // drawIndex 数量
                ugi::Buffer*        uniformBuffer;
                ugi::ArgumentGroup* argument;
            };
        private:
            GDIContext*                          _context;
            /// <summary>
            ///  GPU Resource                    
            /// </summary>
            ugi::Buffer*                        _vertexBuffer;
            ugi::Buffer*                        _indexBuffer;
            ugi::Drawable*                      _drawable;
            //
            std::vector<DrawBatch>              _batches;
        public:
            GeometryGPUDrawData(GDIContext* context);
            void updateGeometryTranslation( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation );
            void draw( RenderCommandEncoder* encoder );

            ~GeometryGPUDrawData();
        };

    }
}