#include "TPTest.h"
#include <cassert>
#include <ugi/device.h>
#include <ugi/swapchain.h>
#include <ugi/command_queue.h>
#include <ugi/command_buffer.h>
#include <ugi/buffer.h>
#include <ugi/render_pass.h>
#include <ugi/semaphore.h>
#include <ugi/pipeline.h>
#include <ugi/descriptor_binder.h>
#include <ugi/texture.h>
#include <ugi/Drawable.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/flight_cycle_invoker.h>
#include <hgl/assets/AssetsSource.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <cmath>

#include <tweeny.h>

#include <random>

namespace ugi {

    hgl::Vector2f anchor3;

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
        ugi::device_descriptor_t descriptor; {
            descriptor.apiType = ugi::GraphicsAPIType::VULKAN;
            descriptor.deviceType = ugi::GraphicsDeviceType::DISCRETE;
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

        char16_t textArray[][64] = {
            u".找不到路径，",
            u"因为该路径不存在。",
            u"全英雄选择",
            u"testgoogle..."
        };
        Style styleArray[5] = {
            { 0xff5555ff, 0xffffffff, 0x00000000 },
            { 0x55ff55ff, 0xffffffff, 0x00000001 },
            { 0x5555ffff, 0xffffffff, 0x00000002 },
            { 0xff0000ff, 0xffffffff, 0x00000003 },
            { 0x005555ff, 0xffffffff, 0x00000002 },
        };
        uint32_t fontSize[] = { 12, 18, 24, 36, 48, 60, 72, 96 };
        uint32_t fontColor[] = { 0xffff00ff, 0xff8800ff, 0x00ffffff, 0x88ff00ff, 0xff0000ff, 0x00ffffff, 0x00ff88ff, 0x0000ffff};
        
        uint32_t baseY = 360;

        _fontRenderer->beginBuild();

        hgl::RectScope2f rc;

        Transform identity = { hgl::Vector3f(1.0f, 0.0f, 0.0f), hgl::Vector3f(0.0f, 1.0f, 0.0f) };
        std::vector<SDFChar> vecChar;
        for( auto& ch : textArray[0]) {
            if(!ch) {
                break;
            }
            SDFChar chr = { 0, 36U, ch };
            vecChar.push_back(chr);
        }
        _h1 = _fontRenderer->appendText(32, baseY, vecChar.data(), vecChar.size(), identity, styleArray[0], rc);

        vecChar.clear();
        for( auto& ch : textArray[1]) {
            if(!ch) {
                break;
            }
            SDFChar chr = { 0, 36U, ch };
            vecChar.push_back(chr);
        }
        _h2 = _fontRenderer->appendTextReuseTransform(rc.GetRight(), baseY, vecChar.data(), vecChar.size(), _h1, styleArray[1], rc);

        vecChar.clear();
        for( auto& ch : textArray[2]) {
            if(!ch) {
                break;
            }
            SDFChar chr = { 0, 36U, ch };
            vecChar.push_back(chr);
        }
        _h3 = _fontRenderer->appendTextReuseStyle(rc.GetRight(), baseY, vecChar.data(), vecChar.size(), _h2, identity, rc);

        anchor3.Set( rc.GetCenterX(), baseY);

        vecChar.clear();
        for( auto& ch : textArray[3]) {
            if(!ch) {
                break;
            }
            SDFChar chr = { 0, 36U, ch };
            vecChar.push_back(chr);
        }
        _h4 = _fontRenderer->appendText(rc.GetRight(), baseY, vecChar.data(), vecChar.size(), identity, styleArray[2], rc);

        _drawData = _fontRenderer->endBuild();

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

        static auto t = tweeny::from(1.5).to(4.0).during(60);
        if(t.progress() == 0.0f) {
            t = t.forward();
        } else if( t.progress() == 1.0f ) {
            t.backward();
        }
        auto scale = t.step(1);

        auto trans = Transform::createTransform( anchor3, hgl::Vector2f(scale) );
        _drawData->updateTransform(_h3, trans);

        static auto redTween = tweeny::from(1.0).to(0.0).during(120);
        static auto blueTween = tweeny::from(0.0).to(1.0).during(90);
        static auto greenTween = tweeny::from(1.0).to(0.0).during(180);
        std::array<tweeny::tween<double>*,3> t3 = {
            &redTween, &blueTween, &greenTween
        };
        float rgb[3];
        for( uint32_t i = 0; i<3; ++i ) {
            auto t = t3[i];
            if(t->progress() == 0.0f) {
                *t = t->forward();
            } else if( t->progress() == 1.0f ) {
                t->backward();
            }
            rgb[i] = t->step(1);
        }
        uint32_t color = (uint32_t)(rgb[0]*255)<<24 | (uint32_t)(rgb[1]*255)<<16 | (uint32_t)(rgb[2]*255)<<8 | 0xff;
        Style style = { color, ~color | 0xff, 0x00000001 };

        _drawData->updateStyle(_h2, style);


        cmdbuf->beginEncode(); {

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();

            _fontRenderer->tickResource(resourceEncoder);
            _fontRenderer->prepareResource( resourceEncoder, &_drawData, 1, _uniformAllocator );
            resourceEncoder->endEncode();
            //
            renderpass_clearval_t clearValues;
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