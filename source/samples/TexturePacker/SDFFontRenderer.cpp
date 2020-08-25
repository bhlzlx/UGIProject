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

    void SDFFontRenderer::beginBuild() {
        _verticesCache.clear();
        _indicesCache.clear();
        // === 
        _indices.clear();
        _effects.clear();
        _transforms.clear();
        _effectRecord.clear();
        _transformRecord.clear();
    }
    
    void SDFFontRenderer::appendText( float x, float y, const std::vector<SDFChar>& text, const hgl::Vector3f (& transform )[2] ) {
        uint32_t transformIndex = 0;

        SDFFontDrawData::Transform trans = {
            transform[0], transform[1]
        };
        auto iter = _transformRecord.find(trans);
        if( iter == _transformRecord.end()) {
            // 没有记录，添加新的
            transformIndex = (uint32_t)_transforms.size();
            _transforms.push_back(trans);
            _transformRecord[trans] = transformIndex;
        } else {
            transformIndex = iter->second;
        }

        for( auto& ch : text ) {
            uint32_t indexID = (uint32_t)_indices.size();
            uint32_t effectIndex = 0;
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
            
            uint16_t baseIndex = _indicesCache.size()? _indicesCache.back()+1:0;
            uint16_t indices[6] = {
                baseIndex + 0u, baseIndex + 1u, baseIndex + 2u, baseIndex + 0u, baseIndex + 2u, baseIndex + 3u, 
            };
            for( auto index : indices ) {
                _indicesCache.push_back(index);
            }
            _verticesCache.push_back(rect[0]);
            _verticesCache.push_back(rect[1]);
            _verticesCache.push_back(rect[2]);
            _verticesCache.push_back(rect[3]);
            //
            SDFFontDrawData::Effect effect;
            effect.type = ch.type;
            effect.colorMask = ch.color;
            effect.effectColor = ch.effectColor;
            effect.gray = 1.0f;
            effect.arrayIndex = glyph->texIndex; 
            {
                auto iter = _effectRecord.find(effect);
                if( iter == _effectRecord.end()) {
                    // 没有记录，添加新的
                    effectIndex = (uint32_t)_effects.size();
                    _effects.push_back(effect);
                } else {
                    effectIndex = iter->second;
                }
            }
            SDFFontDrawData::Index index = { (uint32_t)effectIndex | ((uint32_t)transformIndex<<16) };
            _indices.push_back(index);
        }        
    }

    SDFFontDrawData* SDFFontRenderer::endBuild() {
        // === 
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
        if( 0xffffffff == _indicesHandle) {
            _indicesHandle = drawData->_argumentGroup->GetDescriptorHandle("Indices", _pipelineDescription);
        }
        if( 0xffffffff == _effectsHandle) {
            _effectsHandle = drawData->_argumentGroup->GetDescriptorHandle("Effects", _pipelineDescription);
        }
        if( 0xffffffff == _transformsHandle) {
            _transformsHandle = drawData->_argumentGroup->GetDescriptorHandle("Transforms", _pipelineDescription);
        }
        if( 0xffffffff == _texArraySamplerHandle) {
            _texArraySamplerHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArraySampler", _pipelineDescription);
        }
        if( 0xffffffff == _texArrayHandle) {
            _texArrayHandle = drawData->_argumentGroup->GetDescriptorHandle("TexArray", _pipelineDescription);
        }
        ResourceDescriptor descriptor;
        descriptor.type = ArgumentDescriptorType::Sampler;
        descriptor.sampler.mag = TextureFilter::Linear;
        descriptor.sampler.mip = TextureFilter::Linear;
        descriptor.sampler.min = TextureFilter::Linear;
        descriptor.descriptorHandle = _texArraySamplerHandle;
        drawData->_argumentGroup->updateDescriptor(descriptor);
        descriptor.descriptorHandle = _texArrayHandle;
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
        drawData->_indices = std::move(_indices);
        drawData->_effects = std::move(_effects);
        drawData->_effectRecord = std::move(_effectRecord);
        drawData->_transformRecord = std::move(_transformRecord);
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
            ResourceDescriptor descriptor;
            descriptor.descriptorHandle = _indicesHandle;
            descriptor.bufferRange = sizeof(SDFFontDrawData::Index) * 1024;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_indices);
            drawData->_argumentGroup->updateDescriptor(descriptor);

            descriptor.descriptorHandle = _effectsHandle;
            descriptor.bufferRange = sizeof(SDFFontDrawData::Effect) * 256;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_effects);
            drawData->_argumentGroup->updateDescriptor(descriptor);
            drawData->_argumentGroup->prepairResource(resourceEncoder);

            descriptor.descriptorHandle = _transformsHandle;
            descriptor.bufferRange = sizeof(SDFFontDrawData::Transform) * 256;
            uniformAllocator->allocateForDescriptor(descriptor, drawData->_transform);
            drawData->_argumentGroup->updateDescriptor(descriptor);
            drawData->_argumentGroup->prepairResource(resourceEncoder);

            struct {
                float width, height;
            } contextSize = {(float)_width, (float)_height };
            descriptor.descriptorHandle = _contextHandle;
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

}