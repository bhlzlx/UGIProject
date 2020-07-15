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

        constexpr uint32_t DEFAULT_GEOM_TRANSFORM_COUNT = 1024;

        class GeometryGPUDrawData;

        class GeometryBuilder : public IGeometryBuilder {
        private:
            GDIContext*                     _context;
            GeometryVertex*                 _vertexBuffer;          // vertex 缓存，一般不会销毁，重置后也可以复用
            uint32_t                        _vertexCapacity;        
            uint32_t                        _vertexCount;
            uint16_t*                       _indexBuffer;
            uint32_t                        _indexCapacity;
            uint32_t                        _indexCount;
            //
            uint32_t                        _maxArgPerDraw;
            std::vector<GeometryBatch>      _batches;
        public:
            GeometryBuilder(GDIContext* context)
                : _context(context)
                , _vertexBuffer{ new GeometryVertex[4096] }
                , _vertexCapacity(4096)
                , _vertexCount(0)
                , _indexBuffer{ new uint16_t[4096] }
                , _indexCapacity(4096)
                , _indexCount(0)
                , _batches {}
            {
            }
            //
            virtual void beginBuild() override;
            virtual GeometryGPUDrawData* endBuild() override;
            virtual void drawLine( const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd, float width, uint32_t color ) override;
            virtual GeometryHandle drawRect( float x, float y, float width, float height, uint32_t color, bool dynamic = false ) override;
            ///>
        private:
            uint32_t appendGeometryVertices( GeometryVertex const* vertices, uint32_t vertexCount, uint16_t const* indices, uint32_t indexCount, bool transform = false );
        };
    }

    namespace gdi {

        uint32_t GeometryBuilder::appendGeometryVertices( 
            GeometryVertex const* vertices, 
            uint32_t vertexCount, 
            uint16_t const* indices, 
            uint32_t indexCount, 
            bool transform 
        ) {
            // 处理 uniform buffer batch
            assert( _batches.size() );
            uint32_t argIndex = 0;
            if( transform ) {
                GeometryBatch* batch = &_batches.back();
                if(batch->_transArgCount >= _maxArgPerDraw) {
                    batch->_primitiveCount = _indexCount;
                    // 创建新batch
                    GeometryBatch newBatch(
                        _vertexCount, 
                        _indexCount,
                        0,
                        new GeometryTransformArgument[DEFAULT_GEOM_TRANSFORM_COUNT], 
                        DEFAULT_GEOM_TRANSFORM_COUNT
                    );
                    _batches.push_back(newBatch);
                    batch = &_batches.back();
                }
                if(batch->_transArgCount >= batch->_transArgCapacity) {
                    batch->_transArgBuffer = (GeometryTransformArgument*)realloc(batch->_transArgBuffer, batch->_transArgCapacity*2*sizeof(GeometryTransformArgument));
                    batch->_transArgCapacity*=2;
                }
                argIndex = batch->_transArgCount;
                batch->_transArgBuffer[batch->_transArgCount] = GeometryTransformArgument(); // 给个默认的单位矩阵
                batch->_transArgCount++;
            }
            // 处理顶点数据
            auto vertexLeft = _vertexCapacity - _vertexCount;
            auto indexLeft = _indexCapacity - _indexCount;
            if(vertexLeft<vertexCount) {
                _vertexBuffer = (GeometryVertex*)realloc(_vertexBuffer, (_vertexCapacity*1.5) * sizeof(GeometryVertex));
                assert(!vertxBuffer);
                _vertexCapacity *=1.5;
            }
            memcpy( _vertexBuffer+_vertexCount, vertices, vertexCount );
            // 处理顶点的uniform索引
            for( auto vertex = _vertexBuffer+_vertexCount; vertex<_vertexBuffer+_vertexCount+vertexCount; ++vertex ) {
                vertex->uniformElement = argIndex;
            }
            _vertexCount+=vertexCount;
            // 处理索引数据
            if( indexLeft < indexCount) {
                _indexBuffer = (uint16_t*)realloc(_indexBuffer, (_indexCapacity*1.5) * sizeof(uint16_t));
                assert(_indexBuffer);
                _indexCapacity *=1.5;
            }
            memcpy( _indexBuffer+indexCount, indices, indexCount );
            _vertexCount+=vertexCount;

            uint32_t handle = ((_batches.size()-1)<<16) | argIndex;
            return handle;
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
            this->appendGeometryVertices(points,4, indices, 6, false);
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
            auto handle = this->appendGeometryVertices(
                points, 4, indices, 6, true
            );
            return handle;
        }

        void GeometryBuilder::beginBuild() {

        }

        GeometryGPUDrawData* GeometryBuilder::endBuild() {
            GeometryGPUDrawData* drawData = new GeometryGPUDrawData(_context);
            auto device = _context->device();
            // std::vector<ugi::ArgumentGroup*> argumentGroups;

            auto pipeline = _context->pipeline();
            const auto& pipelineDesc = _context->pipelineDescription();

            for (auto& geomBatch : _batches) {
                // UBO 实际上用 ringbuffer 更好
                auto argument = pipeline->createArgumentGroup();
                ResourceDescriptor contextInfoDescriptor; {
                    contextInfoDescriptor.type = ArgumentDescriptorType::UniformBuffer;
                    contextInfoDescriptor.bufferOffset = 0;
                    contextInfoDescriptor.bufferRange = 8; // 两个float
                    contextInfoDescriptor.descriptorHandle = argument->GetDescriptorHandle("ContextInfo", pipelineDesc);
                    contextInfoDescriptor.buffer = _context->contextUniform();
                }
                argument->updateDescriptor(contextInfoDescriptor);
                drawData->_argGroups.push_back(argument);
            }
            drawData->_batches = std::move(_batches);
            // 创建 vertexBuffer
            auto vertexBuffer = device->createBuffer(ugi::BufferType::VertexBuffer, _vertexCount*sizeof(GeometryVertex));
            auto indexBuffer = device->createBuffer(ugi::BufferType::IndexBuffer, _indexCount*sizeof(uint16_t));
            auto uploadQueue = device->transferQueues()[0];
            auto cmd = uploadQueue->createCommandBuffer(device);

            Buffer* stagingBuffer = device->createBuffer(BufferType::StagingBuffer, vertexBuffer->size() + indexBuffer->size());
            void* ptr = stagingBuffer->map(device);
            uint8_t* u8ptr = (uint8_t*)ptr;
            memcpy(u8ptr, _vertexBuffer, vertexBuffer->size());
            u8ptr+=vertexBuffer->size();
            memcpy(u8ptr, _indexBuffer, indexBuffer->size());
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

            drawData->_drawable = drawable;
            drawData->_indexBuffer = indexBuffer;
            drawData->_vertexBuffer = vertexBuffer;
            ResourceDescriptor transformDescriptor; {
                transformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
                transformDescriptor.bufferOffset = 0;
                transformDescriptor.bufferRange = sizeof(GeometryTransformArgument) * _maxArgPerDraw;
                transformDescriptor.descriptorHandle = drawData->_argGroups[0]->GetDescriptorHandle("Transform", pipelineDesc);
                transformDescriptor.buffer = nullptr;
            }
            drawData->_transformDescriptor = transformDescriptor;
            return drawData;
        }

        IGeometryBuilder* CreateGeometryBuilder( GDIContext* context ) {
            auto builder = new GeometryBuilder(context);
            return builder;
        }
        

    }

}