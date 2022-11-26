#include "GaussBlur.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cmath>
#include <tweeny.h>

#include <cassert>
#include <ugi/device.h>
#include <ugi/swapchain.h>


#include <ugi/command_queue.h>
#include <ugi/command_buffer.h>
#include <ugi/buffer.h>
#include <ugi/render_pass.h>


#include <ugi/semaphore.h>
#include <ugi/pipeline.h>
#include <ugi/helper/pipeline_helper.h>
#include <ugi/descriptor_binder.h>
#include <ugi/texture.h>
#include <ugi/render_components/Renderable.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/render_components/pipeline_material.h>
#include <ugi/render_context.h>
// #include "GaussBlurProcessor.h"

#include <LightWeightCommon/io/archive.h>

#include <ugi/texture_util.h>

namespace ugi {

    bool GaussBlurTest::initialize(void* _wnd, comm::IArchive* archive) {
        printf("initialize\n");
        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        };
        renderContext_ = new StandardRenderContext();
        renderContext_->initialize(_wnd, descriptor, archive);
        auto device = renderContext_->device();
        auto asyncLoadManager = renderContext_->asyncLoadManager();
        { // 创建管线 
            auto pplfile = archive->openIStream("shaders/GaussBlur/pipeline.bin", {comm::ReadFlag::binary});
            PipelineHelper pplhelper = PipelineHelper::FromIStream(pplfile);
            pipeline_ = device->createComputePipeline(pplhelper.desc());
        }
        // 创建纹理
        auto file = archive->openIStream("image/ushio.png", {comm::ReadFlag::binary});
        size_t fileSize = file->size();
        uint8_t* fileBuff = (uint8_t*)malloc(fileSize);
        file->read(fileBuff, fileSize);
        file->close();
        texture_ = CreateTexturePNG(device, fileBuff, fileSize, asyncLoadManager, 
            [](void* res, CommandBuffer* cmd) {
                Texture* texture = (Texture*)res;
                texture->generateMipmap(cmd); // 生成mipmap
                auto resEnc = cmd->resourceCommandEncoder();
                resEnc->imageTransitionBarrier(
                    texture, ResourceAccessType::ShaderRead, 
                    PipelineStages::Bottom, StageAccess::Write,
                    PipelineStages::FragmentShading, StageAccess::Read,
                    nullptr
                );
                resEnc->endEncode();
                texture->markAsUploaded();
            }
        );

