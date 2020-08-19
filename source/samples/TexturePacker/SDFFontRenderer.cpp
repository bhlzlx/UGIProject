#include "SDFFontRenderer.h"
#include "SDFTextureTileManager.h"
#include <hgl/assets/AssetsSource.h>
#include <ugi/Device.h>
#include <ugi/Buffer.h>
#include <ugi/ResourceManager.h>
#include <ugi/Argument.h>
#include <ugi/Pipeline.h>
#include <ugi/Drawable.h>
#include <ugi/UniformBuffer.h>
#include <ugi/commandBuffer/ResourceCommandEncoder.h>
#include <ugi/commandBuffer/RenderCommandEncoder.h>
#include <hgl/io/InputStream.h>

#include <json.hpp>

namespace ugi {

    SDFFontRenderer::SDFFontRenderer()
        : _device(nullptr)
        , _texTileManager(nullptr)
        , _transformHandle(0xffffffff)
        , _sampler2DArrayHandle(0xffffffff)
        , _texture2DArrayHandle(0xffffffff)
        , _sdfArgumentHandle(0xffffffff)
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
        _pipelineDescription = *(PipelineDescription*)pipelineBuffer;
        pipelineBuffer += sizeof(PipelineDescription);
        for( auto& shader : _pipelineDescription.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        _pipelineDescription.pologonMode = PolygonMode::Fill;
        _pipelineDescription.topologyMode = TopologyMode::TriangleList;
        _pipelineDescription.renderState.cullMode = CullMode::None;
        _pipelineDescription.renderState.blendState.enable = true;
        _pipeline = _device->createPipeline(_pipelineDescription);
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

    SDFFontDrawData* SDFFontRenderer::buildDrawData( float x, float y, const std::vector<SDFChar>& text ) {

        _verticesCache.clear();
        _indicesCache.clear();

        for( auto& ch : text ) {
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
            
            uint16_t baseIndex = _indicesCache.size()? _indicesCache.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 0, baseIndex + 2, baseIndex + 3, 
            };
            for( auto index : indices ) {
                _indicesCache.push_back(index);
            }
            _verticesCache.push_back(rect[0]);
            _verticesCache.push_back(rect[1]);
            _verticesCache.push_back(rect[2]);
            _verticesCache.push_back(rect[3]);
        }
        auto vbo = _device->createBuffer( BufferType::VertexBuffer, _verticesCache.size()*sizeof(FontMeshVertex) );
        auto ibo = _device->createBuffer( BufferType::IndexBuffer, _indicesCache.size() * sizeof(uint16_t) );
        // staging buffer 不需要在这里销毁，在更新操作之后再扔到 resource manager　里
        auto stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, vbo->size() + ibo->size());
        uint8_t* ptr= (uint8_t*)stagingBuffer->map(_device);
        memcpy(ptr, _verticesCache.data(), vbo->size());
        memcpy(ptr+vbo->size(), _indicesCache.data(), ibo->size());
        stagingBuffer->unmap(_device);
        //
        _updateItems.push_back( { vbo, ibo, stagingBuffer } );
        //
        SDFFontDrawData* drawData = new SDFFontDrawData();
        // 主要就是设置矩阵
        drawData->_argumentGroup = _pipeline->createArgumentGroup();
        if( 0xffffffff == _transformHandle) {
            _transformHandle = drawData->_argumentGroup->GetDescriptorHandle("Transform", _pipelineDescription);
        }
        if( 0xffffffff == _sampler2DArrayHandle) {
            _sampler2DArrayHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArraySampler", _pipelineDescription);
        }
        if( 0xffffffff == _texture2DArrayHandle) {
            _texture2DArrayHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArray", _pipelineDescription);
        }
        if( 0xffffffff == _sdfArgumentHandle) {
            _sdfArgumentHandle = drawData->_argumentGroup->GetDescriptorHandle("SDFArgument", _pipelineDescription);
        }
        ResourceDescriptor descriptor;
        descriptor.type = ArgumentDescriptorType::Sampler;
        descriptor.sampler.mag = TextureFilter::Linear;
        descriptor.sampler.mip = TextureFilter::Linear;
        descriptor.sampler.min = TextureFilter::Linear;
        descriptor.descriptorHandle = _sampler2DArrayHandle;
        drawData->_argumentGroup->updateDescriptor(descriptor);
        descriptor.descriptorHandle = _texture2DArrayHandle;
        descriptor.type = ArgumentDescriptorType::Image;
        descriptor.texture = _texTileManager->texture();
        drawData->_argumentGroup->updateDescriptor(descriptor);
        // Mesh数据
        drawData->_drawable = _device->createDrawable(_pipelineDescription);
        drawData->_drawable->setIndexBuffer( ibo, 0 );      // 
        drawData->_drawable->setVertexBuffer( vbo, 0, 0 );  // position
        drawData->_drawable->setVertexBuffer( vbo, 1, 8 );  // uv
        drawData->_vertexBuffer = vbo;
        drawData->_indexBuffer = ibo;
        drawData->_indexCount = (uint32_t)_indicesCache.size();

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
            ResourceDescriptor descriptor;
            descriptor.descriptorHandle = _transformHandle;
            descriptor.bufferRange = 32;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_transform);
            drawData->_argumentGroup->updateDescriptor(descriptor);

            descriptor.descriptorHandle = _sdfArgumentHandle;
            descriptor.bufferRange = 16;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_sdfArgument);
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

}