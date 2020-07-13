#include "UI2D.h"
#include <ugi/Device.h>
#include <ugi/CommandQueue.h>
#include <ugi/commandBuffer.h>
#include <ugi/Drawable.h>
#include <ugi/Pipeline.h>
#include <ugi/Argument.h>

namespace ugi {
    namespace gdi {
        GeometryTransformArgument::GeometryTransformArgument()
            : transformMatrix(hgl::Matrix3f::identity)
        {
        }

        GeometryTransformArgument::GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor)
        {
            hgl::Matrix3f A = {
                1.0f, 0.0f, -anchor.x,
                0.0f, 1.0f, -anchor.y,
                0.0f, 0.0f, 1.0f,
            };
            hgl::Matrix3f B = {
                cos(rad), -sin(rad), 0,
                sin(rad), cos(rad), 0,
                0.0f, 0.0f, 1.0f,
            };
            hgl::Matrix3f C = {
                scale.x, 0, 0,
                0, scale.y, 0,
                0, 0,       1
            };
            hgl::Matrix3f D = {
                1.0f, 0.0f, anchor.x,
                0.0f, 1.0f, anchor.y,
                0.0f, 0.0f, 1.0f,
            };
            transformMatrix = A * B * C * D;
            /*
            float cosValue = cos(rad); float sinValue = sin(rad);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            transformMatrix = {
                a * cosValue, -b * sinValue, x * a * cosValue - y * b * sinValue,
                a * sinValue, -b * cosValue, x * a * sinValue + y * b * cosValue,
                0         , 0          , 1
            };*/
        }
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


        bool GeometryMemoryData::GeometryBatchPrivate::checkVertex(size_t vertexCount) {
            return 4096 - _vertices.size() >= vertexCount;
        }
        bool GeometryMemoryData::GeometryBatchPrivate::checkUniform() {
            return 1024 > _uniformElements.size();
        }

