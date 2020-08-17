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

    struct Vertex {
        float x; float y;
        float u; float v;
    };

    Vertex RectangleVertices[] = {
        { -1, -1, 0, 0 }, { 1, -1, 0.25f, 0 },
        { -1,  1, 0, 0.25f }, { 1,  1, 0.25f, 0.25f },
    };

    uint16_t RectangleIndices[] = {
        0, 1, 2, 1, 3, 2
    };

    bool TPTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {
        printf("initialize\n");
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

        _sdfTexTileManager = new SDFTextureTileManager();
        if(!_sdfTexTileManager->initialize( _device, assetsSource, 32, 1024, 4 )) {
            return false;
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
        _SDFUniformDescriptor.bufferRange = 16;

        _samplerState.min = ugi::TextureFilter::Linear;
        _samplerState.mag = ugi::TextureFilter::Linear;
        _samplerState.mip = ugi::TextureFilter::Linear;

        res.type = ArgumentDescriptorType::Sampler;
        res.sampler = _samplerState;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triSampler", pipelineDesc );
        _argumentGroup->updateDescriptor(res);
        
        _texture = _sdfTexTileManager->texture();
        res.type = ArgumentDescriptorType::Image;
        res.texture = _texture;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triTexture", pipelineDesc );
        //
        _argumentGroup->updateDescriptor(res);
        //
        _flightIndex = 0;

        const char16_t chars[] = u"中国智造，慧及全球。";

        for( auto& ch : chars ) {
            if(ch == 0) {
                break;
            }
            GlyphKey key;
            key.fontID = 0;
            key.charCode = ch;
            _sdfTexTileManager->registGlyph(key);
        }
        return true;
    }

    void TPTest::tick() {
        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];


        cmdbuf->beginEncode(); {

            float TransformData [2][4] = {
                { 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f, 0.0f },
            };
            struct SDFArgument {
                float       sdfMin;
                float       sdfMax;
                float       layerIndex;
                uint32_t    colorMask;
            };
            SDFArgument SDFArgumentData = {
                0.46f, 0.50f, 0.0f, 0xffffffff
            };
            _uniformAllocator->allocateForDescriptor(_transformUniformDescriptor, TransformData);
            _uniformAllocator->allocateForDescriptor(_SDFUniformDescriptor, SDFArgumentData);
            //
            _argumentGroup->updateDescriptor(_transformUniformDescriptor);
            _argumentGroup->updateDescriptor(_SDFUniformDescriptor);
            //
            auto resourceEncoder = cmdbuf->resourceCommandEncoder();

            _sdfTexTileManager->resourceTick(resourceEncoder);

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