#include "widgetTest.h"
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

namespace ugi {

    std::default_random_engine randEngine;
    // scale/angle/alpha/x/y
    tweeny::tween<float, float, float, float, float> CreateMouseEffectTween( const hgl::Vector2f& point ) {
        float x = point.x; float y = point.y;
        float angle = randEngine()%360;
        float radian = angle / 180 * 3.1415926f;
        float dx = cos(radian); float dy = sin(radian);
        float destX = x + dx * 96;
        float destY = y + dy * 96;
        auto rst = tweeny::from(.2f, 0.0f, 1.0f, x, y).to(1.0f, 360.0f,0.0f, destX, destY).during(60);
        return rst;
    }

    struct TweenItem {
        tweeny::tween<float, float, float, float, float>    tween;
        uint32_t                                            color;
        gdi::Widget*                                        widget;
    };

    std::vector<TweenItem> tweenList;

    bool WidgetTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {

        printf("initialize\n");
        this->_hwnd = _wnd;

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

        m_GdiContext = new ugi::gdi::GDIContext(_device, assetsSource);
        if (!m_GdiContext->initialize()) {
            return false;
        }
        m_UiSys = new ugi::gdi::UI2DSystem();
        if(!m_UiSys->initialize(m_GdiContext)) {
            return false;
        }
        //
        auto root = m_UiSys->root();
        auto mainComp = root->createComponent();
        root->addWidget(mainComp);
		mainComp->setName("main"); {
            for( uint32_t i = 0; i<96; ++i) {                
                auto rcwgt = mainComp->createColoredRectangle( 0xffffffff );
                rcwgt->setRect( hgl::RectScope2f( -16, -16, 32, 32) );
                mainComp->addWidget(rcwgt);
                auto tween = CreateMouseEffectTween(hgl::Vector2f(64,64));
                auto step = randEngine()%60;
                tween.step(step);
                tweenList.push_back( { std::move(tween), 0xffffffff, (gdi::Widget*)rcwgt} );
            }
        }
        mainComp->setScissor(32, 32, 512, 512);
        _flightIndex = 0;
        return true;
    }

    void WidgetTest::tick() {

        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        //
        _uniformAllocator->tick();
        m_UiSys->onTick();
        //
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );

        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];

        POINT point;
        GetCursorPos(&point);			// 获取鼠标指针位置（屏幕坐标）
		ScreenToClient((HWND)_hwnd, &point);	// 将鼠标指针位置转换为窗口坐标

        for( auto& tweenItem: tweenList) {
            auto values = tweenItem.tween.step(1);
            gdi::Transform transform;
            transform.anchor = hgl::Vector2f( 0,0 );
            transform.radian = values[1] / 180.f * 3.1415926f;
            transform.scale = hgl::Vector2f(values[0], values[0]);
            transform.offset.Set(values[3], values[4]);
            tweenItem.widget->setTransform(transform);
            uint32_t color = tweenItem.color;
            color &= 0xffffff00;
            color |= (uint32_t)(values[2]*256.0f);
            tweenItem.widget->setColorMask(color);
            if(tweenItem.tween.progress()==1.0f) {
                tweenItem.tween = CreateMouseEffectTween(hgl::Vector2f(point.x,point.y));
                uint8_t red = randEngine() % 256;
                uint8_t green = randEngine() % 256;
                uint8_t blue = randEngine() % 256;
                uint32_t colorMask = (red<<24) | (green<<16) | (blue <<8) | 0xff;
                tweenItem.color = colorMask;
            }
        }

        cmdbuf->beginEncode(); {
            hgl::Vector2f screenSize(_width, _height);
            {   ///> resource command encoder
                auto resEncoder = cmdbuf->resourceCommandEncoder(); {
                    m_UiSys->prepareResource(resEncoder, _uniformAllocator);
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
					renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f);
					renderCommandEncoder->setScissor(0, 0, _width, _height);
                    m_UiSys->draw(renderCommandEncoder);
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
        
    void WidgetTest::resize(uint32_t width, uint32_t height) {
        _swapchain->resize( _device, width, height );
        //
        _width = width;
        _height = height;
        m_GdiContext->setSize( hgl::Vector2f(width, height) );
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