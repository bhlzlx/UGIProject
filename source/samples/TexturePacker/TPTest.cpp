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

    SDFRenderParameter sdfParam = {
        1024, // uint32_t texArraySize : 16;
        4, // uint32_t texLayer : 8;
        32, // uint32_t destinationSDFSize : 8;
        64, // uint32_t sourceFontSize : 8;
        8, // uint32_t extraSourceBorder : 8;
        8, // uint32_t searchDistance : 8;
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
        //
        _fontRenderer = new SDFFontRenderer();
        _fontRenderer->initialize( _device, assetsSource, sdfParam);
        //

        char16_t text[] = u"Pixel";
        // char16_t text[] = u"找不到路径，因为该路径不存在。PixelGame test google...";
        uint32_t fontSize[] = { 12, 18, 24, 36, 48, 60, 72, 96 };
        uint32_t fontColor[] = { 0xffff00ff, 0xff8800ff, 0x00ffffff, 0x88ff00ff, 0xff0000ff, 0x00ffffff, 0x00ff88ff, 0x0000ffff};
        
        uint32_t baseY = 24;

        _fontRenderer->beginBuild();

        uint32_t findex = 0;
        for( auto size : fontSize ) {
            std::vector<SDFChar> chars;
            for( auto& ch : text) {
                if(!ch) {
                    break;
                }
                SDFChar chr;
                chr.charCode = ch;
                chr.color = fontColor[findex];
                chr.effectColor = fontColor[findex];
                chr.fontID = 0;
                chr.fontSize = (uint32_t)size;
                chr.type = findex%4;
                chars.push_back(chr);
            }
            hgl::Vector3f transforms[2] = {
                { 1.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f }
            };
            _fontRenderer->appendText(32, baseY, chars, transforms);
            // auto drawData = _fontRenderer->buildDrawData(32, baseY, chars);
            // _drawDatas.push_back(drawData);
            baseY += 96;
            ++findex;
        }
        auto drawData = _fontRenderer->endBuild();

        _drawDatas.push_back(drawData);

        _flightIndex = 0;
        // const char16_t chars[] = u"中国智造，慧及全球。";
        return true;
    }

    void TPTest::tick() {
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();

        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];


        cmdbuf->beginEncode(); {

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();

            _fontRenderer->tickResource(resourceEncoder);
            for( auto drawData: _drawDatas) {
                _fontRenderer->prepareResource( resourceEncoder, &drawData, 1, _uniformAllocator );
            }
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
                _fontRenderer->draw( renderCommandEncoder, _drawDatas.data(), _drawDatas.size() );
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
        _fontRenderer->resize( width, height );
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