﻿#include "TPTest.h"
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
        //
        _fontRenderer = new SDFFontRenderer();
        _fontRenderer->initialize( _device, assetsSource );
        //
        std::vector<SDFChar> text = {
            { 0, 72, u'中'},
            { 0, 72, u'国'},
            { 0, 72, u'智'},
            { 0, 72, u'造'},
            { 0, 64, u'慧'},
            { 0, 64, u'及'},
            { 0, 64, u'全'},
            { 0, 64, u'球'},
            { 0, 48, u'中'},
            { 0, 48, u'国'},
            { 0, 48, u'智'},
            { 0, 48, u'造'},
            { 0, 32, u'慧'},
            { 0, 32, u'及'},
            { 0, 32, u'全'},
            { 0, 32, u'球'},
            { 0, 28, u'中'},
            { 0, 28, u'国'},
            { 0, 28, u'智'},
            { 0, 28, u'造'},
            { 0, 18, u'慧'},
            { 0, 18, u'及'},
            { 0, 18, u'全'},
            { 0, 18, u'球'},
            { 0, 14, u'中'},
            { 0, 14, u'国'},
            { 0, 14, u'智'},
            { 0, 14, u'造'},
            { 0, 12, u'慧'},
            { 0, 12, u'及'},
            { 0, 12, u'全'},
            { 0, 12, u'球'},
        };
        /*std::vector<SDFChar> text = {
            { 0, 48, u'p'},
            { 0, 48, u'h'},
            { 0, 48, u'a'},
            { 0, 48, u'n'},
            { 0, 48, u't'},
            { 0, 48, u'o'},
            { 0, 48, u'm'},
            { 0, 48, u'k'},
            { 0, 48, u'u'},
            { 0, 48, u's'},
            { 0, 48, u'u'},
            { 0, 48, u'g'},
            { 0, 48, u'a'},
            { 0, 48, u'w'},
            { 0, 48, u'a'},
            { 0, 48, u'!'},
        };*/
        _drawData = _fontRenderer->buildDrawData(32, 96, text);
        _flightIndex = 0;
        // const char16_t chars[] = u"中国智造，慧及全球。";
        return true;
    }

    void TPTest::tick() {
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        // 测试用。。。
        _drawData->setScreenSize( _width, _height);

        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];


        cmdbuf->beginEncode(); {

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();

            _fontRenderer->tickResource(resourceEncoder);
            _fontRenderer->prepareResource( resourceEncoder, &_drawData, 1, _uniformAllocator );
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
                _fontRenderer->draw( renderCommandEncoder, &_drawData, 1 );
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