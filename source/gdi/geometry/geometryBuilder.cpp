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

        class GeometryDrawData;

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
                , _maxArgPerDraw(1024)
                , _batches {}
            {
            }
            //
            virtual void beginBuild() override;
            virtual GeometryDrawData* endBuild() override;
            virtual void drawLine( const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd, float width, uint32_t color ) override;
            virtual GeometryHandle drawRect( float x, float y, float width, float height, uint32_t color, bool dynamic = false ) override;
            virtual GeometryHandle drawVertices(const GeometryVertex* vertices, uint32_t vertexCount, const uint16_t* indices, uint32_t indexCount) override {
                return 0;
            }
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
            GeometryBatch* batch = &_batches.back();
            uint32_t argIndex = 0;
            if( transform ) {
                if(batch->_transArgCount >= _maxArgPerDraw) {
                    // 创建新batch
                    GeometryBatch newBatch(
                        _vertexCount, 
                        _indexCount,
                        0,
                        new GeometryTransformArgument[DEFAULT_GEOM_TRANSFORM_COUNT], 
                        DEFAULT_GEOM_TRANSFORM_COUNT
                    );
                    _batches.push_back( std::move(newBatch) );
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
            // 注意这个 indexDisplacement
            // 比如说现在已经有一四个顶点，对应6个索引，画的三角形List,索引内容是  0 1 3 1 2 3, 这次我们又添加了同样的几何内容
            // 这次添加的索引依然是 0 1 3 1 2 3 原封不动复制过来显然是不行的，我们要给索引加偏移，让它变成 4 5 7 5 6 7
            // 所以这个添加的值实际上是现有的顶点数量，即如下代码所示
            uint32_t indexDisplacement = _vertexCount;{
            if(vertexLeft<vertexCount) {
                _vertexBuffer = (GeometryVertex*)realloc(_vertexBuffer, (_vertexCapacity*1.5) * sizeof(GeometryVertex));
                assert(!_vertexBuffer);
                _vertexCapacity *=1.5;
            }
            memcpy( _vertexBuffer+_vertexCount, vertices, vertexCount*sizeof(GeometryVertex) );
            // 处理顶点的uniform索引
            {
                auto iter = _vertexBuffer + _vertexCount;
                for( uint32_t i = 0; i<vertexCount; ++i ) {
                    iter[i].uniformElement = argIndex;
                }
            }
            _vertexCount+=vertexCount;
            // 处理索引数据
            if( indexLeft < indexCount) {
                _indexBuffer = (uint16_t*)realloc(_indexBuffer, (_indexCapacity*1.5) * sizeof(uint16_t));
                assert(_indexBuffer);
                _indexCapacity *=1.5;
            }
            memcpy( _indexBuffer+_indexCount, indices, indexCount*sizeof(uint16_t) );
                auto iter = _indexBuffer + _indexCount;
                for( uint32_t i = 0; i<indexCount; ++i ) {
                    iter[i] += indexDisplacement;
                }
            }
            _indexCount+=indexCount;

            batch->_primitiveCount += indexCount;

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
            // uint32_t firstVertex, uint32_t firstIndex, uint32_t primitiveCount, GeometryTransformArgument* argBuff, uint32_t argCapacity
            GeometryTransformArgument* argBuff = (GeometryTransformArgument*)malloc(sizeof(GeometryTransformArgument)* 512);
            _batches.emplace_back(
                0, 0, 0,
                argBuff,
                512
            );
            _indexCount = _vertexCount = 0;
        }

        GeometryDrawData* GeometryBuilder::endBuild() {
            GeometryDrawData* drawData = new GeometryDrawData(_context);
            auto device = _context->device();
            // std::vector<ugi::ArgumentGroup*> argumentGroups;

            auto pipeline = _context->pipeline();
            const auto& pipelineDesc = _context->pipelineDescription();

            // GDI 的 Argument实际上目前只有uniform
            for( size_t i = 0; i<_batches.size(); ++i) {
                // UBO 实际上用 ringbuffer 更好
                auto argument = pipeline->createArgumentGroup();
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
			uploadQueue->destroyCommandBuffer(device, cmd);
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
                transformDescriptor.descriptorHandle = drawData->_argGroups[0]->GetDescriptorHandle("ElementInformation", pipelineDesc);
                transformDescriptor.buffer = nullptr;
            }
            ResourceDescriptor globalInfoDescriptor; {
                globalInfoDescriptor.type = ArgumentDescriptorType::UniformBuffer;
                globalInfoDescriptor.bufferOffset = 0;
                globalInfoDescriptor.bufferRange = sizeof(ContextInformation);
                globalInfoDescriptor.descriptorHandle = drawData->_argGroups[0]->GetDescriptorHandle("GlobalInformation", pipelineDesc);
                globalInfoDescriptor.buffer = nullptr;
            }
            drawData->_elementInformationDescriptor = transformDescriptor;
            drawData->_globalInformationDescriptor = globalInfoDescriptor;
            return drawData;
        }

        IGeometryBuilder* CreateGeometryBuilder( GDIContext* context ) {
            auto builder = new GeometryBuilder(context);
            return builder;
        }
        

    }

}