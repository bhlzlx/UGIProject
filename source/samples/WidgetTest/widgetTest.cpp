﻿#include "widgetTest.h"
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

#include <gdi/gdi.h>
#include <gdi/widget/GdiSystem.h>
#include <gdi/widget/widget.h>
#include <gdi/widget/component.h>

#include <cmath>
#include <time.h>
#include <random>

#include <tweeny.h>

// scale/angle/alpha
auto tweenTest = tweeny::from(1.0f, 0.0f, 0.8f).to(0.2f, 360.0f,0.0f).during(120);

namespace ugi {

    bool WidgetTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {

        printf("initialize\n");

        m_renderSystem = new ugi::RenderSystem();

        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        m_device = m_renderSystem->createDevice(descriptor, assetsSource);
        m_uniformAllocator = m_device->createUniformAllocator();
        m_swapchain = m_device->createSwapchain( _wnd );
        // command queues
        m_graphicsQueue = m_device->graphicsQueues()[0];
        m_uploadQueue = m_device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            m_frameCompleteFences[i] = m_device->createFence();
            m_renderCompleteSemaphores[i] = m_device->createSemaphore();
            m_commandBuffers[i] = m_graphicsQueue->createCommandBuffer( m_device );
        }

        m_GdiContext = new ugi::gdi::GDIContext(m_device, assetsSource);
        if (!m_GdiContext->initialize()) {
            return false;
        }
        m_UiSys = new ugi::gdi::UI2DSystem();
        if(!m_UiSys->initialize(m_GdiContext)) {
            return false;
        }
        //
        auto root = m_UiSys->root();
        srand(time(0));
        auto mainComp = root->createComponent();
        root->addWidget(mainComp);
		mainComp->setName("main"); {
            for( uint32_t i = 0; i<1; ++i) {                
                auto rcwgt = mainComp->createColoredRectangle( 0xffffffff );
                rcwgt->setRect( hgl::RectScope2f( 16, 16, 32, 32) );
                mainComp->addWidget(rcwgt);
            }
        }
        mainComp->setScissor(0, 0, 512, 512);

        m_flightIndex = 0;
        return true;
    }

    void WidgetTest::tick() {

        m_device->waitForFence( m_frameCompleteFences[m_flightIndex] );
        //
        m_uniformAllocator->tick();
        m_UiSys->onTick();
        //
        uint32_t imageIndex = m_swapchain->acquireNextImage( m_device, m_flightIndex );

        IRenderPass* mainRenderPass = m_swapchain->renderPass(imageIndex);
        
        auto cmdbuf = m_commandBuffers[m_flightIndex];

        auto valueArray = tweenTest.step(1);
        if( tweenTest.progress() == 1.0f ) {
            tweenTest = tweenTest.backward();
        } else if(tweenTest.progress() == 0.0f){
            tweenTest = tweenTest.forward();
        }

        auto comp = (gdi::Component*)(m_UiSys->root()->widgets()[0]);
        gdi::ColoredRectangle* rc = (gdi::ColoredRectangle*)comp->widgets()[0];
        gdi::Transform transform;
        transform.anchor = hgl::Vector2f( 32,32 );
        transform.offset = hgl::Vector2f(0,0);
        transform.radian = valueArray[1] / 180.f * 3.1415926f;
        transform.scale = hgl::Vector2f(valueArray[0], valueArray[0]);
        rc->setTransform(transform);
        srand(time(0));
        uint8_t red = rand() % 256;
        uint8_t green = rand() % 256;
        uint8_t blue = rand() % 256;
        uint32_t colorMask = (red<<24) | (green<<16) | (blue <<8) | (uint32_t)(valueArray[2]*256.0f);
        rc->setColorMask(colorMask);

        cmdbuf->beginEncode(); {
            hgl::Vector2f screenSize(m_width, m_height);
            {   ///> resource command encoder
                auto resEncoder = cmdbuf->resourceCommandEncoder(); {
                    m_UiSys->prepareResource(resEncoder, m_uniformAllocator);
                }
                resEncoder->endEncode();
            }
            {   ///> render pass command encoder
                RenderPassClearValues clearValues;
                clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
                clearValues.depth = 1.0f;
                clearValues.stencil = 0xffffffff;

                mainRenderPass->setClearValues(clearValues);

                auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
					renderCommandEncoder->setViewport(0, 0, m_width, m_height, 0, 1.0f);
					renderCommandEncoder->setScissor(0, 0, m_width, m_height);
                    m_UiSys->draw(renderCommandEncoder);
                }
                renderCommandEncoder->endEncode();
            }
            
        }
        cmdbuf->endEncode();

		Semaphore* imageAvailSemaphore = m_swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&cmdbuf,
			1,
			&imageAvailSemaphore,// submitInfo.semaphoresToWait
			1,
			&m_renderCompleteSemaphores[m_flightIndex], // submitInfo.semaphoresToSignal
			1
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, m_frameCompleteFences[m_flightIndex]);
        bool submitRst = m_graphicsQueue->submitCommandBuffers(submitBatch);
        // assert(submitRst);
		if (!submitRst) {
			printf("submit command failed!");
		}
        m_swapchain->present( m_device, m_graphicsQueue, m_renderCompleteSemaphores[m_flightIndex] );

    }
        
    void WidgetTest::resize(uint32_t _width, uint32_t _height) {
        m_swapchain->resize( m_device, _width, _height );
        //
        m_width = _width;
        m_height = _height;
        m_GdiContext->setSize( hgl::Vector2f(_width, _height) );
    }

    void WidgetTest::release() {
    }

    const char * WidgetTest::title() {
        return "WidgetTest";
    }
        
    uint32_t WidgetTest::rendererType() {
        return 0;
    }
}

UGIApplication* GetApplication() {
    static ugi::WidgetTest app;
    return &app;
}