        for(uint32_t i = 0; i<2; ++i) {
            blurTextures_[i] = device->createTexture(texture_->desc(), ResourceAccessType::ShaderReadWrite);
            blurImageViews_[i] = blurTextures_[i]->createImageView(device, image_view_param_t{});
            blurMaterials_[i]= pipeline_->createMaterial({"InputImage", "OutputImage", "BlurParameter"},{});
        }
        // update materials, except uniform buffer
        for(uint32_t i = 0; i<2; ++i) {
            auto input = blurMaterials_[i]->descriptors()[0];
            input.res.imageView = blurImageViews_[i%2].handle;
            auto output = blurMaterials_[i]->descriptors()[1];
            output.res.imageView = blurImageViews_[(i+1)%2].handle;
            blurMaterials_[i]->updateDescriptor(input); // input image
            blurMaterials_[i]->updateDescriptor(output); // output image
        }
        return true;
    }

    void GaussBlurTest::tick() {
        renderContext_->onPreTick();
        auto device = renderContext_->device();
        auto queue = renderContext_->primaryQueue();
        auto cmdbuf = queue->createCommandBuffer(device, ugi::CmdbufType::Transient);
        device->cycleInvoker().postCallable([queue, device, cmdbuf](){
            queue->destroyCommandBuffer(device, cmdbuf);
        });
        cmdbuf->beginEncode(); {
            renderpass_clearval_t clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;
            auto fbo = renderContext_->mainFramebuffer();
            fbo->setClearValues(clearValues);
            if(texture_->uploaded()) {
                auto imgWidth = texture_->desc().width;
                auto imgHeight = texture_->desc().height;
                ugi::ImageRegion srcRegion;
                srcRegion.arrayIndex = 0; srcRegion.mipLevel = 0;
                srcRegion.offset = {};
                srcRegion.extent = { imgWidth, imgHeight, 1 };
                ugi::ImageRegion dstRegion;
                dstRegion.arrayIndex = 0; dstRegion.mipLevel = 0;
                dstRegion.offset = {};
                dstRegion.extent = { imgWidth, imgHeight, 1 };

                static auto tween = tweeny::from(0).to(12).during(60);
                auto v = tween.step(1);
                auto progress = tween.progress();
                if( progress == 1.0f ) {
                    tween = tween.backward();
                } else if( progress == 0.0f ) {
                    tween = tween.forward();
                }

                auto resEnc = cmdbuf->resourceCommandEncoder();
                resEnc->blitImage(blurTextures_[0], texture_, &dstRegion, &srcRegion, 1 );
                resEnc->blitImage(blurTextures_[1], texture_, &dstRegion, &srcRegion, 1 );
                resEnc->endEncode();

                auto distributions = GenerateGaussDistribution(1.8f);
                GaussBlurParameter parameter = {
                    { 1.0f, 0.0f }, (uint32_t)distributions.size()/2+1, 0,
                    {},
                };
                memcpy(parameter.gaussDistribution, distributions.data()+distributions.size()/2, (distributions.size()/2+1)*sizeof(float) );

                for(auto i = 0; i<2; ++i) { // update uniform buffer
                    if(i == 1) {
                        parameter.direction[0] = 0.0f; parameter.direction[1] = 1.0f;
                    }
                    auto tor = blurMaterials_[i]->descriptors()[2];
                    renderContext_->uniformAllocator()->allocateForDescriptor(tor, &parameter);
                    blurMaterials_[i]->updateDescriptor(tor);
                }

                for( int i = 0; i<v; ++i) {
                    resEnc = cmdbuf->resourceCommandEncoder();
                    resEnc->imageTransitionBarrier(blurTextures_[0], ResourceAccessType::ShaderReadWrite, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Read);
                    resEnc->imageTransitionBarrier(blurTextures_[1], ResourceAccessType::ShaderWrite, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Read);
                    resEnc->endEncode();
                    pipeline_->applyMaterial(blurMaterials_[0]);
                    pipeline_->flushMaterials(cmdbuf);
                    auto computeEncoder = cmdbuf->computeCommandEncoder(); {
                        computeEncoder->bindPipeline(pipeline_);
                        computeEncoder->dispatch(32, 32, 1);
                        computeEncoder->endEncode();
                    }
                    resEnc = cmdbuf->resourceCommandEncoder();
                    resEnc->imageTransitionBarrier(blurTextures_[1], ResourceAccessType::ShaderReadWrite, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Read);
                    resEnc->imageTransitionBarrier(blurTextures_[0], ResourceAccessType::ShaderWrite, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Read);
                    resEnc->endEncode();
                    pipeline_->applyMaterial(blurMaterials_[1]);
                    pipeline_->flushMaterials(cmdbuf);
                    computeEncoder = cmdbuf->computeCommandEncoder(); {
                        computeEncoder->bindPipeline(pipeline_);
                        computeEncoder->dispatch(32, 32, 1);
                        computeEncoder->endEncode();
                    }
                }
                resEnc = cmdbuf->resourceCommandEncoder();
                Texture* screen = fbo->colorTexture(0);
                resEnc->blitImage(screen, blurTextures_[0], &dstRegion, &srcRegion, 1 );
                dstRegion.offset = { (int32_t)srcRegion.extent.width, 0, 0 };
                resEnc->blitImage(screen, texture_, &dstRegion, &srcRegion, 1 );
                resEnc->imageTransitionBarrier(screen, ResourceAccessType::Present, PipelineStages::Transfer, StageAccess::Write, PipelineStages::Top, StageAccess::Read, nullptr);
                resEnc->endEncode();
            }
            // auto renderCommandEncoder = cmdbuf->renderCommandEncoder(fbo); {
            //     renderCommandEncoder->endEncode();
            // }
        }
        cmdbuf->endEncode();
		Semaphore* imageAvailSemaphore = renderContext_->mainFramebufferAvailSemaphore();
		Semaphore* renderCompleteSemaphore = renderContext_->renderCompleteSemephore();
        renderContext_->submitCommand({{cmdbuf}, {imageAvailSemaphore}, {renderCompleteSemaphore}});
        renderContext_->onPostTick();
    }
        
    void GaussBlurTest::resize(uint32_t width, uint32_t height) {
        renderContext_->onResize(width, height);
        width_ = width;
        height_ = height;
    }

    void GaussBlurTest::release() {
    }

    const char * GaussBlurTest::title() {
        return "GaussBlur - Compute Shader";
    }
        
    uint32_t GaussBlurTest::rendererType() {
        return 0;
    }

}

ugi::GaussBlurTest theapp;

UGIApplication* GetApplication() {
    return &theapp;
}