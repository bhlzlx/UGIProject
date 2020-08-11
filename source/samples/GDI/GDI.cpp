#include "GDI.h"
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
#include <gdi/geometry/geometryBuilder.h>

#include <cmath>
#include <time.h>

#include <tweeny.h>

namespace ugi {

    bool GDISample::initialize( void* wnd, hgl::assets::AssetsSource* assetsSource ) {

        printf("initialize\n");

        _renderSystem = new ugi::RenderSystem();

        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = wnd;
        }
        _device = _renderSystem->createDevice(descriptor, assetsSource);
        _uniformAllocator = _device->createUniformAllocator();
        _swapchain = _device->createSwapchain( wnd );
        // command queues
        _graphicsQueue = _device->graphicsQueues()[0];
        _uploadQueue = _device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            _frameCompleteFences[i] = _device->createFence();
            _renderCompleteSemaphores[i] = _device->createSemaphore();
            _commandBuffers[i] = _graphicsQueue->createCommandBuffer( _device );
        }

        _gdiContext = new ugi::gdi::GDIContext(_device, assetsSource);

        if (!_gdiContext->initialize()) {
            return false;
        }

        _flightIndex = 0;
        return true;
    }

    void GDISample::tick() {

        if (!_geomDrawData) {
            return;
        }

        static int angle = 0;
        ++angle;
        float rad = (float)angle/180.0f*3.141592654;

        static auto tween = tweeny::from(0.5f, 0.0f).to(1.5f, 360.0f).during(100);

        auto v = tween.step(1);
        auto progress = tween.progress();
        if( progress == 1.0f ) {
            tween = tween.backward();
        } else if( progress == 0.0f ) {
            tween = tween.forward();
        }

        hgl::Vector2f elementAnchor( 16*24-2-11, 16*24-2-11 );
        _geomDrawData->setElementTransform( 256,elementAnchor, hgl::Vector2f(0.8f, 0.8f), (v[1]/180.0f)*3.1415926f );
        _geomDrawData->setTransform( elementAnchor, hgl::Vector2f(v[0], v[0]), 0);// (v[1]/180.0f)*3.1415926f);        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );

        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];

        cmdbuf->beginEncode(); {
            hgl::Vector2f screenSize(_width, _height);
            //
            {   ///> resource command encoder
                auto resEncoder = cmdbuf->resourceCommandEncoder();
                _geomDrawData->prepareResource(resEncoder, _uniformAllocator);
                resEncoder->endEncode();
            }
            {   ///> render pass command encoder
                RenderPassClearValues clearValues;
                clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
                clearValues.depth = 1.0f;
                clearValues.stencil = 0xffffffff;

                mainRenderPass->setClearValues(clearValues);

                auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                    renderCommandEncoder->setLineWidth(1.0f);
                    renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f );
                    renderCommandEncoder->setScissor( 0, 0, _width, _height );
                    renderCommandEncoder->bindPipeline(_gdiContext->pipeline());

                    _geomDrawData->draw(renderCommandEncoder);

                }
                renderCommandEncoder->endEncode();
            }
            
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
        // assert(submitRst);
		if (!submitRst) {
			printf("submit command failed!");
		}
        _swapchain->present( _device, _graphicsQueue, _renderCompleteSemaphores[_flightIndex] );

    }
        
    void GDISample::resize(uint32_t width, uint32_t height) {
        _swapchain->resize( _device, width, height );
        //
        _width = width;
        _height = height;
        //
        if(_geomDrawData) {
            delete _geomDrawData;
        }
        _gdiContext->setSize( hgl::Vector2f(_width, _height) );
        //
        if (!_geomBuilder) {
            _geomBuilder = ugi::gdi::CreateGeometryBuilder(_gdiContext);
		}

		_geomBuilder->beginBuild();
		// m_geomBuilder->drawLine(hgl::Vector2f(4, 4), hgl::Vector2f(200, 200), 1, 0xffff0088);
		srand(time(0));
		for (uint32_t i = 0; i < 16; i++) {
			for (uint32_t j = 0; j < 16; j++) {
				uint32_t color = 0x88 | (rand() % 0xff) << 8 | (rand() % 0xff) << 16 | (rand() % 0xff) << 24;
				_geomBuilder->drawRect(i * 24, j * 24, 22, 22, color, true);
			}
		}
		_geomDrawData = _geomBuilder->endBuild();
        _geomDrawData->setScissor(11, 256, 11, 256);
    }

    void GDISample::release() {
    }

    const char * GDISample::title() {
        return "GDI";
    }
        
    uint32_t GDISample::rendererType() {
        return 0;
    }

}

ugi::GDISample theapp;

UGIApplication* GetApplication() {
    return &theapp;
}