        bool GeometryMemoryData::GeometryBatchPrivate::appendVertices(
            GeometryVertex const*               vertices,
            size_t                              vertexCount,
            uint16_t const*                     indices,
            size_t                              indexCount,
            const GeometryTransformArgument*    uniform
        )
        {
            if (checkVertex(vertexCount)) {
                uint16_t uniformIndex = uniform ? _uniformElements.size() : 0;
                if (uniformIndex != 0 && !checkUniform()) {
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
                    _indices[indexLocation + sourceIndicesIndex] = vertexLocation + indices[sourceIndicesIndex];
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
            GeometryMemoryData*           geomData, 
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
            geomData->addGeometry(points, 4, indices, 6);
        }

        GeometryHandle GeometryBuilder::drawRect(
            GeometryMemoryData* geomData, 
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
            geomData->addGeometry(
                points, 4, indices, 6, 
                0, 
                hgl::Vector2f(1.0f, 1.0f),
                hgl::Vector2f(0.0f, 0.0f)
            );
            return 0;
        }
        
        GeometryGPUDrawData::GeometryGPUDrawData( GDIContext* context )
            : _context(context)
            , _vertexBuffer(nullptr)
            , _indexBuffer(nullptr)
            , _batches {}
        {
        }

        void GeometryGPUDrawData::draw(RenderCommandEncoder* encoder) {
            for (const auto& batch : _batches) {
                encoder->bindArgumentGroup(batch.argument);
                encoder->drawIndexed(_drawable, batch.indexOffset, batch.indexCount);
            }
        }
        
        GeometryGPUDrawData::~GeometryGPUDrawData() {

            if (_vertexBuffer) {
                _vertexBuffer->release(_context->device());
            }
            if (_indexBuffer) {
                _indexBuffer->release(_context->device());
            }
            if (_drawable) {
                delete _drawable;
            }
            for( auto& batch : _batches) {
                delete batch.argument;
                batch.uniformBuffer->release(_context->device());
            }
        }

        GeometryHandle GeometryMemoryData::addGeometry(GeometryVertex const* vertices, size_t vertexCount, uint16_t const* indices, size_t indexCount) {
            auto& last = _geometryBatches.back();
            if (!last.appendVertices(vertices, vertexCount, indices, indexCount, nullptr)) {
                _geometryBatches.emplace_back();
                bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, nullptr);
                assert(rst);
            }
            return 0;
        }
        
        GeometryHandle GeometryMemoryData::addGeometry(GeometryVertex const* vertices, size_t vertexCount, uint16_t const* indices, size_t indexCount, float angle, const hgl::Vector2f& scale, const hgl::Vector2f& anchor) {
            float rad = angle / 180.0f * 3.1415926f;
            GeometryTransformArgument uniform(rad, scale, anchor);
            auto& last = _geometryBatches.back();
            if (!last.appendVertices(vertices, vertexCount, indices, indexCount, &uniform)) {
                _geometryBatches.emplace_back();
                bool rst = _geometryBatches.back().appendVertices(vertices, vertexCount, indices, indexCount, &uniform);
                assert(rst);
            }
            uint16_t batchIndex = _geometryBatches.size() - 1;
            uint16_t elementIndex = _geometryBatches.back().uniformElementCount() - 1;
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
                auto ubo = context->device()->createBuffer(ugi::BufferType::UniformBuffer, sizeof(GeometryTransformArgument) * 1024);

                ubo->map(context->device()); // uniform buffer 支持 perssistent mapping
                memcpy( ubo->pointer(), geomBatch.uniformData(), geomBatch.uniformDataLength());
                ubo->unmap(context->device());
                ubo->map(context->device());

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
                    transformDescriptor.bufferRange = sizeof(GeometryTransformArgument) * 1024;
                    transformDescriptor.descriptorHandle = argument->GetDescriptorHandle("Transform", pipelineDesc);
                    transformDescriptor.buffer = ubo;
                }
                argument->updateDescriptor(contextInfoDescriptor);
                argument->updateDescriptor(transformDescriptor);
                argument->prepairResource();
                argumentGroups.push_back(argument);

                GeometryGPUDrawData::DrawBatch drawBatch = {
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
            QueueSubmitBatchInfo submitBatchInfo;
            QueueSubmitInfo submitInfo;
            submitBatchInfo.fenceToSignal = nullptr;
            submitBatchInfo.submitInfoCount = 1;
            submitBatchInfo.submitInfos = &submitInfo;
            submitInfo.commandBuffers = &cmd;
            submitInfo.commandCount = 1;
            uploadQueue->submitCommandBuffers(submitBatchInfo);
            uploadQueue->waitIdle();
            // 销毁 staging buffer
            stagingBuffer->release(device);
            //
            auto drawable = new Drawable(1);
            drawable->setVertexBuffer(vertexBuffer, 0, 0);
            drawable->setIndexBuffer(indexBuffer, 0);

            drawBatch->_batches = std::move(drawBatches);
            drawBatch->_drawable = drawable;
            drawBatch->_indexBuffer = indexBuffer;
            drawBatch->_vertexBuffer = vertexBuffer;

            return drawBatch;
        }

        void GeometryGPUDrawData::updateGeometryTranslation( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation ) {
            assert( handle>>16 < _batches.size());
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;

            hgl::Matrix3f A = {
                1.0f, 0.0f, -anchor.x,
                0.0f, 1.0f, -anchor.y,
                0.0f, 0.0f, 1.0f
            };
            hgl::Matrix3f B = {
                cos(rotation), -sin(rotation), 0,
                sin(rotation), cos(rotation), 0,
                0.0f, 0.0f, 1.0f
            };
            hgl::Matrix3f C = {
                scale.x, 0, 0,
                0, scale.y, 0,
                0, 0,       1
            };
            hgl::Matrix3f D = {
                1.0f, 0.0f, anchor.x,
                0.0f, 1.0f, anchor.y,
                0.0f, 0.0f, 1.0f,
            };
            hgl::Matrix3f transformMatrix = A * B * C * D;
            /*
            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Matrix3f transformMatrix = {
                a * cosValue, -b * sinValue, x * a * cosValue - y * b * sinValue,
                a * sinValue, -b * cosValue, x * a * sinValue + y * b * cosValue,
                0         , 0          , 1
            };
            */
            uint8_t* ptr = (uint8_t*)_batches[batchIndex].uniformBuffer->pointer();
            memcpy( ptr+sizeof(GeometryTransformArgument)*elementIndex , &transformMatrix, sizeof(transformMatrix));
        }

        bool GDIContext::initialize()
        {
            hgl::io::InputStream* pipelineFile = _assetsSource->Open( hgl::UTF8String("/shaders/GDITest/pipeline.bin") );
            if (!pipelineFile) {
                return false;
            }
            auto pipelineFileSize = pipelineFile->GetSize();
            auto mem = malloc(pipelineFileSize);
            char* pipelineBuffer = (char*)mem;
            pipelineFile->ReadFully(pipelineBuffer, pipelineFileSize);
            PipelineDescription& pipelineDesc = *(PipelineDescription*)pipelineBuffer;
            pipelineBuffer += sizeof(PipelineDescription);
            for (auto& shader : pipelineDesc.shaders) {
                if (shader.spirvData) {
                    shader.spirvData = (uint64_t)pipelineBuffer;
                    pipelineBuffer += shader.spirvSize;
                }
            }
            pipelineDesc.pologonMode = PolygonMode::Fill;
            pipelineDesc.topologyMode = TopologyMode::TriangleList;
            pipelineDesc.renderState.cullMode = CullMode::None;
            pipelineDesc.renderState.blendState.enable = true;
            _pipeline = _device->createPipeline(pipelineDesc);
            _pipelineDesc = *(ugi::PipelineDescription*)mem;
            free(mem);
            if (!_pipeline) {
                return false;
            }
            _contextInfo = _device->createBuffer(BufferType::UniformBuffer, 8);
            return true;
        }

        ugi::Pipeline* GDIContext::pipeline() const noexcept
        {
            return _pipeline;
        }

        const ugi::PipelineDescription& GDIContext::pipelineDescription() const noexcept {
            return _pipelineDesc;
        }

        ugi::Buffer* GDIContext::contextUniform() const noexcept {
            return _contextInfo;
        }

        void GDIContext::setSize(const hgl::Vector2f& size)
        {
            _size = size;
            _contextInfo->map(_device);
            memcpy(_contextInfo->pointer(), &size, sizeof(size));
            _contextInfo->unmap(_device);
        }
    }
}