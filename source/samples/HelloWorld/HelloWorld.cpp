#include "HelloWorld.h"
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
#include <hgl/assets/AssetsSource.h>

#include <cmath>

namespace ugi {

        bool HelloWorld::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {

        hgl::io::InputStream* pipelineFile = assetsSource->Open( hgl::UTF8String("/shaders/triangle/pipeline.bin"));
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
        //
        pipelineDesc.topologyMode = TopologyMode::TriangleList;

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
        pipelineDesc.renderState.cullMode = CullMode::None;
        pipelineDesc.renderState.blendState.enable = false;
        _pipeline = _device->createGraphicsPipeline(pipelineDesc);
        //
        _argumentGroup = _pipeline->createArgumentGroup();

        _vertexBuffer = _device->createBuffer( BufferType::VertexBuffer, sizeof(float) * 5 * 3 );
        Buffer* vertexStagingBuffer = _device->createBuffer( BufferType::StagingBuffer, sizeof(float) * 5 * 3 );
        float vertexData[] = {
            -0.5, -0.5, 0, 0, 0,
            0, 0.5, 0, 0.5, 1.0,
            0.5, -0.5, 0, 1.0, 0
        };
        void* ptr = vertexStagingBuffer->map( _device );
        memcpy(ptr, vertexData, sizeof(vertexData));
        vertexStagingBuffer->unmap(_device);

        uint16_t indexData[] = {
            0, 1, 2
        };
        _indexBuffer = _device->createBuffer( BufferType::IndexBuffer, sizeof(indexData));
        Buffer* indexStagingBuffer = _device->createBuffer( BufferType::StagingBuffer, sizeof(indexData) );
        ptr = indexStagingBuffer->map( _device );
        memcpy(ptr, indexData, sizeof(indexData));
        indexStagingBuffer->unmap(_device);

        auto updateCmd = _uploadQueue->createCommandBuffer( _device );

        updateCmd->beginEncode();

        auto resourceEncoder = updateCmd->resourceCommandEncoder();

        BufferSubResource subRes;
        subRes.offset = 0; subRes.size = _vertexBuffer->size();
        resourceEncoder->updateBuffer( _vertexBuffer, vertexStagingBuffer, &subRes, &subRes );
        subRes.size = sizeof(indexData);
        resourceEncoder->updateBuffer( _indexBuffer, indexStagingBuffer, &subRes, &subRes );

        _drawable = _device->createDrawable(pipelineDesc);
        _drawable->setVertexBuffer( _vertexBuffer, 0, 0 );
        _drawable->setVertexBuffer( _vertexBuffer, 1, 12 );
        _drawable->setIndexBuffer( _indexBuffer, 0 );
        //
        TextureDescription texDesc;
        texDesc.format = UGIFormat::RGBA8888_UNORM;
        texDesc.depth = 1;
        texDesc.width = 16;
        texDesc.height = 16;
        texDesc.type = TextureType::Texture2D;
        texDesc.mipmapLevel = 1;
        texDesc.arrayLayers = 1;
        _texture = _device->createTexture(texDesc, ResourceAccessType::ShaderRead );

        uint32_t texData[] = {
            0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
            0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
            0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
            0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
            0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
            0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
            0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
            0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
        };
        auto texStagingBuffer =  _device->createBuffer( BufferType::StagingBuffer, sizeof(texData) );
        ptr = texStagingBuffer->map(_device);
        memcpy(ptr, texData, sizeof(texData));
        texStagingBuffer->unmap(_device);

        TextureSubResource texSubRes;
        texSubRes.baseLayer = 0;
        texSubRes.offset = { 0, 0, 0 };
        texSubRes.size.depth = 1;
        texSubRes.size.width = texSubRes.size.height = 8;
        texSubRes.mipLevelCount = 1;
        texSubRes.baseMipLevel = 0;
        texSubRes.layerCount = 1;
        subRes.size = sizeof(texData);

        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.x = 8;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 0;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 8;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );

        resourceEncoder->endEncode();

        updateCmd->endEncode();
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
        _uniformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
        _uniformDescriptor.descriptorHandle = ArgumentGroup::GetDescriptorHandle("Argument1", pipelineDesc );
        _uniformDescriptor.bufferRange = 64;

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
        _flightIndex = 0;
        return true;
    }

    void HelloWorld::tick() {
        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        // _frameCompleteFences[_flightIndex]->
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];

        cmdbuf->beginEncode(); {

            static uint64_t angle = 0;
            ++angle;
            float sinVal = sin( ((float)angle)/180.0f * 3.1415926f );
            float cosVal = cos( ((float)angle)/180.0f * 3.1415926f );

            float col2[2][4] = {
                { cosVal, -sinVal, 0, 0 },
                { sinVal,  cosVal, 0, 0 }
            };

            static ugi::RasterizationState rasterizationState;
            if( angle % 180 == 0 ) {
                rasterizationState.polygonMode = PolygonMode::Fill;
            } else if( angle % 90 == 0) {
                rasterizationState.polygonMode = PolygonMode::Line;
            }
            auto ubo = _uniformAllocator->allocate(sizeof(col2));
            ubo.writeData(0, &col2, sizeof(col2));
            _uniformDescriptor.bufferOffset = ubo.offset();
            _uniformDescriptor.buffer = ubo.buffer(); 
            _argumentGroup->updateDescriptor(_uniformDescriptor);

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
                _pipeline->setRasterizationState(rasterizationState);
                renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, _width, _height );
                renderCommandEncoder->bindPipeline(_pipeline);
                renderCommandEncoder->bindArgumentGroup(_argumentGroup);
                // renderCommandEncoder->drawIndexed( _drawable, 0, 3 );
                renderCommandEncoder->draw( _drawable, 3, 0 );
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
        
    void HelloWorld::resize(uint32_t width, uint32_t height) {
        _swapchain->resize( _device, width, height );
        //
        _width = width;
        _height = height;
    }

    void HelloWorld::release() {
    }

    const char * HelloWorld::title() {
        return "HelloWorld";
    }
        
    uint32_t HelloWorld::rendererType() {
        return 0;
    }

}

ugi::HelloWorld theapp;

UGIApplication* GetApplication() {
    return &theapp;
}