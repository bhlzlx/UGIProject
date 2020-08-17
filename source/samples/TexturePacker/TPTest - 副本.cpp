#include "TPTest.h"
#include <cassert>
#include <ugi/Device.h>
#include <ugi/Swapchain.h>
#include <ugi/CommandQueue.h>
#include <ugi/CommandBuffer.h>
#include <ugi/Buffer.h>
#include <ugi/RenderPass.h>
#include <ugi/Semaphore.h>
#include <ugi/Pipeline.h>
#include <ugi/Argument.h>
#include <ugi/Texture.h>
#include <ugi/Drawable.h>
#include <ugi/UniformBuffer.h>
#include <ugi/ResourceManager.h>
#include <hgl/assets/AssetsSource.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <cmath>


namespace ugi {

    struct SDFFlag {
        uint8_t inner : 1;
        uint8_t opaque : 1;
    };

    void SignedDistanceFieldImage2D(
        uint8_t* src, int32_t srcWid, int32_t srcHei,
        uint8_t* dst, int32_t dstWid, int32_t dstHei
    ) {
        SDFFlag* flagMap = new SDFFlag[srcWid*srcHei];
        constexpr int searchDistance = 12;
        float maxDistance = 16.0f*sqrt(2.0f);
        // 像素类型划分
        // 如果像素是半透
        for( int32_t i = 0; i<srcHei; ++i) {
            for( int32_t j = 0; j<srcWid; ++j) {
                int32_t index = i*srcWid+j;
                flagMap[index].opaque = 0;  // 默认透明
                flagMap[index].inner = 0;   // 默认为外部
                if( src[index] != 0x0 ) {   // 值不为0证明为内部像素
                    flagMap[index].inner = 1;
                    if(src[index] == 0xff) {
                        flagMap[index].opaque = 1;
                    }
                }
            }
        }
        float ratioX = (float)srcWid / (float)dstWid;
        float ratioY = (float)srcHei / (float)dstHei;
        //
        for( int32_t i = 0; i<dstHei; ++i) {
            for( int32_t j = 0; j<dstWid; ++j) {

                int srcX = j * ratioX, srcY = i * ratioY;
                int srcMinX = srcX - searchDistance >= 0 ? srcX - searchDistance : 0;
                int srcMaxX = srcX + searchDistance >= srcWid ? srcWid-1 : srcX + searchDistance;
                int srcMinY = srcY - searchDistance >= 0 ? srcY - searchDistance : 0;
                int srcMaxY = srcY + searchDistance >= srcHei ? srcHei-1 : srcY + searchDistance;

                int srcIndex = srcX + srcY * srcWid;
                int dstIndex = j + i * dstWid;
                //
                const SDFFlag& flag = flagMap[srcX+srcWid*srcY];
                //
                float distance = maxDistance;
                if(flag.inner) {
                    if( flag.opaque ) { // 不透明的直接算距离
                        for( int xpos = srcMinX; xpos<=srcMaxX; ++xpos ) {
                            for( int ypos = srcMinY; ypos<=srcMaxY; ++ypos) {
                                SDFFlag tempFlag = flagMap[xpos+ypos*srcWid];
                                if(tempFlag.opaque == 0) { // 半透明
                                    float tempDistance = sqrt(pow(xpos-srcX,2) + pow(ypos-srcY,2));
                                    // 两个版本的透明像素距离处理
                                    //tempDistance += (src[xpos+ypos*srcWid] / 255.0f);
                                    tempDistance += ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                                    if(tempDistance<distance) {
                                        distance = tempDistance;
                                    }
                                }
                            }
                        }
                    } else { // 半透明的，这个特别处理一下
                        distance = ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                    }
                    dst[dstIndex] = ((distance / maxDistance)/2.f + 0.5) * 255;
                } else {
                    for( int xpos = srcMinX; xpos<=srcMaxX; ++xpos ) {
                        for( int ypos = srcMinY; ypos<=srcMaxY; ++ypos) {
                            SDFFlag tempFlag = flagMap[xpos+ypos*srcWid];
                            if(tempFlag.inner == 1) { // 半透明
                                float tempDistance = sqrt(pow(xpos-srcX,2) + pow(ypos-srcY,2));
                                // 两个版本的透明像素距离处理
                                // tempDistance += (1.0f - (src[xpos+ypos*srcWid] / 255.0f));
                                tempDistance -= ((float)src[srcX+srcWid*srcY]-127) / 255.0f;
                                if(tempDistance<distance) {
                                    distance = tempDistance;
                                }
                            }
                        }
                    }
                    dst[dstIndex] = (0.5f - (distance / maxDistance)/2.f)* 255;
                }
            }
        }
        delete []flagMap;
    }

