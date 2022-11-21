#include "GaussBlur.h"
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
#include <ugi/render_components/Renderable.h>
#include <ugi/UniformBuffer.h>
// #include <ugi/TextureUtility.h>
#include <ugi/TextureKTX.h>
#include <ugi/RenderContext.h>
#include "GaussBlurProcessor.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <tweeny.h>

#include <LightWeightCommon/io/archive.h>

namespace ugi {

    bool GaussBlurTest::initialize(void* _wnd, comm::IArchive* archive) {
        printf("initialize\n");
        renderContext_ = new StandardRenderContext();
        auto file = archive->openIStream("image/island.png", {comm::ReadFlag::binary});
        size_t fileSize = file->size();
        uint8_t* fileBuff = (uint8_t*)malloc(fileSize);
        file->read(fileBuff, fileSize);
        file->close();
        CreateTextureKTX
        _texture = texUtil.createTexturePNG(fileBuff, fileSize);
        free(fileBuff);
        delete fileStream;

        _bluredTexture = _device->createTexture(_texture->desc(), ResourceAccessType::ShaderReadWrite );
        _bluredTextureFinal = _device->createTexture(_texture->desc(), ResourceAccessType::ShaderReadWrite);

        // fileStream = assetsSource->Open(u8"image/island.ktx");
        // if(fileStream) {
        //     fileBuff = (uint8_t*)malloc(fileStream->GetSize());
        //     fileStream->ReadFully( fileBuff,fileStream->GetSize());
        //     TextureUtility texUtil(_device, _uploadQueue);
        //     _texture = texUtil.createTextureKTX(fileBuff, fileStream->GetSize());
        // }
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
        parameter.direction[0] = 0.0f; parameter.direction[1] = 1.0f;
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
            renderpass_clearval_t clearValues;
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
            resEncoder->blitImage( _bluredTextureFinal, _texture, &dstRegion, &srcRegion, 1 );
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