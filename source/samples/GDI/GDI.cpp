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

namespace ugi {

    bool GDISample::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {

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
        // m_uniformAllocator = m_device->createUniformAllocator();
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

        m_gdiContext = new ugi::gdi::GDIContext(m_device, assetsSource);

        if (!m_gdiContext->initialize()) {
            return false;
        }

        m_flightIndex = 0;
        return true;
    }

    void GDISample::tick() {

        if (!m_geomDrawData) {
            return;
        }

        static int angle = 0;
        ++angle;
        float rad = (float)angle/180.0f*3.141592654;

        // for( uint32_t i = 0; i<16; i++) {
        //     for( uint32_t j = 0; j<16; j++) {
        //         m_geomDrawData->updateGeometryTranslation( i*16+j+1, hgl::Vector2f(i*24+12, j*24+12), hgl::Vector2f(1.5f, 1.5f), rad);
        //     }
        // }

        // for( uint32_t i = 0; i<2;++i ) {
        //     for( uint32_t j = 1; j<64; ++j ) {
        //         m_geomDrawData->updateGeometryTranslation(j,hgl::Vector2f(0, 0), hgl::Vector2f(1.5f, 1.5f), 0 );
        //     }
        // }
        
        m_device->waitForFence( m_frameCompleteFences[m_flightIndex] );
        // m_uniformAllocator->tick();
        uint32_t imageIndex = m_swapchain->acquireNextImage( m_device, m_flightIndex );
        // m_frameCompleteFences[m_flightIndex]->
        IRenderPass* mainRenderPass = m_swapchain->renderPass(imageIndex);
        
        auto cmdbuf = m_commandBuffers[m_flightIndex];

        cmdbuf->beginEncode(); {
            hgl::Vector2f screenSize(m_width, m_height);
            //
            RenderPassClearValues clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                renderCommandEncoder->setLineWidth(1.0f);
                renderCommandEncoder->setViewport(0, 0, m_width, m_height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, m_width, m_height );
                renderCommandEncoder->bindPipeline(m_gdiContext->pipeline());

                m_geomDrawData->draw(renderCommandEncoder);

            }
            renderCommandEncoder->endEncode();
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
        
    void GDISample::resize(uint32_t _width, uint32_t _height) {
        m_swapchain->resize( m_device, _width, _height );
        //
        m_width = _width;
        m_height = _height;
        //
        m_gdiContext->setSize( hgl::Vector2f(_width, _height) );
		if (!m_geomDrawData) {
			if (!m_geomBuilder) {
				m_geomBuilder = ugi::gdi::CreateGeometryBuilder(m_gdiContext);
				m_geomBuilder->prepareBuildGeometry(512);
				m_geomBuilder->drawLine(hgl::Vector2f(4, 4), hgl::Vector2f(200, 200), 1, 0xffff0088);
				srand(time(0));
				for (uint32_t i = 0; i<16; i++) {
					for (uint32_t j = 0; j<16; j++) {
						uint32_t color = 0x88 | (rand() % 0xff) << 8 | (rand() % 0xff) << 16 | (rand() % 0xff) << 24;
						m_geomBuilder->drawRect(i * 24, j * 24, 22, 22, color, true);
					}
				}
				m_geomDrawData = m_geomBuilder->endBuildGeometry();
			}
		}
               

        // for( uint32_t i = 0; i<16; i++) {
        //     for( uint32_t j = 0; j<16; j++) {
        //         m_geomDrawData->updateGeometryTranslation( i*16+j+1, hgl::Vector2f(i*24, j*24), hgl::Vector2f(1.2f, 1.2f), 3.1415926f / 16 * j);
        //     }
        // }
        
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