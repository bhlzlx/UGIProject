﻿#include "GaussBlur.h"
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

#include "GaussBlurProcessor.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <tweeny.h>

namespace ugi {

    void TextureUtility::replaceTexture( CommandQueue* transferQueue, Texture* texture, const ImageRegion* regions,void* data, uint32_t dataLength, uint32_t* offsets, uint32_t regionCount  ) {

        auto commandBuffer = transferQueue->createCommandBuffer(_device); 
        auto stagingBuffer = _device->createBuffer( ugi::BufferType::StagingBuffer, dataLength );
        commandBuffer->beginEncode(); {
            auto resCmdEncoder = commandBuffer->resourceCommandEncoder();
            void* mappingPtr = stagingBuffer->map(_device);
            memcpy( mappingPtr, data, dataLength);
            stagingBuffer->unmap(_device);
            resCmdEncoder->replaceImage( texture, stagingBuffer, regions, offsets, regionCount);
            resCmdEncoder->endEncode();
        }
        commandBuffer->endEncode();
		QueueSubmitInfo submitInfo {
			&commandBuffer,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, nullptr );
        _transferQueue->submitCommandBuffers(submitBatch);
        _transferQueue->waitIdle();
        //
        stagingBuffer->release(_device);
    }

    bool GaussBlurTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {

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
        //
        _flightIndex = 0;
        //
        auto fileStream = assetsSource->Open("image/island.png");
        size_t fileSize = fileStream->GetSize();
        uint8_t* fileBuff = (uint8_t*)malloc(fileStream->GetSize());
        fileStream->ReadFully(fileBuff, fileStream->GetSize());
        fileStream->Close();
        int x; int y; int channels;
        auto pixel = stbi_load_from_memory( fileBuff, fileSize,&x, &y, &channels, 4 );
        //
        TextureDescription textureDescription;
        textureDescription.format = ugi::UGIFormat::RGBA8888_UNORM;
        textureDescription.width = x;
        textureDescription.height = y;
        textureDescription.depth = 1;
        textureDescription.mipmapLevel = 1;
        textureDescription.arrayLayers = 1;
        textureDescription.type = TextureType::Texture2D;
        _texture = _device->createTexture(textureDescription);
        _bluredTexture = _device->createTexture(textureDescription);
        _bluredTextureFinal = _device->createTexture(textureDescription);
        //
        delete fileStream;
        TextureUtility textureUtility(_device, _uploadQueue, _graphicsQueue);
        ImageRegion region;
        region.extent = ImageRegion::Extent(x, y, 1);
        uint32_t offset = 0;
        textureUtility.replaceTexture( _uploadQueue, _texture, &region, pixel, x*y*4, &offset, 1 );
        //
		_gaussProcessor = new GaussBlurProcessor();
        auto rst = _gaussProcessor->intialize(_device, assetsSource);
        _blurItem = _gaussProcessor->createGaussBlurItem(_bluredTexture, _bluredTextureFinal);

        auto distributions = GenerateGaussDistribution(1.8f);

        GaussBlurParameter parameter = {
            { 1.0f, 0.0f }, distributions.size()/2+1, 0,
            {},
        };
        memcpy( parameter.gaussDistribution, distributions.data()+distributions.size()/2, (distributions.size()/2+1)*sizeof(float) );
        _blurItem->setParameter(parameter);
        _blurItem2 = _gaussProcessor->createGaussBlurItem(_bluredTextureFinal, _bluredTexture);
        parameter.direction[0] = 0.0f;
        parameter.direction[1] = 1.0f;
        _blurItem2->setParameter(parameter);
        //
        return true;
    }

    void GaussBlurTest::tick() {
        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );

        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];

        cmdbuf->beginEncode(); {
            //
            RenderPassClearValues clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            ugi::ImageRegion srcRegion;
            srcRegion.arrayIndex = 0; srcRegion.mipLevel = 0;
            srcRegion.offset = {};
            srcRegion.extent = { _texture->desc().width, _texture->desc().height, 1 };
            ugi::ImageRegion dstRegion;
            dstRegion.arrayIndex = 0; dstRegion.mipLevel = 0;
            dstRegion.offset = {};
            dstRegion.extent = { _texture->desc().width, _texture->desc().height, 1 };

            auto resEncoder = cmdbuf->resourceCommandEncoder();
            resEncoder->blitImage( _bluredTexture, _texture, &dstRegion, &srcRegion, 1 );
            resEncoder->endEncode();

            static auto tween = tweeny::from(0).to(12).during(60);
            auto v = tween.step(1);
            auto progress = tween.progress();
            if( progress == 1.0f ) {
                tween = tween.backward();
            } else if( progress == 0.0f ) {
                tween = tween.forward();
            }
            for( int i = 0; i<v; ++i) {
                _gaussProcessor->processBlur(_blurItem, cmdbuf, _uniformAllocator);
                _gaussProcessor->processBlur(_blurItem2, cmdbuf, _uniformAllocator);
            }
            resEncoder = cmdbuf->resourceCommandEncoder();
            Texture* screen = mainRenderPass->color(0);
            resEncoder->blitImage( screen, _bluredTextureFinal, &dstRegion, &srcRegion, 1 );
            dstRegion.offset = { (int32_t)srcRegion.extent.width, 0, 0 };
            resEncoder->blitImage( screen, _texture, &dstRegion, &srcRegion, 1 );
            
            resEncoder->endEncode();

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
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
        
    void GaussBlurTest::resize(uint32_t width, uint32_t height) {
        _swapchain->resize( _device, width, height );
        //
        _width = width;
        _height = height;
    }

    void GaussBlurTest::release() {
    }

    const char * GaussBlurTest::title() {
        return "HelloWorld";
    }
        
    uint32_t GaussBlurTest::rendererType() {
        return 0;
    }

}

ugi::GaussBlurTest theapp;

UGIApplication* GetApplication() {
    return &theapp;
}