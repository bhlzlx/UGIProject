#pragma once
#include <ugi/UGIDeclare.h>
#include <vector>
#include <cstdint>
#include <hgl/math/Vector.h>

namespace ugi {

    namespace gdi {

        typedef uint32_t GeometryHandle;

        class GDIContext;

        class GeometryGPUDrawData {
            friend class GeometryMemoryData;
            struct DrawBatch {
                size_t              vertexOffset;            // vertex offset
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
            uint32_t                            _maxVertex;
            uint32_t                            _maxUniformElement;
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