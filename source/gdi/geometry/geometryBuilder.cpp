#include "geometryBuilder.h"
#include "geometryDefine.h"
#include "geometryDrawData.h"
#include "../gdi.h"

#include <ugi/Descriptor.h>
#include <ugi/Device.h>
#include <ugi/Pipeline.h>
#include <ugi/Buffer.h>
#include <ugi/Argument.h>
#include <ugi/CommandQueue.h>
#include <ugi/CommandBuffer.h>
#include <ugi/Drawable.h>

namespace ugi {

    namespace gdi {

        class GeometryBatch {
        protected:
            std::vector<GeometryVertex>             _vertices;          // 顶点数据，上限由外部指定
            std::vector<uint16_t>                   _indices;           // 和顶点数量上限一致
            std::vector<GeometryTransformArgument>  _uniformElements;   // UBO块 顶点的1/4
            bool checkVertex(size_t vertexCount, uint32_t maxVertex);
            bool checkUniform( uint32_t maxUniformElement );
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

            bool appendVertices(
                GeometryVertex const*               vertices,
                size_t                              vertexCount,
                uint16_t const*                     indices,
                size_t                              indexCount,
                const GeometryTransformArgument*    uniform,
                uint32_t                            maxVertex, 
                uint32_t                            maxUniformElement
            );

            // void setMaxBatchVertexCount( uint32_t maxVertexCount ) noexcept ;
            const GeometryVertex* vertexData() const noexcept;
            size_t vertexDataLength() const noexcept;
            const uint16_t* indexData() const noexcept;
            size_t indexDataLength() const noexcept;
            const void* uniformData() const noexcept;
            size_t uniformDataLength() const noexcept;
            size_t uniformElementCount() const  noexcept;
            //
            void reset() {
                _vertices.clear();
                _indices.clear();
                _uniformElements.resize(1);
            }
        };

        class GeometryGPUDrawData;

        class GeometryMemoryData {

        private:
            std::vector<GeometryBatch>              _geometryBatches;
            uint32_t                               _maxVertex;
            uint32_t                               _maxUniformElement;
        public:
            GeometryMemoryData()
                : _geometryBatches(1)
            {
            }

            void setMaxVertex( uint32_t count ) { _maxVertex = count; _maxUniformElement = _maxVertex/4; }

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

            void reset() {
                _geometryBatches.resize(1);
                _geometryBatches[0].reset();
                // for( auto& batch : _geometryBatches ) {
                //     batch.reset();
                // }
            }

            GeometryGPUDrawData* createGeometryDrawData( GDIContext* context );
        };

        class GeometryBuilder : public IGeometryBuilder {
        private:
            GDIContext*                     _context;
            std::vector<GeometryVertex>     _vertexBuffer;
            std::vector<uint16_t>           _indicesBuffer;
            //
            GeometryMemoryData              _geometryData;
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
            virtual void prepareBuildGeometry( uint32_t maxBatchVertex ) override;
            virtual GeometryGPUDrawData* endBuildGeometry() override;
            virtual void drawLine( const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd, float width, uint32_t color ) override;
            virtual GeometryHandle drawRect( float x, float y, float width, float height, uint32_t color, bool dynamic = false ) override;
        };
    }

    namespace gdi {

        GeometryHandle GeometryMemoryData::addGeometry(GeometryVertex const* vertices, size_t vertexCount, uint16_t const* indices, size_t indexCount ) {
            auto& last = _geometryBatches.back();
            if (!last.appendVertices(vertices, vertexCount, indices, indexCount, nullptr, _maxVertex, _maxUniformElement)) {
                _geometryBatches.emplace_back();
                bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, nullptr, _maxVertex, _maxUniformElement);
                assert(rst);
            }
            return 0;
        }
        
