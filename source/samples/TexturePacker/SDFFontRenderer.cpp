#include "SDFFontRenderer.h"
#include "SDFTextureTileManager.h"
#include "sdf_ubo.h"
#include "pipeline_bindings.h"
#include <ugi/device.h>
#include <ugi/buffer.h>
#include <ugi/flight_cycle_invoker.h>
#include <ugi/descriptor_binder.h>
#include <ugi/pipeline.h>
#include <ugi/texture.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_components/mesh.h>
#include <ugi/render_components/pipeline_material.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/command_encoder/resource_cmd_encoder.h>
#include <ugi/command_encoder/render_cmd_encoder.h>

#include <json.hpp>

namespace ugi {

    SDFFontRenderer::SDFFontRenderer()
        : _device(nullptr)
        , _texTileManager(nullptr)
        , _descIndices{}
        , _descEffects{}
        , _descTransforms{}
        , _descContext{}
        , _descSampler{}
        , _descTexArray{}
    {
    }
    
    bool SDFFontRenderer::initialize( Device* device, comm::IArchive* archive, GPUAsyncLoadManager* asyncLoader, const SDFRenderParameter& sdfParam ) {
        _device = device;
        _archive = archive;
        _asyncLoader = asyncLoader;
        _meshAllocator = new MeshBufferAllocator();
        _meshAllocator->initialize(device, 1024 * 256);
        _texTileManager = new SDFTextureTileManager();
        //
        _sdfParameter = sdfParam;
        nlohmann::json js;
        auto inputStream = archive->openIStream("./shaders/SDF2D/sdf.json", {comm::ReadFlag::binary});
        if(inputStream) {
            char* configBuffer = new char[inputStream->size()+1];
            configBuffer[inputStream->size()] = 0;
            inputStream->read(configBuffer, inputStream->size());
            js = nlohmann::json::parse(configBuffer);
            _sdfParameter.texArraySize = js["textureSize"].get<uint32_t>();
            _sdfParameter.texLayer = js["layer"];
            _sdfParameter.destinationSDFSize = js["sdfSize"];
            _sdfParameter.sourceFontSize = js["sourceSize"];
            _sdfParameter.extraSourceBorder = js["extraBorder"];
            _sdfParameter.searchDistance = js["searchDistance"];
            inputStream->close();
        }

        // Create Pipeline
        auto pipelineFile = archive->openIStream("/shaders/SDF2D/pipeline.bin", {comm::ReadFlag::binary});
        auto pipelineFileSize = pipelineFile->size();
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->read(pipelineBuffer, pipelineFileSize);
        _pipelineDescription = *(pipeline_desc_t*)pipelineBuffer;
        pipelineBuffer += sizeof(pipeline_desc_t);
        for (auto& shader : _pipelineDescription.shaders) {
            if (shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        // _pipelineDescription.pologonMode = polygon_mode_t::Fill;
        _pipelineDescription.topologyMode = topology_mode_t::TriangleList;
        _pipelineDescription.renderState.cullMode = cull_mode_t::None;
        _pipelineDescription.renderState.blendState.enable = true;
        _pipeline = _device->createGraphicsPipeline(_pipelineDescription);
        pipelineFile->close();
        if( !_pipeline) {
            return false;
        }
        bool rst = _texTileManager->initialize( device, archive, 
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
    
    IndexHandle SDFFontRenderer::appendText( float x, float y, SDFChar* text, uint32_t length, const Transform& transform, const Style& style, RectScope2f& rect ) {

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
            
            uint16_t baseIndex = _meshIBO.empty() ? 0u : (uint16_t)(_meshIBO.back() + 1);
            uint16_t indices[6] = {
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 1u), (uint16_t)(baseIndex + 2u),
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 2u), (uint16_t)(baseIndex + 3u),
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
        IndexHandle reuseHandle, const Style& style, RectScope2f& rect
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
            
            uint16_t baseIndex = _meshIBO.empty() ? 0u : (uint16_t)(_meshIBO.back() + 1);
            uint16_t indices[6] = {
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 1u), (uint16_t)(baseIndex + 2u),
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 2u), (uint16_t)(baseIndex + 3u),
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
        IndexHandle reuseHandle, const Transform& transform, RectScope2f& rect
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
            
            uint16_t baseIndex = _meshIBO.empty() ? 0u : (uint16_t)(_meshIBO.back() + 1);
            uint16_t indices[6] = {
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 1u), (uint16_t)(baseIndex + 2u),
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 2u), (uint16_t)(baseIndex + 3u),
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
        IndexHandle reuseHandle, RectScope2f& rect
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
            
            uint16_t baseIndex = _meshIBO.empty() ? 0u : (uint16_t)(_meshIBO.back() + 1);
            uint16_t indices[6] = {
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 1u), (uint16_t)(baseIndex + 2u),
                (uint16_t)(baseIndex + 0u), (uint16_t)(baseIndex + 2u), (uint16_t)(baseIndex + 3u),
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
        auto material = _pipeline->createMaterial(
            {BIND_INDICES,BIND_EFFECTS,BIND_TRANSFORMS,BIND_CONTEXT,BIND_TEXARRAYSAMPLER,BIND_TEXARRAY}, {});
        auto descs = material->descriptors();
        _descIndices    = descs[0];
        _descEffects    = descs[1];
        _descTransforms = descs[2];
        _descContext    = descs[3];
        _descSampler    = descs[4];
        _descTexArray   = descs[5];
        // Sampler
        _descSampler.type = res_descriptor_type::Sampler;
        _descSampler.res.samplerState.mag = TextureFilter::Linear;
        _descSampler.res.samplerState.mip = TextureFilter::Linear;
        _descSampler.res.samplerState.min = TextureFilter::Linear;
        material->updateDescriptor(_descSampler);
        _descTexArray.type = res_descriptor_type::Image;
        _descTexArray.res.imageView = _texTileManager->texture()->defaultView().handle;
        material->updateDescriptor(_descTexArray);
        // Mesh + Renderable (HelloWorld 模式)
        auto mesh = Mesh::CreateMesh(
            _device, _meshAllocator, _asyncLoader,
            (uint8_t const*)_meshVBO.data(), _meshVBO.size() * sizeof(FontMeshVertex),
            _meshIBO.data(), _meshIBO.size(),
            _pipelineDescription.vertexLayout,
            _pipelineDescription.topologyMode,
            polygon_mode_t::Fill,
            [](void*, CommandBuffer*){}
        );
        drawData->_renderable = new Renderable(mesh, material, _pipeline, raster_state_t());
        drawData->_indexCount = (uint32_t)_meshIBO.size();
        drawData->_indices = std::move(_indices);
        drawData->_styles = std::move(_styles);
        drawData->_transforms = std::move(_transforms);
        //
        return drawData;
    }

    void SDFFontRenderer::tickResource( ResourceCommandEncoder* resEncoder ) {
        for( auto& item : _updateItems) {
            buffer_subres_t vbSubRes = { 0, item.vertexBuffer->size() };
            buffer_subres_t ibSubRes = { 0, item.indexBuffer->size() };
            buffer_subres_t vbStagingSubRes = { 0, item.vertexBuffer->size() };
            buffer_subres_t ibStagingSubRes = { item.vertexBuffer->size(), item.indexBuffer->size() };
            resEncoder->updateBuffer( item.vertexBuffer, item.stagingBuffer, &vbSubRes, &vbStagingSubRes );
            resEncoder->updateBuffer( item.indexBuffer, item.stagingBuffer, &ibSubRes, &ibStagingSubRes );
            _device->cycleInvoker().postCallable([device = _device, buf = item.stagingBuffer]() {
                device->destroyBuffer(buf);
            });
            //
        }
        _updateItems.clear();
        _texTileManager->tickResource(resEncoder, _device);
    }

    void SDFFontRenderer::prepareResource( ResourceCommandEncoder* resourceEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount, UniformAllocator* uniformAllocator ) {
        for (uint32_t i = 0; i < drawDataCount; ++i) {
            auto* drawData = drawDatas[i];
            auto* mtl = drawData->_renderable->material();

            if (!drawData->_indices.empty()) {
                auto ubo = uniformAllocator->allocate(sizeof(IndexHandle) * drawData->_indices.size());
                memcpy(ubo.ptr, drawData->_indices.data(), ubo.size);
                _descIndices.res.buffer.buffer = ubo.buffer;
                _descIndices.res.buffer.offset = ubo.offset;
                _descIndices.res.buffer.size   = (uint32_t)ubo.size;
                mtl->updateDescriptor(_descIndices);
            }
            if (!drawData->_styles.empty()) {
                auto ubo = uniformAllocator->allocate(sizeof(Style) * drawData->_styles.size());
                memcpy(ubo.ptr, drawData->_styles.data(), ubo.size);
                _descEffects.res.buffer.buffer = ubo.buffer;
                _descEffects.res.buffer.offset = ubo.offset;
                _descEffects.res.buffer.size   = (uint32_t)ubo.size;
                mtl->updateDescriptor(_descEffects);
            }
            if (!drawData->_transforms.empty()) {
                auto ubo = uniformAllocator->allocate(sizeof(Transform) * drawData->_transforms.size());
                memcpy(ubo.ptr, drawData->_transforms.data(), ubo.size);
                _descTransforms.res.buffer.buffer = ubo.buffer;
                _descTransforms.res.buffer.offset = ubo.offset;
                _descTransforms.res.buffer.size   = (uint32_t)ubo.size;
                mtl->updateDescriptor(_descTransforms);
            }

            float contextSize[2] = { (float)_width, (float)_height };
            uniformAllocator->allocateForDescriptor(_descContext, contextSize);
            mtl->updateDescriptor(_descContext);
        }
    }

    void SDFFontRenderer::draw( RenderCommandEncoder* renderEncoder, SDFFontDrawData** drawDatas, uint32_t drawDataCount ) {
        // 1. bind pipeline
        renderEncoder->bindPipeline(_pipeline);
        for (size_t i = 0; i < drawDataCount; i++) {
            auto drawData = drawDatas[i];
            // 2. bind + draw
            _pipeline->applyMaterial(drawData->_renderable->material());
            _pipeline->flushMaterials(renderEncoder->commandBuffer());
            renderEncoder->draw(drawData->_renderable->mesh(), drawData->_indexCount);
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