    struct Font {
        stbtt_fontinfo stbTtfInfo;
        int x0, y0;
        int x1, y1;
        int maxHeight;
        int ascent; int descent; int lineGap;
    };

    struct Glyph {
        int width;
        int height;
        int bearingX;
        int bearingY;
        int advance;
        //
        float uvX;
        float uvY;
        float uvWidth;
        float uvHeight;
    };

    uint8_t GlyphBitmapPool[128*128];
    uint8_t GlyphSDFPool[128*128];

    std::vector<Glyph> glyphs;

    hgl::assets::AssetsSource* appAssetsSource = nullptr;

    struct Vertex {
        float x; float y;
        float u; float v;
    };

    Vertex RectangleVertices[] = {
        { -1, -1, 0, 0 }, { 1, -1, 1.0f, 0 },
        { -1,  1, 0, 1.0f }, { 1,  1, 1.0f, 1.0f },
    };

    uint16_t RectangleIndices[] = {
        0, 1, 2, 1, 3, 2
    };

    static const float DPI = 96;

    Glyph GetGlyphData( uint16_t charCode ) {
        static Font* pInfo = nullptr;
        if(!pInfo) {
            pInfo = new Font();
            auto inputStream = appAssetsSource->Open(hgl::UTF8String("hwzhsong.ttf"));
            auto size = inputStream->GetSize();
            uint8_t* data = new uint8_t[size];
            inputStream->ReadFully(data, size);
            int error = stbtt_InitFont(&pInfo->stbTtfInfo, data, 0);
            stbtt_GetFontBoundingBox(&pInfo->stbTtfInfo, &pInfo->x0, &pInfo->y0, &pInfo->x1, &pInfo->y1);
            pInfo->maxHeight = pInfo->y1 - pInfo->y0;
            stbtt_GetFontVMetrics(&pInfo->stbTtfInfo, &pInfo->ascent, &pInfo->descent, &pInfo->lineGap);
        }
        // ==== 
        float scale = 1.0f;
        uint32_t charSize = 64;
        float fontSize = charSize* DPI / 72.0f;
        scale = stbtt_ScaleForMappingEmToPixels(&pInfo->stbTtfInfo, fontSize);
        //int baseline = (int)(pInfo->ascent * scale);
        int advance, lsb, x0, y0, x1, y1;
        float shiftX = 0.0f; float shiftY = 0.0f;
        stbtt_GetCodepointHMetrics(&pInfo->stbTtfInfo, charCode, &advance, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(&pInfo->stbTtfInfo, charCode, scale, scale, shiftX, shiftY, &x0, &y0, &x1, &y1);
        stbtt_MakeCodepointBitmapSubpixel(
            &pInfo->stbTtfInfo,
            &GlyphBitmapPool[0],
            x1 - x0,
            y1 - y0,
            x1 - x0,
            scale, scale,
            shiftX, shiftY,
            charCode
        );
        //
        Glyph c;
        c.bearingX = x0;
        c.bearingY = -y0;
        c.width = x1 - x0;
        c.height = y1 - y0;
        c.advance = advance * scale;
        //
        float r = 32.0f / ((c.width>c.height) ? c.width : c.height);
        SignedDistanceFieldImage2D( GlyphBitmapPool, c.width,c.height, GlyphSDFPool, c.width*r, c.height*r );
        //
        c.width *= r;
        c.height *= r;
        //
        return c;
    }

    bool TPTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {
        printf("initialize\n");
        appAssetsSource = assetsSource;
        _renderSystem = new ugi::RenderSystem();
        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        _device = _renderSystem->createDevice(descriptor, assetsSource);
        _resourceManager = new ResourceManager(_device);
        _uniformAllocator = _device->createUniformAllocator();
        _swapchain = _device->createSwapchain( _wnd );
        // command queues
        _graphicsQueue = _device->graphicsQueues()[0];
        _uploadQueue = _device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            _frameCompleteFences[i] = _device->createFence();
            _renderCompleteSemaphores[i] = _device->createSemaphore();
            _commandBuffers[i] = _graphicsQueue->createCommandBuffer( _device );
        }

        // Create Pipeline
        hgl::io::InputStream* pipelineFile = assetsSource->Open( hgl::UTF8String("/shaders/sdf2d/pipeline.bin"));
        auto pipelineFileSize = pipelineFile->GetSize();
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->ReadFully(pipelineBuffer,pipelineFileSize);
        PipelineDescription& pipelineDesc = *(PipelineDescription*)pipelineBuffer;
        pipelineBuffer += sizeof(PipelineDescription);
        for( auto& shader : pipelineDesc.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        pipelineDesc.pologonMode = PolygonMode::Fill;
        pipelineDesc.topologyMode = TopologyMode::TriangleList;
        pipelineDesc.renderState.cullMode = CullMode::None;
        pipelineDesc.renderState.blendState.enable = false;
        _pipeline = _device->createPipeline(pipelineDesc);
        _argumentGroup = _pipeline->createArgumentGroup();
        // _argumentGroup2 = _pipeline->createArgumentGroup();
        // == 创建纹理，VB，IB
        TextureDescription texDesc = {
            TextureType::Texture2D, UGIFormat::R8_UNORM, 1, 1, 512, 512, 1 
        };
        _texture = _device->createTexture(texDesc, ResourceAccessType::ShaderRead );
        _vertexBuffer = _device->createBuffer( BufferType::VertexBuffer, sizeof(RectangleVertices) );
        _indexBuffer = _device->createBuffer( BufferType::IndexBuffer, sizeof(RectangleIndices));
        // = 更新 VB, iB
        Buffer* stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, _vertexBuffer->size() + _indexBuffer->size() );
        uint8_t* ptr = (uint8_t*)stagingBuffer->map( _device );
        memcpy(ptr, RectangleVertices, sizeof(RectangleVertices));  // 复制 vertex data
        memcpy(ptr + _vertexBuffer->size(), RectangleIndices, sizeof(RectangleIndices)); // 复制 index data
        stagingBuffer->unmap(_device);
        // 
        auto updateCmd = _uploadQueue->createCommandBuffer( _device );
        updateCmd->beginEncode(); 
        auto resourceEncoder = updateCmd->resourceCommandEncoder();
        {
            BufferSubResource vbSubRes;
            BufferSubResource vbStageSubRes;
            vbSubRes.offset = 0; vbSubRes.size = _vertexBuffer->size();
            vbStageSubRes.offset = 0; vbStageSubRes.size = _vertexBuffer->size();
            resourceEncoder->updateBuffer( _vertexBuffer, stagingBuffer, &vbSubRes, &vbStageSubRes );
            //
            BufferSubResource ibSubRes;
            BufferSubResource ibStageSubRes;
            ibSubRes.offset = 0; ibSubRes.size = _indexBuffer->size();
            ibStageSubRes.offset = _vertexBuffer->size(); ibStageSubRes.size = _indexBuffer->size();
            resourceEncoder->updateBuffer( _indexBuffer, stagingBuffer, &ibSubRes, &ibStageSubRes );
        }
        resourceEncoder->endEncode();

        updateCmd->endEncode();

        _drawable = _device->createDrawable(pipelineDesc);
        _drawable->setVertexBuffer( _vertexBuffer, 0, 0 );
        _drawable->setVertexBuffer( _vertexBuffer, 1, 8 );
        _drawable->setIndexBuffer( _indexBuffer, 0 );

        //
		Semaphore* imageAvailSemaphore = _swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&updateCmd,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, _frameCompleteFences[_flightIndex]);
        _uploadQueue->submitCommandBuffers(submitBatch);

