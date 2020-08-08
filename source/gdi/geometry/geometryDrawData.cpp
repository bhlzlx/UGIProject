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
    
        GeometryBatch::GeometryBatch( GeometryBatch&& batch ) noexcept
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

        GeometryDrawData::GeometryDrawData( GDIContext* context )
            : _context(context)
            , _vertexBuffer(nullptr)
            , _indexBuffer(nullptr)
            , _drawable(nullptr)
            , _batches {}
            , _contextInformation()
            , _transform ()
            , _translation{0.0f, 0.0f}
            , _colorMask(0xffffffff)
            , _extraFlags(0x00ffffff)
        {
        }

        void GeometryDrawData::draw(RenderCommandEncoder* encoder) {

            for (size_t i = 0; i < _batches.size(); i++)
            {
                encoder->bindArgumentGroup(_argGroups[i]);
                encoder->drawIndexed( _drawable, _batches[i]._firstIndex, _batches[i]._primitiveCount, _batches[i]._firstVertex);
            }

        }
        
        GeometryDrawData::~GeometryDrawData() {

            if (_vertexBuffer) {
                _context->device()->destroyBuffer(_vertexBuffer);
            }
            if (_indexBuffer) {
                _context->device()->destroyBuffer(_indexBuffer);
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

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& offset ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            hgl::Vector3f col[2] = {
                hgl::Vector3f( 1.0f, 0.0f, offset.x),
                hgl::Vector3f( 0.0f, 1.0f, offset.y),
            };
            _batches[batchIndex]._transArgBuffer[elementIndex].col1 = col[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].col2 = col[1];
        }

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector3f data[2];
            data[0] = hgl::Vector3f(a, 0, -a*x + x);
            data[1] = hgl::Vector3f(0, b, -b*y + y);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].col1 = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].col2 = data[1];
        }

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, float radian ){
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(radian); float sinValue = sin(radian);
            float x = anchor.x; float y = anchor.y;
            hgl::Vector3f data[2];
            data[0] = hgl::Vector3f(cosValue, -sinValue, -cosValue*x + sinValue*y + x);
            data[1] = hgl::Vector3f(sinValue, cosValue,  -sinValue*x - cosValue*y + y);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].col1 = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].col2 = data[1];
        }
        
        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation ) {
            //
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector3f data[2];
            /*
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector3f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x);
            data[1] = hgl::Vector3f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].col1 = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].col2 = data[1];
        }


        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian, const hgl::Vector2f& offset ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(radian); float sinValue = sin(radian);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector3f data[2];
            /*
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector3f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x);
            data[1] = hgl::Vector3f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y);
            //
            data[0].z += offset.x;
            data[1].z += offset.y;
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].col1 = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].col2 = data[1];
        }

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector3f(& matrix)[2] ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            float* ptr = &_batches[batchIndex]._transArgBuffer[elementIndex].data[0].x;
            memcpy( ptr, &matrix[0].x, sizeof(float)*3 );
            ptr = &_batches[batchIndex]._transArgBuffer[elementIndex].data[1].x;
            memcpy( ptr, &matrix[1].x, sizeof(float)*3 );
        }

        void GeometryDrawData::setElementColorMask( GeometryHandle handle, uint32_t colorMask ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            _batches[batchIndex]._transArgBuffer[elementIndex].colorMask = colorMask;
        }

        void GeometryDrawData::setElementExtraFlags( GeometryHandle handle, uint32_t flags ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            _batches[batchIndex]._transArgBuffer[elementIndex].extra = flags;
        }

        void GeometryDrawData::prepareResource( ResourceCommandEncoder* encoder, UniformAllocator* allocator ) {
            for( size_t i = 0; i<_batches.size(); ++i) {
                uint32_t uboSize = _batches[i]._transArgCount* sizeof(GeometryTransformArgument);
                auto ubo = allocator->allocate(uboSize);
                ubo.writeData(0, _batches[i]._transArgBuffer, uboSize);
                _elementInformationDescriptor.buffer = ubo.buffer();
                _elementInformationDescriptor.bufferOffset = ubo.offset();
                //
                _contextInformation.contextSize = _context->size();
                _contextInformation.transform = _transform;
                ubo = allocator->allocate(sizeof(_contextInformation));
                ubo.writeData(0, &_contextInformation, sizeof(_contextInformation));
                _globalInformationDescriptor.bufferOffset = ubo.offset();
                _globalInformationDescriptor.buffer = ubo.buffer();
                //
                _argGroups[i]->updateDescriptor(_elementInformationDescriptor);
                _argGroups[i]->updateDescriptor(_globalInformationDescriptor);
                _argGroups[i]->prepairResource(encoder);
            }
        }
        
        void GeometryDrawData::setTransform( const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian ) {
            float cosValue = cos(radian); float sinValue = sin(radian);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            _transform.col1 = hgl::Vector3f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x);
            _transform.col2 = hgl::Vector3f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y);
            //
            _contextInformation.transform = _transform;
            //
            _contextInformation.transform.col1.z += _translation.x;
            _contextInformation.transform.col2.z += _translation.y;
        }

        void GeometryDrawData::setTranslation( const hgl::Vector2f& translate ) {
            hgl::Vector2f offset = translate - _translation;
            _translation = translate;
            //
            _contextInformation.transform.col1.z += offset.x;
            _contextInformation.transform.col2.z += offset.y;
        }

        void GeometryDrawData::setColorMask( uint32_t colorMask ) {
            _contextInformation.transform.colorMask = colorMask;
        }
        
        void GeometryDrawData::setExtraFlags( uint32_t flags) {
            _contextInformation.transform.extra = flags;
        }

        void GeometryDrawData::setScissor( float left, float right, float top, float bottom ) {
            _contextInformation.contextSicssor = hgl::Vector4f( left, right, top, bottom );
        }

    }

}