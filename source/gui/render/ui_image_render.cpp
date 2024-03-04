#include "ui_image_render.h"
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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace gui {

    bool UIImageRender::initialize() {
        auto material = _pipeline->createMaterial({"Argument1", "triSampler", "triTexture"}, {});
        _uboptor = material->descriptors()[0];
        _samptor = material->descriptors()[1];
        _texptor = material->descriptors()[2];
        _bufferAllocator = new ugi::MeshBufferAllocator();
        auto rst = _bufferAllocator->initialize(_device, 4096);
        return rst;
    }

    void UIImageRender::setRasterization(ugi::raster_state_t rasterState) {
        _pipeline->setRasterizationState(rasterState);
    }

    void UIImageRender::tick() {
        _pipeline->resetMaterials();
    }

    void UIImageRender::bind(ugi::RenderCommandEncoder* encoder) {
        encoder->bindPipeline(_pipeline);
    }

    void UIImageRender::draw(ugi::RenderCommandEncoder* enc, ugi::Renderable* renderable) {
        _pipeline->applyMaterial(renderable->material());
        _pipeline->flushMaterials(enc->commandBuffer());
        enc->draw(renderable->mesh(), renderable->mesh()->indexCount());
    }

    void UIImageRender::setUBO(ugi::Renderable* renderable, uint8_t* data) {
        auto mtl = renderable->material();
        _uniformAllocator->allocateForDescriptor(_uboptor, data);
        mtl->updateDescriptor(_uboptor);
    }

    void UIImageRender::setSampler(ugi::Renderable* renderable, ugi::sampler_state_t sampler) {
        auto mtl = renderable->material();
        _samptor.res.samplerState = sampler;
        mtl->updateDescriptor(_samptor);
    }

    void UIImageRender::setTexture(ugi::Renderable* renderable, ugi::image_view_t imageView) {
        auto mtl = renderable->material();
        _texptor.res.imageView = imageView.handle;
        mtl->updateDescriptor(_texptor);
    }

    ugi::Renderable* UIImageRender::createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount, ugi::GPUAsyncLoadManager* asyncLoadManager) {
        auto mesh = ugi::Mesh::CreateMesh(
            _device, _bufferAllocator, asyncLoadManager,
            (uint8_t const*)vd, vdsize,
            id, indexCount,
            _pipeline->desc().vertexLayout,
            _pipeline->desc().topologyMode,
            ugi::polygon_mode_t::Fill,
            [](void*, ugi::CommandBuffer* cb){}
        );
        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        auto renderable = new ugi::Renderable(mesh, material, _pipeline, ugi::raster_state_t());
        return renderable;
    }


};