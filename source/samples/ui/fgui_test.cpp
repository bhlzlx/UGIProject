#include "fgui_test.h"
#include "core/ui/stage.h"
#include "glm/ext/matrix_transform.hpp"
#include "io/archive.h"
#include "ugi_types.h"
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
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/descriptor_binder.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/render_components/mesh.h>
#include <ugi/asyncload/gpu_asyncload_manager.h>
#include <ugi/render_components/pipeline_material.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_context.h>
#include <ugi/texture_util.h>
#include <ugi/helper/pipeline_helper.h>
#include <cmath>

#include "gui/core/package.h"
#include "render/ui_render.h"
// #include "gui/render/render_data.h"
// #include "gui/render/ui_image_render.h"
#include "gui.h"

#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ugi {

    // 生成2D的相机view * projection矩阵
    glm::mat4 CreateVPMat(glm::vec2 screenSize, float fovy) {
        glm::vec3 camPos;
        camPos.z = (screenSize.y / 2) / tan(fovy/2.f/180.f * 3.1415926);
        camPos.x = screenSize.x / 2;
        camPos.y = screenSize.y / 2;
        glm::mat4 viewMat = glm::lookAt(camPos, glm::vec3(camPos.x, camPos.y, 0.f), glm::vec3(0, 1,0)); // view mat
        glm::mat4 projMat = glm::perspective(fovy, screenSize.x / screenSize.y, 0.1f, camPos.z * 2);
        return projMat * viewMat;
    }

    bool HelloWorld::initialize( void* _wnd, comm::IArchive* arch) {
        auto pipelineFile = arch->openIStream("/shaders/fgui_image/pipeline.bin", {comm::ReadFlag::binary});
        PipelineHelper ppl = PipelineHelper::FromIStream(pipelineFile);
        pipelineFile->close();
        auto ppldesc = ppl.desc();
        ppldesc.topologyMode = topology_mode_t::TriangleList;
        printf("initialize\n");
        ugi::device_descriptor_t descriptor; {
            descriptor.apiType = ugi::GraphicsAPIType::VULKAN;
            descriptor.deviceType = ugi::GraphicsDeviceType::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        _renderContext = StandardRenderContext::Instance();
        _renderContext->initialize(_wnd, descriptor, arch);
        ppldesc.renderState.cullMode = cull_mode_t::None;
        ppldesc.renderState.blendState.enable = false;
        auto pipeline = _renderContext->device()->createGraphicsPipeline(ppldesc);
        auto bufferAllocator = new MeshBufferAllocator();
        bufferAllocator->initialize(_renderContext->device(), 1024);

        auto device = _renderContext->device();
        _render = gui::UIImageRender::Instance();
        _render->initialize(device, pipeline, bufferAllocator, _renderContext->uniformAllocator(), _renderContext->asyncLoadManager());
        // char const* imagePaths[] = {
        //     "image/ushio.png",
        //     "image/island.png",
        // };
        // for(int i = 0; i<2; ++i) {
        //     auto imgFile = _renderContext->archive()->openIStream(imagePaths[i], {comm::ReadFlag::binary});
        //     auto buffer = malloc(imgFile->size());
        //     imgFile->read(buffer, imgFile->size());
        //     _textures[i] = CreateTexturePNG(device, (uint8_t const*)buffer, imgFile->size(), _renderContext->asyncLoadManager(), 
        //     // Texture* texture = CreateTextureKTX(device, (uint8_t const*)buffer, imgFile->size(), _renderContext->asyncLoadManager(), 
        //         [this,i,device](void* res, CommandBuffer* cb) {
        //             auto texture = (Texture*)res;
        //             // _textures[i] = (Texture*)res;
        //             texture->generateMipmap(cb);
        //             auto resEnc = cb->resourceCommandEncoder();
        //             resEnc->imageTransitionBarrier(
        //                 _textures[i], ResourceAccessType::ShaderRead, 
        //                 pipeline_stage_t::Bottom, StageAccess::Write,
        //                 pipeline_stage_t::FragmentShading, StageAccess::Read,
        //                 nullptr
        //             );
        //             image_view_param_t ivp;
        //             _imageViews[i] = _textures[i]->createImageView(device, ivp);
        //             resEnc->endEncode();
        //             texture->markAsUploaded();
        //         }
        //     );
        //     free(buffer);
        //     imgFile->close();
        // }
        // _flightIndex = 0;
        // //
        // gui::image_desc_t image_desc[2] = {
        //     {
        //         {64, 64},
        //         {{0, 0}, {.5f, .5f}}
        //     },
        //     {
        //         {32, 32},
        //         {{0.5f, 0.5f}, {1.0f, 1.0f}}
        //     }
        // };
        // auto vp = CreateVPMat(glm::vec2(633, 450), 45.f);
        // auto unit = glm::identity<glm::mat4>();
        // gui::ui_inst_data_t inst_data[2] = {
        //     {
        //         vp *glm::translate(unit, glm::vec3(0, 0,0)),
        //         glm::vec4(1.f, 1.f, 1.f, 0.5f),
        //         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        //     },
        //     {
        //         vp *glm::translate(unit, glm::vec3(64,64,0)),
        //         glm::vec4(1.f, 1.f, 1.f, 0.5f),
        //         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        //     }
        // };
        // gui::image_item_t* imageItems[2] = {
        //     // _render->createImageItem()
        //     _render->createImageItem(image_desc[0]),
        //     _render->createImageItem(image_desc[1])
        // };
        // std::vector<gui::image_render_data_t> renderDatas = {
        //     {
        //         imageItems[0],
        //         inst_data,
        //     },
        //     {
        //         imageItems[1],
        //         inst_data + 1,
        //     },
        // };
        // _imageBatches = _render->buildImageRenderBatch(renderDatas, _textures[0]);
        //
        gui::Package::archive_ = comm::CreateFSArchive(arch->rootPath() + "/test/bytes");
        auto uipack = gui::Package::AddPackage("test");
        uipack->loadAllAssets();
        //
        auto stage = gui::Stage::Instance();
        stage->initialize();
        auto root = stage->defaultRoot();
        gui::Object* uiobj = uipack->createObject("test");
        root->addChild(uiobj);
        return true;
    }

    void HelloWorld::tick() {
        _renderContext->onPreTick();
        _render->tick();
        //
        Device* device = _renderContext->device();
        IRenderPass* mainRenderPass = _renderContext->mainFramebuffer();
        auto cmdbuf = _renderContext->primaryQueue()->createCommandBuffer(device, CmdbufType::Transient);
        device->cycleInvoker().postCallable([this, device, cmdbuf](){
            _renderContext->primaryQueue()->destroyCommandBuffer(device, cmdbuf);
        });
        cmdbuf->beginEncode(); {
            //
            renderpass_clearval_t clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            gui::GuiTick();

            auto renderEnc = cmdbuf->renderCommandEncoder(mainRenderPass); {
                static ugi::raster_state_t rasterizationState;
                rasterizationState.polygonMode = polygon_mode_t::Fill;
                renderEnc->setLineWidth(1.0f);
                renderEnc->setViewport(0, 0, _width, _height, 0, 1.0f);
                renderEnc->setScissor(0, 0, _width, _height);
                // _render->setRasterization(rasterizationState);
                // _render->bind(renderEnc);
                // _render->drawBatch(_imageBatches, renderEnc);
                //
                auto vp = CreateVPMat(glm::vec2(633, 450), 45.f);
                gui::SetVPMat(vp);
                gui::DrawRenderBatches(renderEnc);
            }
            renderEnc->endEncode();
        }
        cmdbuf->endEncode();

		Semaphore* imageAvailSemaphore = _renderContext->mainFramebufferAvailSemaphore();
		Semaphore* renderCompleteSemaphore = _renderContext->renderCompleteSemephore();
        _renderContext->submitCommand({{cmdbuf}, {imageAvailSemaphore}, {renderCompleteSemaphore}});
        _renderContext->onPostTick();
    }
        
    void HelloWorld::resize(uint32_t width, uint32_t height) {
        _renderContext->onResize(width, height);
        _width = width;
        _height = height;
    }

    void HelloWorld::release() {
    }

    const char * HelloWorld::title() {
        return "ui api demo";
    }
        
    uint32_t HelloWorld::rendererType() {
        return 0;
    }

}

ugi::HelloWorld theapp;

UGIApplication* GetApplication() {
    return &theapp;
}