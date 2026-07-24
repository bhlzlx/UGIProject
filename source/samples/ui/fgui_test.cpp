#include "fgui_test.h"
#include "core/ui/stage.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
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
#include "render/text_sdf_render.h"
#include "gui/core/font_manager.h"
#include "gui/core/fairy_gui_context.h"
#include "gui/core/ui/g_text_field.h"
// #include "gui/render/render_data.h"
// #include "gui/render/ui_image_render.h"
#include "gui.h"
#include "gui/debug/debug_server.h"

#include <glm/ext.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ugi {

    // 生成2D的相机view * projection矩阵
    bool FGUIDemo::initialize( void* _wnd, comm::IArchive* arch) {
        _arch = arch;
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

        auto bufferAllocator = new MeshBufferAllocator();
        bufferAllocator->initialize(_renderContext->device(), 1024);

        auto device = _renderContext->device();
        _render = gui::UIImageRender::Instance();
        _render->initialize(device, arch, bufferAllocator, _renderContext->uniformAllocator(), _renderContext->asyncLoadManager());

        // Text SDF pipeline
        {
            auto textPipelineFile = arch->openIStream("/shaders/fgui_text/pipeline.bin", {comm::ReadFlag::binary});
            PipelineHelper textPpl = PipelineHelper::FromIStream(textPipelineFile);
            textPipelineFile->close();
            auto textDesc = textPpl.desc();
            textDesc.topologyMode = topology_mode_t::TriangleList;
            textDesc.renderState.cullMode = cull_mode_t::None;
            textDesc.renderState.blendState.enable = true;
            textDesc.renderState.blendState.srcAlphaFactor = blend_factor_t::SourceAlpha;
            textDesc.renderState.blendState.dstAlphaFactor = blend_factor_t::DestinationAlpha;
            auto textPipeline = device->createGraphicsPipeline(textDesc);
            auto textBufferAllocator = new MeshBufferAllocator();
            textBufferAllocator->initialize(device, 1024);
            gui::TextSDFRender::Instance()->initialize(device, textPipeline,
                textBufferAllocator, _renderContext->uniformAllocator(), _renderContext->asyncLoadManager());
        }

        // FontManager 初始化 + 加载字体
        {
            gui::FontManager::Config fontCfg;
            fontCfg.sdfSourceSize  = 64;
            fontCfg.extraBorder    = 8;
            fontCfg.searchDistance = 8;
            fontCfg.tileSize       = 64;
            fontCfg.atlasSize      = 1024;
            fontCfg.maxLayers      = 4;
            auto* fm = gui::FontManager::Instance();
            fm->initialize(device, _renderContext->asyncLoadManager(), fontCfg);

            auto fontArch = comm::CreateFSArchive(_arch->rootPath());
            auto fontStream = fontArch->openIStream("hwzhsong.ttf", {comm::ReadFlag::binary});
            if (fontStream) {
                std::vector<uint8_t> ttf(fontStream->size());
                fontStream->read(ttf.data(), fontStream->size());
                fontStream->close();
                int fid = fm->addFont(ttf.data(), ttf.size());
                if (fid >= 0) _defaultFontID = fid;
            }
        }

        gui::Package::InitPackageModule(_arch);

        // Start debug TCP server for widget tree inspection
        gui::DebugServer::Instance().start();
        //
        return true;
    }

    glm::mat4 vp;

    void FGUIDemo::onMouseEvent(eMouseButton bt, eMouseEvent event, int x, int y) {
        switch (event) {
        case MouseDown: gui::FairyGUIContext::Instance()->onMouseDown(x, y); break;
        case MouseUp:   gui::FairyGUIContext::Instance()->onMouseUp(x, y);   break;
        case MouseMove: gui::FairyGUIContext::Instance()->onMouseMove(x, y); break;
        default: break;
        }
    }

    void FGUIDemo::tick() {
        if (!_renderContext->onPreTick()) return;
        _render->tick();
        gui::TextSDFRender::Instance()->tick();
        gui::FontManager::Instance()->tickUpload(_renderContext->device());
        //
        Device* device = _renderContext->device();
        IRenderPass* mainRenderPass = _renderContext->mainFramebuffer();
        auto cmdbuf = _renderContext->primaryQueue()->createCommandBuffer(device, CmdbufType::Resetable);
        device->cycleInvoker().postCallable([this, device, cmdbuf](){
            _renderContext->primaryQueue()->destroyCommandBuffer(device, cmdbuf);
        });
        // game logic
        {
            static float angle = 0;
            angle += 0.1f;
            auto root = gui::Stage::Instance()->defaultRoot();
            auto test = (gui::Component*)root->getChildAt(0);
            // auto child = test->getChild("rotation");
            auto child = test->getChildAt(0);
            if (child) {
                child->setRotation(angle);
            }
        }

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
                gui::SetVPMat(gui::FairyGUIContext::Instance()->vp());
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
        
    void FGUIDemo::resize(uint32_t width, uint32_t height) {
        _renderContext->onResize(width, height);
        _width = width;
        _height = height;

        static bool uiInitialized = false;
        if (!uiInitialized) {
            uiInitialized = true;
            auto& scaler = *gui::UIContentScaler::Instance();
            scaler.scaleMode = gui::UIContentScaler::ScaleMode::ScaleWithScreenSize;
            scaler.screenMatchMode = gui::UIContentScaler::ScreenMatchMode::MatchWidthOrHeight;
            scaler.designResolutionX = 800;
            scaler.designResolutionY = 600;

            gui::Package::archive_ = comm::CreateFSArchive(_arch->rootPath() + "/test/bytes");
            auto uipack = gui::Package::AddPackage("test");
            auto* stage = gui::Stage::Instance();
            stage->initialize(scaler.designResolutionX, scaler.designResolutionY);

            auto* root = stage->defaultRoot();
            auto* uiobj = uipack->createObject("test");
            uipack->loadAllAssets();
            root->addChild(uiobj);
            uiobj->relations().add(root, gui::RelationType::Width);
            uiobj->relations().add(root, gui::RelationType::Height);
        }

        gui::FairyGUIContext::Instance()->setScreenSize((float)width, (float)height);
    }

    void FGUIDemo::release() {
        gui::DebugServer::Instance().stop();
    }

    const char * FGUIDemo::title() {
        return "ui api demo";
    }
        
    uint32_t FGUIDemo::rendererType() {
        return 0;
    }

}

ugi::FGUIDemo theapp;

UGIApplication* GetApplication() {
    return &theapp;
}