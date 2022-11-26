#include "SDFFontRenderer.h"
#include "SDFTextureTileManager.h"
#include <hgl/assets/AssetsSource.h>
#include <ugi/device.h>
#include <ugi/buffer.h>
#include <ugi/flight_cycle_invoker.h>
#include <ugi/descriptor_binder.h>
#include <ugi/pipeline.h>
#include <ugi/Drawable.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/command_encoder/resource_cmd_encoder.h>
#include <ugi/command_encoder/render_cmd_encoder.h>
#include <hgl/io/InputStream.h>

#include <json.hpp>

namespace ugi {

    SDFFontRenderer::SDFFontRenderer()
        : _device(nullptr)
        , _texTileManager(nullptr)
        , _indicesHandle(0xffffffff)
        , _effectsHandle(0xffffffff)
        , _transformsHandle(0xffffffff)
        , _contextHandle(0xffffffff)
        , _texArraySamplerHandle(0xffffffff)
        , _texArrayHandle(0xffffffff)
    {
    }
    
    bool SDFFontRenderer::initialize( Device* device, hgl::assets::AssetsSource* assetsSource, const SDFRenderParameter& sdfParam ) {
        _device = device;
        _assetsSource = assetsSource;
        _resourceManager = new ResourceManager(device);
        _texTileManager = new SDFTextureTileManager();
        //
        _sdfParameter = sdfParam;
        nlohmann::json js;
        auto inputStream = assetsSource->Open("./shaders/sdf2d/sdf.json");
        if(inputStream) {
            char* configBuffer = new char[inputStream->GetSize()+1];
            configBuffer[inputStream->GetSize()] = 0;
            inputStream->ReadFully(configBuffer, inputStream->GetSize());
            js = nlohmann::json::parse(configBuffer);
            _sdfParameter.texArraySize = js["textureSize"].get<uint32_t>();
            _sdfParameter.texLayer = js["layer"];
            _sdfParameter.destinationSDFSize = js["sdfSize"];
            _sdfParameter.sourceFontSize = js["sourceSize"];
            _sdfParameter.extraSourceBorder = js["extraBorder"];
            _sdfParameter.searchDistance = js["searchDistance"];
            inputStream->Close();
        }

        // Create Pipeline
        hgl::io::InputStream* pipelineFile = assetsSource->Open( hgl::UTF8String("/shaders/sdf2d/pipeline.bin"));
        auto pipelineFileSize = pipelineFile->GetSize();
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->ReadFully(pipelineBuffer,pipelineFileSize);
        _pipelineDescription = *(pipeline_desc_t*)pipelineBuffer;
        pipelineBuffer += sizeof(pipeline_desc_t);
        for( auto& shader : _pipelineDescription.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        //_pipelineDescription.vertexLayout.buffers[2].
        _pipelineDescription.pologonMode = polygon_mode_t::Fill;
        _pipelineDescription.topologyMode = topology_mode_t::TriangleList;
        _pipelineDescription.renderState.cullMode = CullMode::None;
        _pipelineDescription.renderState.blendState.enable = true;
        _pipeline = _device->createGraphicsPipeline(_pipelineDescription);
        pipelineFile->Close();
        if( !_pipeline) {
            return false;
        }
        bool rst = _texTileManager->initialize( device, assetsSource, 
            _sdfParameter.destinationSDFSize, _sdfParameter.texArraySize, _sdfParameter.texLayer, 
            _sdfParameter.sourceFontSize, _sdfParameter.extraSourceBorder, _sdfParameter.searchDistance
        );
        if(!rst) {
            return false;
        }
        return true;
    }

    void SDFFontRenderer::beginBuild() {
        _meshVBO.clear();
        _meshIBO.clear();
        // === 
        _indices.clear();
        _styles.clear();
        _transforms.clear();
        //
    }
    
    IndexHandle SDFFontRenderer::appendText( float x, float y, SDFChar* text, uint32_t length, const Transform& transform, const Style& style, hgl::RectScope2f& rect ) {

        uint32_t transformIndex = _transforms.size();
        uint32_t styleIndex = _styles.size();
        _transforms.push_back(transform);
        _styles.push_back(style);

        rect.SetLeft(x);

        IndexHandle indexHandle;
        indexHandle.styleIndex = styleIndex;
        indexHandle.transformIndex = transformIndex;
        //
        for( uint32_t i = 0; i<length; ++i ) {
            const SDFChar& ch = text[i];
            //
            uint32_t indexID = (uint32_t)_indices.size();
            uint32_t styleIndex = 0;
            //
            GlyphKey glyphKey;
            glyphKey.charCode = ch.charCode;
            glyphKey.fontID = ch.fontID;
            GlyphInfo* glyph = _texTileManager->getGlyph(glyphKey);
            //
            // 64.0f / (float)ch.fontSize
            float targetFontSize = ch.fontSize;
            //  因为 float origionFontSize = 96.0f;（我们采样的时候写死了）
            // float originWidth = glyph->bitmapWidth / glyph->SDFScale; 算回原始宽度的公式
            // float destWidth = (targetFontSize / 96.0f) * originWidth; 目标宽度
            // float destWidth = (targetFontSize / 96.0f) * glyph->bitmapWidth / glyph->SDFScale; 就等于
            // float scaleFactor = destWidth / glyph->bitmapWidth; 而这个系数
            // 而 glyph->bitmapWidth = destWidth * glyph->SDFScale / targetFontSize * 96.0f
            // 所以 scaleFactor = destWidth / (destWidth * glyph->SDFScale / targetFontSize * 96.0f)
            // 即 scaleFactor = targetFontSize / ( glyph->SDFScale*96.0f)
            float scaleFactor = targetFontSize / (glyph->SDFScale*(float)_sdfParameter.sourceFontSize );
            
            float posLeft = x, posTop = y;
            posLeft += glyph->bitmapBearingX * scaleFactor;
            posTop += glyph->bitmapBearingY * scaleFactor;
            float posRight = posLeft + glyph->bitmapWidth * scaleFactor;
            float posBottom = posTop + glyph->bitmapHeight * scaleFactor;
            //
            x += glyph->bitmapAdvance*scaleFactor;
            //
            float uvLeft = glyph->texU;
            float uvRight = glyph->texU + glyph->texWidth;
            float uvTop = glyph->texV;
            float uvBottom = glyph->texV + glyph->texHeight;

            FontMeshVertex rect[4] = {
                { {posLeft, posTop}, {uvLeft, uvTop} },
                { {posRight, posTop}, {uvRight, uvTop} },
                { {posRight, posBottom}, {uvRight, uvBottom} },
                { {posLeft, posBottom}, {uvLeft, uvBottom} },
            };
            
            uint16_t baseIndex = _meshIBO.size()? _meshIBO.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0u, baseIndex + 1u, baseIndex + 2u, baseIndex + 0u, baseIndex + 2u, baseIndex + 3u, 
            };
            for( auto index : indices ) {
                _meshIBO.push_back(index);
            }
            _meshVBO.push_back(rect[0]);
            _meshVBO.push_back(rect[1]);
            _meshVBO.push_back(rect[2]);
            _meshVBO.push_back(rect[3]);
            //
            _indices.push_back(indexHandle);
        }
        //
        rect.SetRight(x);

        return indexHandle;
    }