        _uploadQueue->waitIdle();
        //
        ResourceDescriptor res;
        _transformUniformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
        _transformUniformDescriptor.descriptorHandle = ArgumentGroup::GetDescriptorHandle("Transform", pipelineDesc );
        _transformUniformDescriptor.bufferRange = 32;

        _SDFUniformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
        _SDFUniformDescriptor.descriptorHandle = ArgumentGroup::GetDescriptorHandle("SDFArgument", pipelineDesc );
        _SDFUniformDescriptor.bufferRange = 12;

        _samplerState.min = ugi::TextureFilter::Linear;
        _samplerState.mag = ugi::TextureFilter::Linear;
        _samplerState.mip = ugi::TextureFilter::Linear;

        res.type = ArgumentDescriptorType::Sampler;
        res.sampler = _samplerState;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triSampler", pipelineDesc );
        _argumentGroup->updateDescriptor(res);
        
        res.type = ArgumentDescriptorType::Image;
        res.texture = _texture;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triTexture", pipelineDesc );
        //
        _argumentGroup->updateDescriptor(res);
        //
        auto glyph = GetGlyphData(u'囊');
        glyphs.push_back(glyph);
        //
        _flightIndex = 0;
        return true;
    }

    void TPTest::tick() {
        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];


        cmdbuf->beginEncode(); {
            if(glyphs.size()) {
                auto resourceEncoder = cmdbuf->resourceCommandEncoder();
                for( auto& glyph : glyphs ) {
                    Buffer* stagingBuffer = _device->createBuffer( BufferType::StagingBuffer, glyph.width* glyph.height );
                    void* ptr = stagingBuffer->map(_device);
                    memcpy( ptr, GlyphSDFPool, stagingBuffer->size());
                    //memcpy( ptr, GlyphBitmapPool, stagingBuffer->size());
                    ImageRegion region;
                    region.arrayIndex = 0;
                    region.mipLevel = 0;
                    region.offset = {};
                    region.extent.width = glyph.width;
                    region.extent.height = glyph.height;
                    region.extent.depth = 1;
                    uint32_t offset = 0;
                    resourceEncoder->updateImage( _texture, stagingBuffer, &region, &offset, 1);
                }
                glyphs.clear();
                resourceEncoder->endEncode();
            }

            float TransformData [2][4] = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f, 0.0f },
            };
            struct SDFArgument {
                float       sdfMin;
                float       sdfMax;
                uint32_t    colorMask;
            };
            SDFArgument SDFArgumentData = {
                0.495f, 0.45f, 0xffffffff
            };
            _uniformAllocator->allocateForDescriptor(_transformUniformDescriptor, TransformData);
            _uniformAllocator->allocateForDescriptor(_SDFUniformDescriptor, SDFArgumentData);
            //
            _argumentGroup->updateDescriptor(_transformUniformDescriptor);
            _argumentGroup->updateDescriptor(_SDFUniformDescriptor);
            //
            auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            resourceEncoder->prepareArgumentGroup(_argumentGroup);
            resourceEncoder->endEncode();
            //
            RenderPassClearValues clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                renderCommandEncoder->setLineWidth(1.0f);
                renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, _width, _height );
                renderCommandEncoder->bindPipeline(_pipeline);
                renderCommandEncoder->bindArgumentGroup(_argumentGroup);
                renderCommandEncoder->drawIndexed( _drawable, 0, 6 );
            }
            renderCommandEncoder->endEncode();
        }
        cmdbuf->endEncode();

		Semaphore* imageAvailSemaphore = _swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&cmdbuf,
			1,
			&imageAvailSemaphore,// submitInfo.semaphoresToWait
			1,
			&_renderCompleteSemaphores[_flightIndex], // submitInfo.semaphoresToSignal
			1
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, _frameCompleteFences[_flightIndex]);

        bool submitRst = _graphicsQueue->submitCommandBuffers(submitBatch);
        assert(submitRst);
        _swapchain->present( _device, _graphicsQueue, _renderCompleteSemaphores[_flightIndex] );

    }
        
    void TPTest::resize(uint32_t width, uint32_t height) {
        _swapchain->resize( _device, width, height );
        //
        _width = width;
        _height = height;
    }

    void TPTest::release() {
    }

    const char * TPTest::title() {
        return "HelloWorld";
    }
        
    uint32_t TPTest::rendererType() {
        return 0;
    }

}

ugi::TPTest theapp;

UGIApplication* GetApplication() {
    return &theapp;
}