        GeometryHandle GeometryMemoryData::addGeometry( GeometryVertex const* vertices, size_t vertexCount, uint16_t const* indices, size_t indexCount, float angle, const hgl::Vector2f& scale, const hgl::Vector2f& anchor ) {
            float rad = angle / 180.0f * 3.1415926f;
            GeometryTransformArgument uniform(rad, scale, anchor);
            auto& last = _geometryBatches.back();
            if (!last.appendVertices(vertices, vertexCount, indices, indexCount, &uniform, _maxVertex, _maxUniformElement)) {
                _geometryBatches.emplace_back();
                bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, &uniform, _maxVertex, _maxUniformElement);
                assert(rst);
            }
            uint16_t batchIndex = (uint16_t)_geometryBatches.size() - 1;
            uint16_t elementIndex = (uint16_t)_geometryBatches.back().uniformElementCount() - 1;
            return (batchIndex<<16) | elementIndex;
        }

        GeometryGPUDrawData* GeometryMemoryData::createGeometryDrawData( GDIContext* context ) {
            GeometryGPUDrawData* drawBatch = new GeometryGPUDrawData(context);
            size_t vertexDataLength = 0;
            size_t indexDataLength = 0;

            std::vector<GeometryGPUDrawData::DrawBatch> drawBatches;
            std::vector<ugi::ArgumentGroup*> argumentGroups;

            auto pipeline = context->pipeline();
            const auto& pipelineDesc = context->pipelineDescription();

            for (auto& geomBatch : _geometryBatches) {
                // 分配 UBO 每个drawcall 1024 个 变换元素
                auto ubo = context->device()->createBuffer(ugi::BufferType::UniformBuffer, sizeof(GeometryTransformArgument) * _maxUniformElement);

                ubo->map(context->device()); // uniform buffer 支持 perssistent mapping
                memcpy( ubo->pointer(), geomBatch.uniformData(), geomBatch.uniformDataLength());

                auto argument = pipeline->createArgumentGroup();
                ResourceDescriptor contextInfoDescriptor; {
                    contextInfoDescriptor.type = ArgumentDescriptorType::UniformBuffer;
                    contextInfoDescriptor.bufferOffset = 0;
                    contextInfoDescriptor.bufferRange = 8; // 两个float
                    contextInfoDescriptor.descriptorHandle = argument->GetDescriptorHandle("ContextInfo", pipelineDesc);
                    contextInfoDescriptor.buffer = context->contextUniform();
                }
                ResourceDescriptor transformDescriptor; {
                    transformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
                    transformDescriptor.bufferOffset = 0;
                    transformDescriptor.bufferRange = sizeof(GeometryTransformArgument) * _maxUniformElement;
                    transformDescriptor.descriptorHandle = argument->GetDescriptorHandle("Transform", pipelineDesc);
                    transformDescriptor.buffer = ubo;
                }
                argument->updateDescriptor(contextInfoDescriptor);
                argument->updateDescriptor(transformDescriptor);
                argument->prepairResource();
                argumentGroups.push_back(argument);

                GeometryGPUDrawData::DrawBatch drawBatch = {
                    vertexDataLength /sizeof(GeometryVertex),       ///> vertex offset
                    indexDataLength / sizeof(uint16_t),             ///> index offset( first index )
                    geomBatch.indexDataLength() / sizeof(uint16_t), ///> index count
                    ubo,                                            ///> uniform buffer
                    argument                                        ///> argument
                };
                drawBatches.emplace_back(drawBatch);
                //
                vertexDataLength += geomBatch.vertexDataLength();
                indexDataLength += geomBatch.indexDataLength();
            }
            auto device = context->device();
            // 创建 vertexBuffer
            auto vertexBuffer = device->createBuffer(ugi::BufferType::VertexBuffer, vertexDataLength);
            auto indexBuffer = device->createBuffer(ugi::BufferType::IndexBuffer, indexDataLength);
            auto uploadQueue = device->transferQueues()[0];
            auto cmd = uploadQueue->createCommandBuffer(device);

            Buffer* stagingBuffer = device->createBuffer(BufferType::StagingBuffer, vertexBuffer->size() + indexBuffer->size());
            void* ptr = stagingBuffer->map(device);
            uint8_t* u8ptr = (uint8_t*)ptr;
            for (auto& geomBatch : _geometryBatches) {
                memcpy(u8ptr, geomBatch.vertexData(), geomBatch.vertexDataLength());
                u8ptr += geomBatch.vertexDataLength();
            }
            for (auto& geomBatch : _geometryBatches) {
                memcpy(u8ptr, geomBatch.indexData(), geomBatch.indexDataLength());
                u8ptr += geomBatch.indexDataLength();
            }
            stagingBuffer->unmap(device);
            cmd->beginEncode(); {
                auto resourceEncoder = cmd->resourceCommandEncoder(); {
                    BufferSubResource srcSubRes;
                    srcSubRes.offset = 0; srcSubRes.size = vertexBuffer->size();
                    resourceEncoder->updateBuffer(vertexBuffer, stagingBuffer, &srcSubRes, &srcSubRes);
                    srcSubRes.offset = vertexBuffer->size();
                    srcSubRes.size = indexBuffer->size();
                    BufferSubResource dstSubRes = srcSubRes;
                    dstSubRes.offset = 0;
                    resourceEncoder->updateBuffer(indexBuffer, stagingBuffer, &dstSubRes, &srcSubRes);
                    resourceEncoder->endEncode();
                }
                cmd->endEncode();
            }
            // 提交指令去执行
            
			QueueSubmitInfo submitInfo(&cmd, 1, nullptr, 0, nullptr, 0);
			QueueSubmitBatchInfo submitBatchInfo(&submitInfo, 1, nullptr);
            uploadQueue->submitCommandBuffers(submitBatchInfo);
            uploadQueue->waitIdle();
            // 销毁 staging buffer
            stagingBuffer->release(device);
            delete cmd;
            //
            auto drawable = new Drawable(1);
            drawable->setVertexBuffer(vertexBuffer, 0, 0);
            drawable->setIndexBuffer(indexBuffer, 0);

            drawBatch->_batches = std::move(drawBatches);
            drawBatch->_drawable = drawable;
            drawBatch->_indexBuffer = indexBuffer;
            drawBatch->_vertexBuffer = vertexBuffer;
            drawBatch->_maxVertex = _maxVertex;
            drawBatch->_maxUniformElement = _maxUniformElement;

            return drawBatch;
        }

        // void GeometryBatch::setMaxBatchVertexCount( uint32_t maxVertexCount ) noexcept {
        //     _maxVertex = maxVertexCount;
        //     _maxUniformElement = maxVertexCount >> 2;
        // }

        const GeometryVertex* GeometryBatch::vertexData() const  noexcept {
            if (_vertices.size()) {
                return _vertices.data();
            }
            else {
                return nullptr;
            }
        }
        size_t GeometryBatch::vertexDataLength() const  noexcept {
            return _vertices.size() * sizeof(GeometryVertex);
        }
        const uint16_t* GeometryBatch::indexData() const  noexcept {
            return _indices.data();
        }
        size_t GeometryBatch::indexDataLength() const  noexcept {
            return _indices.size() * sizeof(uint16_t);
        }
        const void* GeometryBatch::uniformData() const  noexcept {
            return _uniformElements.data();
        }
        size_t GeometryBatch::uniformDataLength() const  noexcept {
            return _uniformElements.size() * sizeof(GeometryTransformArgument);
        }
        size_t GeometryBatch::uniformElementCount() const  noexcept {
            return _uniformElements.size();
        }


        bool GeometryBatch::checkVertex(size_t vertexCount, uint32_t maxVertex) {
            return maxVertex - _vertices.size() >= vertexCount;
        }
        bool GeometryBatch::checkUniform( uint32_t maxUniform ) {
            return maxUniform > _uniformElements.size();
        }

        bool GeometryBatch::appendVertices(
            GeometryVertex const*               vertices,
            size_t                              vertexCount,
            uint16_t const*                     indices,
            size_t                              indexCount,
            const GeometryTransformArgument*    uniform,
            uint32_t                            maxVertex, 
            uint32_t                            maxUniformElement
        )
        {
            if (checkVertex(vertexCount,maxVertex)) {
                uint16_t uniformIndex = uniform ? (uint16_t)_uniformElements.size() : 0;
                if (uniformIndex != 0 && !checkUniform(maxUniformElement)) {
                    return false;
                }
                size_t vertexLocation = _vertices.size();
                size_t indexLocation = (uint16_t)_indices.size();
                // 处理顶点
                _vertices.resize(_vertices.size() + vertexCount);
                size_t sourceVericesIndex = 0;
                while (sourceVericesIndex < vertexCount) {
                    _vertices[vertexLocation + sourceVericesIndex] = vertices[sourceVericesIndex];
                    _vertices[vertexLocation + sourceVericesIndex].uniformElement = uniformIndex;
                    ++sourceVericesIndex;
                }
                _indices.resize(_indices.size() + indexCount);
                // 这里得更新一下索引
                size_t sourceIndicesIndex = 0;
                while (sourceIndicesIndex < indexCount) {
                    _indices[indexLocation + sourceIndicesIndex] = (uint16_t)vertexLocation + indices[sourceIndicesIndex];
                    ++sourceIndicesIndex;
                }
                // uniform
                if (uniformIndex) {
                    _uniformElements.push_back(*uniform);
                }
                return true;
            }
            else {
                return false;
            }
        }

        void GeometryBuilder::drawLine(
            const hgl::Vector2f&    pointStart, 
            const hgl::Vector2f&    pointEnd, 
            float                   width, 
            uint32_t                color
        )
        {
            // 思路，算出来直线向量，然后算垂直方向，加宽度
            hgl::Vector3f lineVector = hgl::Vector3f( pointStart - pointEnd, 0 );
            hgl::Vector3f orthVector = lineVector.Cross(hgl::Vector3f(0, 0, 1)).Normalized() * width / 2;
            // start,end分别生成两个点
            GeometryVertex points[4] = {
                { hgl::Vector2f(pointStart.x + orthVector.x, pointStart.y + orthVector.y), color, 0 }, 
                { hgl::Vector2f(pointStart.x - orthVector.x, pointStart.y - orthVector.y), color, 0 },
                { hgl::Vector2f(pointEnd.x - orthVector.x, pointEnd.y - orthVector.y),     color, 0 },
                { hgl::Vector2f(pointEnd.x + orthVector.x, pointEnd.y + orthVector.y),     color, 0 }
            };
            uint16_t indices[6] = { 0, 1, 3, 1, 2, 3 };
            _geometryData.addGeometry(points, 4, indices, 6);
        }

        GeometryHandle GeometryBuilder::drawRect(
            float x, float y, 
            float width, float height, 
            uint32_t color, 
            bool dynamic
        )
        {
            GeometryVertex points[4] = {
                { hgl::Vector2f( x, y ), color, 0 },
                { hgl::Vector2f( x + width, y ), color, 0 },
                { hgl::Vector2f( x + width, y + height ),     color, 0 },
                { hgl::Vector2f( x, y + height ),     color, 0 }
            };
            uint16_t indices[6] = { 0, 1, 3, 1, 2, 3 };
            _geometryData.addGeometry(
                points, 4, indices, 6, 
                0, 
                hgl::Vector2f(1.0f, 1.0f),
                hgl::Vector2f(0.0f, 0.0f)
            );
            return 0;
        }

        void GeometryBuilder::prepareBuildGeometry( uint32_t maxBatchVertex ) {
            _geometryData.setMaxVertex(maxBatchVertex);
            _geometryData.reset();
        }

        GeometryGPUDrawData* GeometryBuilder::endBuildGeometry() {
            auto drawData = _geometryData.createGeometryDrawData(_context);
            return drawData;
        }

        IGeometryBuilder* CreateGeometryBuilder( GDIContext* context ) {
            auto builder = new GeometryBuilder(context);
            return builder;
        }
        

    }

}