    IndexHandle SDFFontRenderer::appendTextReuseTransform( 
        float x, float y, SDFChar* text, uint32_t length, 
        IndexHandle reuseHandle, const Style& style, hgl::RectScope2f& rect
    ) {

        rect.SetLeft(x);
        uint32_t styleIndex = _styles.size();
        _styles.push_back(style);
        reuseHandle.styleIndex = styleIndex;
        //
        for( uint32_t i = 0; i<length; ++i ) {
            const SDFChar& ch = text[i];
            //
            uint32_t indexID = (uint32_t)_indices.size();
            uint32_t styleIndex = 0;
            //
            GlyphKey glyphKey;
            glyphKey.charCode = ch.charCode;
            glyphKey.fontID = ch.fontID;
            GlyphInfo* glyph = _texTileManager->getGlyph(glyphKey);
            float targetFontSize = ch.fontSize;
            float scaleFactor = targetFontSize / (glyph->SDFScale*(float)_sdfParameter.sourceFontSize );
            
            float posLeft = x, posTop = y;
            posLeft += glyph->bitmapBearingX * scaleFactor;
            posTop += glyph->bitmapBearingY * scaleFactor;
            float posRight = posLeft + glyph->bitmapWidth * scaleFactor;
            float posBottom = posTop + glyph->bitmapHeight * scaleFactor;
            //
            x += glyph->bitmapAdvance*scaleFactor;
            //
            float uvLeft = glyph->texU;
            float uvRight = glyph->texU + glyph->texWidth;
            float uvTop = glyph->texV;
            float uvBottom = glyph->texV + glyph->texHeight;

            FontMeshVertex rect[4] = {
                { {posLeft, posTop}, {uvLeft, uvTop} },
                { {posRight, posTop}, {uvRight, uvTop} },
                { {posRight, posBottom}, {uvRight, uvBottom} },
                { {posLeft, posBottom}, {uvLeft, uvBottom} },
            };
            
            uint16_t baseIndex = _meshIBO.size()? _meshIBO.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0u, baseIndex + 1u, baseIndex + 2u, baseIndex + 0u, baseIndex + 2u, baseIndex + 3u, 
            };
            for( auto index : indices ) {
                _meshIBO.push_back(index);
            }
            _meshVBO.push_back(rect[0]);
            _meshVBO.push_back(rect[1]);
            _meshVBO.push_back(rect[2]);
            _meshVBO.push_back(rect[3]);
            //
            _indices.push_back(reuseHandle);
        }
        rect.SetRight(x);

