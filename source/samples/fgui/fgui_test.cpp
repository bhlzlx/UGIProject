#include "fgui_test.h"
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

#include <glm/glm.hpp>
#include <glm/ext.hpp>
// #include <glm/glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ugi {

    glm::mat4 CreateVPMat(glm::vec2 screenSize, float fov) {
        glm::vec3 camPos;
        camPos.z = (screenSize.x / 2) / tan(fov/2.f/180.f);
        camPos.x = screenSize.x / 2;
        camPos.y = -screenSize.y / 2;
        glm::mat4 viewMat = glm::lookAt(camPos, glm::vec3(camPos.x, camPos.y, 0.f), glm::vec3(0, 1,0)); // view mat
        float fovy = atan((screenSize.y / 2) / camPos.z) * 2 * 180.f;
        glm::mat4 projMat = glm::perspective(fovy, screenSize.y / screenSize.x, camPos.z / 2, camPos.z * 2);
        return viewMat * projMat;
    }

    bool Render::initialize() {
        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        _uboptor = material->descriptors()[0];
        _samptor = material->descriptors()[1];
        _texptor = material->descriptors()[2];
        _bufferAllocator = new MeshBufferAllocator();
        auto rst = _bufferAllocator->initialize(_device, 4096);
        return rst;
    }

    void Render::setRasterization(raster_state_t rasterState) {
        _pipeline->setRasterizationState(rasterState);
    }

    void Render::tick() {
        _pipeline->resetMaterials();
    }

    void Render::bind(RenderCommandEncoder* encoder) {
        encoder->bindPipeline(_pipeline);
    }

    void Render::draw(RenderCommandEncoder* enc, Renderable* renderable) {
        _pipeline->applyMaterial(renderable->material());
        _pipeline->flushMaterials(enc->commandBuffer());
        enc->draw(renderable->mesh(), renderable->mesh()->indexCount());
    }

    void Render::setUBO(Renderable* renderable, uint8_t* data) {
        auto mtl = renderable->material();
        _uniformAllocator->allocateForDescriptor(_uboptor, data);
        mtl->updateDescriptor(_uboptor);
    }

    void Render::setSampler(Renderable* renderable, sampler_state_t sampler) {
        auto mtl = renderable->material();
        _samptor.res.samplerState = sampler;
        mtl->updateDescriptor(_samptor);
    }

    void Render::setTexture(Renderable* renderable, image_view_t imageView) {
        auto mtl = renderable->material();
        _texptor.res.imageView = imageView.handle;
        mtl->updateDescriptor(_texptor);
    }

    Renderable* Render::createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount, GPUAsyncLoadManager* asyncLoadManager) {
        auto mesh = Mesh::CreateMesh(
            _device, _bufferAllocator, asyncLoadManager,
            (uint8_t const*)vd, vdsize,
            id, indexCount,
            _pipeline->desc().vertexLayout,
            _pipeline->desc().topologyMode,
            ugi::polygon_mode_t::Fill,
            [](void*, CommandBuffer* cb){}
        );
        auto material = _pipeline->createMaterial({"Argument1", "triSampler", "triTexture"}, {});
        auto renderable = new Renderable(mesh, material, _pipeline, raster_state_t());
        return renderable;
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
        _renderContext = new StandardRenderContext();
        _renderContext->initialize(_wnd, descriptor, arch);
        ppldesc.renderState.cullMode = cull_mode_t::None;
        ppldesc.renderState.blendState.enable = false;
        auto pipeline = _renderContext->device()->createGraphicsPipeline(ppldesc);
        auto bufferAllocator = new MeshBufferAllocator();
        bufferAllocator->initialize(_renderContext->device(), 1024);

        auto device = _renderContext->device();
        _render = new Render(device, pipeline, bufferAllocator, _renderContext->uniformAllocator());
        _render->initialize();

        float centerH = 1.0f / std::sqrt(3.0f);
        float edge = 0.5f;
        float vertexData[] = {
            -edge, -centerH * edge, 0, 0,
            0, edge * (sqrt(3.0f) - centerH), 0.5, 1.0,
            edge, -centerH * edge, 1.0, 0
        };
        uint16_t indexData[] = {
            0, 1, 2
        };
        _renderable = _render->createRenderable((uint8_t const*)vertexData, sizeof(vertexData), indexData, 3, _renderContext->asyncLoadManager());
        // tex_desc_t texDesc;
        // texDesc.format = UGIFormat::RGBA8888_UNORM;
        // texDesc.depth = 1;
        // texDesc.width = 16;
        // texDesc.height = 16;
        // texDesc.type = TextureType::Texture2D;
        // texDesc.mipmapLevel = 1;
        // texDesc.layerCount = 1;
        // _texture = device->createTexture(texDesc, ResourceAccessType::ShaderRead );
        // uint32_t texData[] = {
        //     0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
        //     0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
        //     0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
        //     0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
        //     0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
        //     0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
        //     0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
        //     0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
        // };
        // std::vector<image_region_t> regions;
        // std::vector<uint64_t> offsets = {0, 0, 0, 0};
        // image_region_t region;
        // region.offset = {};
        // region.mipLevel = 0;
        // region.arrayIndex = 0;
        // region.arrayCount = 1;
        // region.extent.height = region.extent.width = 8;
        // region.extent.depth = 1;
        // regions.push_back(region);
        // region.offset.x = 8;
        // regions.push_back(region);
        // region.offset.x = 0; region.offset.y = 8;
        // regions.push_back(region);
        // region.offset.x = 8; region.offset.y = 8;
        // regions.push_back(region);
        // _texture->updateRegions(
        //     device, 
        //     regions.data(), regions.size(), 
        //     (uint8_t const*)texData, sizeof(texData), offsets.data(), 
        //     _renderContext->asyncLoadManager(),
        //     [this](CommandBuffer* cb) {
        //         auto resEnc = cb->resourceCommandEncoder();
        //         resEnc->imageTransitionBarrier(
        //             _texture, ResourceAccessType::ShaderRead, 
        //             pipeline_stage_t::Bottom, StageAccess::Write,
        //             pipeline_stage_t::FragmentShading, StageAccess::Read,
        //             nullptr
        //         );
        //         resEnc->endEncode();
        //         _texture->markAsUploaded();
        //     }
        // );
        char const* imagePaths[] = {
            "image/ushio.png",
            "image/island.png",
        };
        for(int i = 0; i<2; ++i) {
            auto imgFile = _renderContext->archive()->openIStream(imagePaths[i], {comm::ReadFlag::binary});
            auto buffer = malloc(imgFile->size());
            imgFile->read(buffer, imgFile->size());
            Texture* texture = CreateTexturePNG(device, (uint8_t const*)buffer, imgFile->size(), _renderContext->asyncLoadManager(), 
            // Texture* texture = CreateTextureKTX(device, (uint8_t const*)buffer, imgFile->size(), _renderContext->asyncLoadManager(), 
                [this,i,device](void* res, CommandBuffer* cb) {
                    _textures[i] = (Texture*)res;
                    _textures[i]->generateMipmap(cb);
                    auto resEnc = cb->resourceCommandEncoder();
                    resEnc->imageTransitionBarrier(
                        _textures[i], ResourceAccessType::ShaderRead, 
                        pipeline_stage_t::Bottom, StageAccess::Write,
                        pipeline_stage_t::FragmentShading, StageAccess::Read,
                        nullptr
                    );
                    image_view_param_t ivp;
                    _imageViews[i] = _textures[i]->createImageView(device, ivp);
                    resEnc->endEncode();
                    _textures[i]->markAsUploaded();
                }
            );
            free(buffer);
            imgFile->close();
        }
        _render->setSampler(_renderable, _samplerState);
        // _render->setTexture(_renderable, _imageView);
        _flightIndex = 0;
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

            auto renderEnc = cmdbuf->renderCommandEncoder(mainRenderPass); {
                if(_textures[0] && _textures[1] && _renderable->mesh()->prepared()) {
                    static uint64_t angle = 0;
                    float sinVal = sin( ((float)angle)/180.0f * 3.1415926f );
                    float cosVal = cos( ((float)angle)/180.0f * 3.1415926f );
                    float col2[2][4] = {
                        { cosVal, -sinVal, 0, 0 },
                        { sinVal,  cosVal, 0, 0 }
                    };
                    static ugi::raster_state_t rasterizationState;
                    if( angle % 180 == 0 ) {
                        rasterizationState.polygonMode = polygon_mode_t::Fill;
                    } else if( angle % 90 == 0) {
                        rasterizationState.polygonMode = polygon_mode_t::Line;
                    }
                    if( angle % 60 == 0 ) {
                        _render->setTexture(_renderable, _imageViews[0]);
                    } else if( angle % 30 == 0) {
                        _render->setTexture(_renderable, _imageViews[1]);
                    }
                    ++angle;
                    _render->setUBO(_renderable, (uint8_t*)col2);
                    renderEnc->setLineWidth(1.0f);
                    renderEnc->setViewport(0, 0, _width, _height, 0, 1.0f);
                    renderEnc->setScissor(0, 0, _width, _height);
                    _render->setRasterization(rasterizationState);
                    _render->bind(renderEnc);
                    _render->draw(renderEnc, _renderable);
                }
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
        return "HelloWorld";
    }
        
    uint32_t HelloWorld::rendererType() {
        return 0;
    }

}

ugi::HelloWorld theapp;

UGIApplication* GetApplication() {
    return &theapp;
}