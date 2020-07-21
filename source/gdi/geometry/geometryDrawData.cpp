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

        GeometryDrawData::GeometryDrawData( GDIContext* context )
            : _context(context)
            , _vertexBuffer(nullptr)
            , _indexBuffer(nullptr)
            , _batches {}
            , _contextInformation()
            , _transform { hgl::Vector4f(1.0, 0.0f, 0.0f, 1.0f), hgl::Vector4f(0.0, 1.0f, 0.0f, 0.0f) }
            , _translation{0.0f, 0.0f}
            , _alpha(1.0f)
            , _gray(0.0f)
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

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float rotation ) {
            //
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(rotation); float sinValue = sin(rotation);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector4f data[2];
            /*
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector4f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x, 0.0f);
            data[1] = hgl::Vector4f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y, 0.0f);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].data[0] = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].data[1] = data[1];
        }

        
        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector4f data[2];
            data[0] = hgl::Vector4f(a, 0, -a*x + x, _alpha);
            data[1] = hgl::Vector4f(0, b, -b*y + y, _gray);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].data[0] = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].data[1] = data[1];
        }
        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, float radian ){
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(radian); float sinValue = sin(radian);
            float x = anchor.x; float y = anchor.y;
            hgl::Vector4f data[2];
            data[0] = hgl::Vector4f(cosValue, -sinValue, -cosValue*x + sinValue*y + x, 0.0f);
            data[1] = hgl::Vector4f(sinValue, cosValue,  -sinValue*x - cosValue*y + y, 0.0f);
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].data[0] = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].data[1] = data[1];
        }

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector2f& anchor, const hgl::Vector2f& scale, float radian, const hgl::Vector2f& offset ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            //
            float cosValue = cos(radian); float sinValue = sin(radian);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            hgl::Vector4f data[2];
            /*
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector4f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x, 0.0f);
            data[1] = hgl::Vector4f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y, 0.0f);
            //
            data[0].z += offset.x;
            data[1].z += offset.y;
            //
            _batches[batchIndex]._transArgBuffer[elementIndex].data[0] = data[0];
            _batches[batchIndex]._transArgBuffer[elementIndex].data[1] = data[1];
        }

        void GeometryDrawData::setElementTransform( GeometryHandle handle, const hgl::Vector3f(& matrix)[2] ) {
            uint16_t batchIndex = handle>>16;
            uint16_t elementIndex = handle&0xffff;
            float* ptr = &_batches[batchIndex]._transArgBuffer[elementIndex].data[0].x;
            memcpy( ptr, &matrix[0].x, sizeof(float)*3 );
            ptr = &_batches[batchIndex]._transArgBuffer[elementIndex].data[1].x;
            memcpy( ptr, &matrix[1].x, sizeof(float)*3 );
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
                ubo = allocator->allocate(sizeof(_contextInformation));
                _contextInformation.contextSize = _context->size();
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
            _transform[0] = hgl::Vector4f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x, _alpha);
            _transform[1] = hgl::Vector4f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y, _gray);
            //
            _contextInformation.contextTransform[0] = _transform[0];
            _contextInformation.contextTransform[1] = _transform[1];
            //
            _contextInformation.contextTransform[0].z += _translation.x;
            _contextInformation.contextTransform[1].z += _translation.y;
        }

        void GeometryDrawData::setTranslation( const hgl::Vector2f& translate ) {
            _translation = translate;
            //
            _contextInformation.contextTransform[0] = _transform[0];
            _contextInformation.contextTransform[1] = _transform[1];
            //
            _contextInformation.contextTransform[0].z += _translation.x;
            _contextInformation.contextTransform[1].z += _translation.y;
        }

        void GeometryDrawData::setAlpha( float alpha ) {
            _alpha = alpha;
            _contextInformation.contextTransform[0].w = alpha;
        }

        void GeometryDrawData::setGray( float gray ) {
            _gray = gray;
            _contextInformation.contextTransform[1].w = gray;
        }

        void GeometryDrawData::setScissor( float left, float right, float top, float bottom ) {
            _contextInformation.contextSicssor = hgl::Vector4f( left, right, top, bottom );
        }

    }

}