        return reuseHandle;
    }

    IndexHandle SDFFontRenderer::appendTextReuseStyle ( 
        float x, float y, SDFChar* text, uint32_t length, 
        IndexHandle reuseHandle, const Transform& transform, hgl::RectScope2f& rect
    ) {
        uint32_t transformIndex = _transforms.size();
        _transforms.push_back(transform);
        reuseHandle.transformIndex = transformIndex;

        rect.SetLeft(x);
        //
        for( uint32_t i = 0; i<length; ++i ) {
            const SDFChar& ch = text[i];
            //
            uint32_t indexID = (uint32_t)_indices.size();
            uint32_t styleIndex = 0;
            //
            GlyphKey glyphKey;
            glyphKey.charCode = ch.charCode;
            glyphKey.fontID = ch.fontID;
            GlyphInfo* glyph = _texTileManager->getGlyph(glyphKey);
            float targetFontSize = ch.fontSize;
            float scaleFactor = targetFontSize / (glyph->SDFScale*(float)_sdfParameter.sourceFontSize );
            
            float posLeft = x, posTop = y;
            posLeft += glyph->bitmapBearingX * scaleFactor;
            posTop += glyph->bitmapBearingY * scaleFactor;
            float posRight = posLeft + glyph->bitmapWidth * scaleFactor;
            float posBottom = posTop + glyph->bitmapHeight * scaleFactor;
            //
            x += glyph->bitmapAdvance*scaleFactor;
            //
            float uvLeft = glyph->texU;
            float uvRight = glyph->texU + glyph->texWidth;
            float uvTop = glyph->texV;
            float uvBottom = glyph->texV + glyph->texHeight;

            FontMeshVertex rect[4] = {
                { {posLeft, posTop}, {uvLeft, uvTop} },
                { {posRight, posTop}, {uvRight, uvTop} },
                { {posRight, posBottom}, {uvRight, uvBottom} },
                { {posLeft, posBottom}, {uvLeft, uvBottom} },
            };
            
            uint16_t baseIndex = _meshIBO.size()? _meshIBO.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0u, baseIndex + 1u, baseIndex + 2u, baseIndex + 0u, baseIndex + 2u, baseIndex + 3u, 
            };
            for( auto index : indices ) {
                _meshIBO.push_back(index);
            }
            _meshVBO.push_back(rect[0]);
            _meshVBO.push_back(rect[1]);
            _meshVBO.push_back(rect[2]);
            _meshVBO.push_back(rect[3]);
            _indices.push_back(reuseHandle);
        }
        rect.SetRight(x);

        return reuseHandle;
    }

    IndexHandle SDFFontRenderer::appendTextReuse ( 
        float x, float y, SDFChar* text, uint32_t length, 
        IndexHandle reuseHandle, hgl::RectScope2f& rect
    ) {
        rect.SetLeft(x);
        for( uint32_t i = 0; i<length; ++i ) {
            const SDFChar& ch = text[i];
            //
            uint32_t indexID = (uint32_t)_indices.size();
            uint32_t styleIndex = 0;
            //
            GlyphKey glyphKey;
            glyphKey.charCode = ch.charCode;
            glyphKey.fontID = ch.fontID;
            GlyphInfo* glyph = _texTileManager->getGlyph(glyphKey);
            float targetFontSize = ch.fontSize;
            float scaleFactor = targetFontSize / (glyph->SDFScale*(float)_sdfParameter.sourceFontSize );
            
            float posLeft = x, posTop = y;
            posLeft += glyph->bitmapBearingX * scaleFactor;
            posTop += glyph->bitmapBearingY * scaleFactor;
            float posRight = posLeft + glyph->bitmapWidth * scaleFactor;
            float posBottom = posTop + glyph->bitmapHeight * scaleFactor;
            //
            x += glyph->bitmapAdvance*scaleFactor;
            //
            float uvLeft = glyph->texU;
            float uvRight = glyph->texU + glyph->texWidth;
            float uvTop = glyph->texV;
            float uvBottom = glyph->texV + glyph->texHeight;

            FontMeshVertex rect[4] = {
                { {posLeft, posTop}, {uvLeft, uvTop} },
                { {posRight, posTop}, {uvRight, uvTop} },
                { {posRight, posBottom}, {uvRight, uvBottom} },
                { {posLeft, posBottom}, {uvLeft, uvBottom} },
            };
            
            uint16_t baseIndex = _meshIBO.size()? _meshIBO.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0u, baseIndex + 1u, baseIndex + 2u, baseIndex + 0u, baseIndex + 2u, baseIndex + 3u, 
            };
            for( auto index : indices ) {
                _meshIBO.push_back(index);
            }
            _meshVBO.push_back(rect[0]);
            _meshVBO.push_back(rect[1]);
            _meshVBO.push_back(rect[2]);
            _meshVBO.push_back(rect[3]);
            _indices.push_back(reuseHandle);
        }
        rect.SetRight(x);

        return reuseHandle;
    }

    void SDFFontRenderer::resize( uint32_t width, uint32_t height ) {
        this->_width = width; this->_height = height;
    }

    SDFFontDrawData* SDFFontRenderer::endBuild() {
        // === 
        auto vbo = _device->createBuffer( BufferType::VertexBuffer, _meshVBO.size()*sizeof(FontMeshVertex) );
        auto ibo = _device->createBuffer( BufferType::IndexBuffer, _meshIBO.size() * sizeof(uint16_t) );
        // staging buffer 不需要在这里销毁，在更新操作之后再扔到 resource manager　里
        auto stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, vbo->size() + ibo->size());
        uint8_t* ptr= (uint8_t*)stagingBuffer->map(_device);
        memcpy(ptr, _meshVBO.data(), vbo->size());
        memcpy(ptr+vbo->size(), _meshIBO.data(), ibo->size());
        stagingBuffer->unmap(_device);
        //
        _updateItems.push_back( { vbo, ibo, stagingBuffer } );
        //

        SDFFontDrawData* drawData = new SDFFontDrawData();
        // 主要就是设置矩阵
        drawData->_argumentGroup = _pipeline->createArgumentGroup();
        if( 0xffffffff == _indicesHandle) {
            _indicesHandle = drawData->_argumentGroup->GetDescriptorHandle("Indices", _pipelineDescription);
        }
        if( 0xffffffff == _effectsHandle) {
            _effectsHandle = drawData->_argumentGroup->GetDescriptorHandle("Effects", _pipelineDescription);
        }
        if( 0xffffffff == _transformsHandle) {
            _transformsHandle = drawData->_argumentGroup->GetDescriptorHandle("Transforms", _pipelineDescription);
        }
        if( 0xffffffff == _contextHandle) {
            _contextHandle = drawData->_argumentGroup->GetDescriptorHandle("Context", _pipelineDescription);
        }

        if( 0xffffffff == _texArraySamplerHandle) {
            _texArraySamplerHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArraySampler", _pipelineDescription);
        }
        if( 0xffffffff == _texArrayHandle) {
            _texArrayHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArray", _pipelineDescription);
        }
        res_descriptor_t descriptor;
        descriptor.type = res_descriptor_type::Sampler;
        descriptor.sampler.mag = TextureFilter::Linear;
        descriptor.sampler.mip = TextureFilter::Linear;
        descriptor.sampler.min = TextureFilter::Linear;
        descriptor.handle = _texArraySamplerHandle;
        drawData->_argumentGroup->updateDescriptor(descriptor);
        descriptor.handle = _texArrayHandle;
        descriptor.type = res_descriptor_type::Image;
        descriptor.texture = _texTileManager->texture();
        drawData->_argumentGroup->updateDescriptor(descriptor);
        // Mesh数据
        drawData->_drawable = _device->createDrawable(_pipelineDescription);
        drawData->_drawable->setIndexBuffer( ibo, 0 );      // 
        drawData->_drawable->setVertexBuffer( vbo, 0, 0 );  // position
        drawData->_drawable->setVertexBuffer( vbo, 1, 8 );  // uv
        drawData->_vertexBuffer = vbo;
        drawData->_indexBuffer = ibo;
        drawData->_indexCount = (uint32_t)_meshIBO.size();
        drawData->_indices = std::move(_indices);
        drawData->_styles = std::move(_styles);
        drawData->_transforms = std::move(_transforms);
        //
        return drawData;
    }

    void SDFFontRenderer::tickResource( ResourceCommandEncoder* resEncoder ) {
        for( auto& item : _updateItems) {
            BufferSubResource vbSubRes = { 0, item.vertexBuffer->size() };
            BufferSubResource ibSubRes = { 0, item.indexBuffer->size() };
            BufferSubResource vbStagingSubRes = { 0, item.vertexBuffer->size() };
            BufferSubResource ibStagingSubRes = { item.vertexBuffer->size(), item.indexBuffer->size() };
            resEncoder->updateBuffer( item.vertexBuffer, item.stagingBuffer, &vbSubRes, &vbStagingSubRes );
            resEncoder->updateBuffer( item.indexBuffer, item.stagingBuffer, &ibSubRes, &ibStagingSubRes );
            // 回收 staging buffer
            _resourceManager->trackResource(item.stagingBuffer);
            //
        }
        _updateItems.clear();
        _texTileManager->tickResource(resEncoder, _resourceManager) ;
    }

    void SDFFontRenderer::prepareResource( ResourceCommandEncoder* resourceEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator ) {

        for( uint32_t i = 0; i<drawDataCount; ++i ) {
            auto drawData = drawDatas[i];
            res_descriptor_t descriptor;
            descriptor.handle = _indicesHandle;
            descriptor.bufferRange = sizeof(IndexHandle) * 1024;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_indices);
            drawData->_argumentGroup->updateDescriptor(descriptor);

            descriptor.handle = _effectsHandle;
            descriptor.bufferRange = sizeof(Style) * 256;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_styles);
            drawData->_argumentGroup->updateDescriptor(descriptor);
            drawData->_argumentGroup->prepairResource(resourceEncoder);

            descriptor.handle = _transformsHandle;
            descriptor.bufferRange = sizeof(Transform) * 256;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_transforms);
            drawData->_argumentGroup->updateDescriptor(descriptor);
            drawData->_argumentGroup->prepairResource(resourceEncoder);

            struct {
                float width, height;
            } contextSize = {(float)_width, (float)_height };
            descriptor.handle = _contextHandle;
            descriptor.bufferRange = sizeof(contextSize);
            uniformAllocator->allocateForDescriptor(descriptor, contextSize);
            drawData->_argumentGroup->updateDescriptor(descriptor);
            drawData->_argumentGroup->prepairResource(resourceEncoder);
        }

    }

    void SDFFontRenderer::draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount ) {
        // 1. bind pipeline
        renderEncoder->bindPipeline(_pipeline);
        for (size_t i = 0; i < drawDataCount; i++) {
            auto drawData = drawDatas[i];
            // 2. bind argument group
            renderEncoder->bindArgumentGroup(drawData->_argumentGroup);
            // 3. draw
            renderEncoder->drawIndexed( drawData->_drawable, 0, drawData->_indexCount, 0);
        }
    }

    void SDFFontDrawData::updateTransform( IndexHandle handle, Transform transform ) {
        if( handle.transformIndex < _transforms.size() ) {
            _transforms[handle.transformIndex] = transform;
        }
    }

    void SDFFontDrawData::updateStyle( IndexHandle handle, Style style ) {
        if( handle.styleIndex < _styles.size() ) {
            _styles[handle.styleIndex] = style;
        }
    }

}