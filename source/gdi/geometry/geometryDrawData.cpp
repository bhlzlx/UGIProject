#include "geometryDrawData.h"
#include "../gdi.h"

#include <ugi/buffer.h>
#include <ugi/Device.h>
#include <ugi/Drawable.h>
#include <ugi/CommandBuffer.h>
#include <ugi/CommandQueue.h>
#include <ugi/Argument.h>
#include <ugi/UniformBuffer.h>

#include <hgl/assets/AssetsSource.h>
#include "geometryDefine.h"

namespace ugi {

    namespace gdi {
    
        GeometryBatch::GeometryBatch( GeometryBatch&& batch )
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

        GeometryBatch::GeometryBatch( uint32_t firstVertex, uint32_t firstIndex, uint32_t primitiveCount, GeometryTransformArgument* argBuff, uint32_t argCapacity )
                :_firstVertex(firstVertex)
                ,_firstIndex(firstIndex) 
                ,_primitiveCount(primitiveCount)
                ,_transArgBuffer(argBuff)  
                ,_transArgCount(1) // 第一个默认单位矩阵
                ,_transArgCapacity(argCapacity)
        {
            argBuff[0] = GeometryTransformArgument();
        }

        void GeometryBatch::reset() {
            _firstVertex = 0;
            _firstIndex = 0;
            _primitiveCount = 0;
            _transArgBuffer = nullptr;
            _transArgCount = 0;
            _transArgCapacity = 0;
        }

        GeometryGPUDrawData::GeometryGPUDrawData( GDIContext* context )
            : _context(context)
            , _vertexBuffer(nullptr)
            , _indexBuffer(nullptr)
            , _batches {}
        {
        }

        void GeometryGPUDrawData::draw(RenderCommandEncoder* encoder) {

            for (size_t i = 0; i < _batches.size(); i++)
            {
                encoder->bindArgumentGroup(_argGroups[i]);
                encoder->drawIndexed( _drawable, _batches[i]._firstIndex, _batches[i]._primitiveCount, _batches[i]._firstVertex);
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
                free(batch._transArgBuffer);
            }
            for( auto& argGroup : _argGroups) {
                delete argGroup;
            }
            
        }

        void GeometryGPUDrawData::updateGeometryTranslation( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation ) {
            // assert( (handle>>16) < _batches.size());
            // assert( (handle&0xffff) < _maxUniformElement );
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;

            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector4f data[2];

            /*
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector4f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x, 0.0f);
            data[1] = hgl::Vector4f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y, 0.0f);
            // uint8_t* ptr = (uint8_t*)_batches[batchIndex].uniformBuffer->pointer();
            // memcpy( ptr+sizeof(data)*elementIndex , &data, sizeof(data));
        }

        void GeometryGPUDrawData::prepareResource( ResourceCommandEncoder* encoder, UniformAllocator* allocator ) {
            for( size_t i = 0; i<_batches.size(); ++i) {
                uint32_t uboSize = _batches[i]._transArgCount* sizeof(GeometryTransformArgument);
                auto ubo = allocator->allocate(uboSize);
                ubo.writeData(0, _batches[i]._transArgBuffer, uboSize);
                _transformDescriptor.buffer = ubo.buffer();
                _transformDescriptor.bufferOffset = ubo.offset();
                //
                _argGroups[i]->updateDescriptor(_transformDescriptor);
                _argGroups[i]->prepairResource(encoder);
            }
        }
        
    }

}