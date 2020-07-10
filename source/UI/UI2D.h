#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>
#include <utility>

namespace ugi {

    namespace gdi {

        // 存储 GDI 设备上下文的状态
        // 先 使用GeometryBuilder 在CPU端组装顶点以及索引数据
        class GDIContext {
        private:
        public:
            GDIContext() {
            }
        };
        
        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        *  1. uniformElement 如果是0表明不旋转也不缩放
        *  2. color 这个属性实际上可以在shader里算最终颜色也可以提供一个颜色表来做映射（可能更快，目前先不这么做）
        *  3. position 的值如果想省CPU则可以放到GPU里作浮点运算（先放CPU里算）
        * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        struct GeometryVertex {
            hgl::Vector2f   position;           // 位置
            uint16_t        color;              // 颜色索引 R(16) G(16) B(16) A(16) 每个8位
            uint16_t        uniformElement;     // uniform 元素的索引
        };

        /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        *  uniform 里主要是存共用的值，比如旋转，缩放，
        *  一般画一个矩形或者多个复杂的形状缩放和角度都是相同的，这个值是可以复用的
        * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        struct GeometryUniformElement {
            union {
                float values[6];
                struct {
                    hgl::Vector4f  rotate;
                    hgl::Vector2f  scale;
                };
            };
            GeometryUniformElement()
                : values { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f } 
            {
            }
            bool isIdentity() const noexcept {
                constexpr float ID[6] = { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
                return memcmp(this, &ID, sizeof(ID)) == 0;
            }
        };

        class GeometryBatch {
        protected:
            std::vector<GeometryVertex>         _vertices;          // 顶点数据，上限为 4096，即大约1024个矩形
            std::vector<uint16_t>               _indices;           // 顶点索引上限为 4096 和顶点数量上限一致
            std::vector<GeometryUniformElement> _uniformElements;   // UBO块 1024
        public:
            GeometryBatch()
                : _vertices{}
                , _indices{}
                , _uniformElements{}
            {
                _vertices.reserve(4096);
                _indices.reserve(4096);
                _uniformElements.reserve(1024);
                // 第一个uniform默认就是不作任务变换
                _uniformElements[0].rotate[0] = 1; _uniformElements[0].rotate[1] = 0;
                _uniformElements[0].rotate[2] = 0; _uniformElements[0].rotate[3] = 1;
                _uniformElements[0].scale.x = _uniformElements[0].scale.y = 1.0f;
            }

            const GeometryVertex* vertexData() {
                if( _vertices.size()) {
                    return _vertices.data();
                } else {
                    return nullptr;
                }
            }
            size_t vertexDataLength() {
                return _vertices.size() * sizeof(GeometryVertex);
            }
            const uint16_t* indexData() {
                return _indices.data();
            }
            size_t indexDataLength() {
                return _indices.size() * sizeof(uint16_t);
            }
            const void* uniformData() {
                return _uniformElements.data();
            }
            size_t uniformDataLength() {
                return _uniformElements.size() * sizeof(GeometryUniformElement);
            }
        };

        class GeometryData {
            class GeometryBatchPrivate : public GeometryBatch {
                bool checkVertex( size_t vertexCount ) {
                    return 4096 - _vertices.size() >= vertexCount;
                }
                bool checkUniform() {
                    return 1024 > _uniformElements.size();
                }
            public:
                bool appendVertices(  
                    GeometryVertex const*           vertices, 
                    size_t                          vertexCount, 
                    uint16_t const*                 indices, 
                    size_t                          indexCount, 
                    const GeometryUniformElement*   uniform
                    ) 
                {
                    if( checkVertex(vertexCount)) {
                        uint16_t uniformIndex = uniform ? 0 : _uniformElements.size();
                        if( uniformIndex!=0 && !checkUniform()) {
                            return false;
                        }                        
                        size_t vertexLocation = _vertices.size();
                        size_t indexLocation = (uint16_t)_indices.size();
                        // 处理顶点
                        _vertices.resize( _vertices.size() + vertexCount );
                        size_t sourceVericesIndex = 0;
                        while(sourceVericesIndex<vertexCount) {
                            _vertices[vertexLocation+sourceVericesIndex] = vertices[sourceVericesIndex];
                            _vertices[vertexLocation+sourceVericesIndex].uniformElement = _uniformElements.size();
                            ++sourceVericesIndex;
                        }
                        _indices.resize( _indices.size() + indexCount );
                        // 这里得更新一下索引
                        size_t sourceIndicesIndex = 0;
                        while(sourceIndicesIndex<indexCount) {
                            _indices[indexLocation+sourceIndicesIndex] = vertexLocation + indices[sourceIndicesIndex];
                            ++sourceIndicesIndex;
                        }
                        // uniform
                        _uniformElements.push_back(*uniform);
                        return true;
                    } else {
                        return false;
                    }
                }
            };
        private:
            std::vector<GeometryBatchPrivate>      _geometryBatches;
        public:
            GeometryData()
                : _geometryBatches(1)
            {
            }

            void addGeometry( 
                GeometryVertex const* vertices, 
                size_t vertexCount, 
                uint16_t const* indices, 
                size_t indexCount
            ) {
                auto& last = _geometryBatches.back();
                if( !last.appendVertices(vertices, vertexCount, indices, indexCount, nullptr) ) {
                    _geometryBatches.emplace_back();
                    bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, nullptr);
                    assert(rst);
                }
            }

            void addGeometry( 
                GeometryVertex const* vertices, 
                size_t vertexCount, 
                uint16_t const* indices, 
                size_t indexCount, 
                float angle, 
                hgl::Vector2f scale 
            ) {
                GeometryUniformElement uniform;
                float rad = angle / 180.0f * 3.1415926f;
                uniform.rotate[0] = uniform.rotate[3] = cos(rad);
                uniform.rotate[2] = sin(rad);
                uniform.rotate[1] = -uniform.rotate[2];
                uniform.scale = scale;
                //
                auto& last = _geometryBatches.back();
                if( !last.appendVertices(vertices, vertexCount, indices, indexCount, &uniform ) ) {
                    _geometryBatches.emplace_back();
                    bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, nullptr);
                    assert(rst);
                }
            }
        };

        class GeometryBuilder {
        private:
        public:
            void drawLine( GDIContext* context, GeometryData* geomData, const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd );
            void drawRect( GDIContext* context, GeometryData* geomData, const hgl::RectScope2<float>& rect );
        };

    }
}