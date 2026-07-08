#include "ui_image_render.h"
#include "render/render_data.h"
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

// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <cstring>

namespace gui {

    bool UIImageRender::initialize_() {
        _globalMtl = _pipeline->createMaterial({"global"}, {});
        _globalMat = _globalMtl->descriptors()[0];
        //
        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        _uboptor = material->descriptors()[0];
        _samptor = material->descriptors()[1];
        _texptor = material->descriptors()[2];
        delete material;
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
        //
        _pipeline->flushMaterials(enc->commandBuffer());
        enc->draw(renderable->mesh(), renderable->mesh()->indexCount());
    }

    struct GlobalUBO {
        glm::mat4 vp;
        glm::mat4 batchWorld;
    };

    void UIImageRender::setVP(glm::mat4 const& vp) {
        _vp = vp;  // 只缓存，draw 时才分配 UBO
    }

    void UIImageRender::drawBatch(ui_render_batches_t batches, glm::mat4 const& batchWorld, ugi::RenderCommandEncoder* encoder) {
        if(batches.type != UIMeshType::Image) {
            assert(false);
            return;
        }
        // 一次性分配 global UBO（vp + batchWorld）
        {
            auto ubo = _uniformAllocator->allocate(sizeof(GlobalUBO));
            auto* g = (GlobalUBO*)ubo.ptr;
            g->vp = _vp;
            g->batchWorld = batchWorld;
            _globalMat.res.buffer.buffer = ubo.buffer;
            _globalMat.res.buffer.offset = ubo.offset;
            _globalMat.res.buffer.size = ubo.size;
            _globalMtl->updateDescriptor(_globalMat);
            _pipeline->applyMaterial(_globalMtl);
        }
        for(auto batch: batches.batches) {
            auto ubo = _uniformAllocator->allocate(batch->cachedArgs.size() * sizeof(item_args_t));
            memcpy(ubo.ptr, batch->cachedArgs.data(), ubo.size);
            batch->argsDetor.res.buffer.buffer = ubo.buffer;
            batch->argsDetor.res.buffer.offset = ubo.offset;
            batch->argsDetor.res.buffer.size = ubo.size;
            batch->renderable->material()->updateDescriptor(batch->argsDetor);
            draw(encoder, batch->renderable);
        }
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

    ugi::Renderable* UIImageRender::createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount) {
        auto mesh = ugi::Mesh::CreateMesh(
            _device, _bufferAllocator, _asyncLoadManager,
            (uint8_t const*)vd, vdsize,
            id, indexCount,
            _pipeline->desc().vertexLayout,
            _pipeline->desc().topologyMode,
            ugi::polygon_mode_t::Fill,
            [](void*, ugi::CommandBuffer* cb){}
        );
        if (!mesh) {
            return nullptr;
        }
        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        auto renderable = new ugi::Renderable(mesh, material, _pipeline, ugi::raster_state_t());
        return renderable;
    }


    gui::ui_render_batches_t UIImageRender::buildImageRenderBatch(std::vector<image_render_data_t> const& renderDatas, ugi::Texture* texture) {
        std::vector<image_vertex_t> vertices;
        std::vector<uint16_t> indices;
        std::vector<item_args_t> cachedArgs;
        ui_render_batches_t batches;

        for(auto const& renderData : renderDatas) {
            auto indicesPos = indices.size();
            auto indexBase = vertices.size();
            vertices.insert(vertices.end(), renderData.item->vertices.begin(), renderData.item->vertices.end());
            indices.insert(indices.end(), renderData.item->indices.begin(), renderData.item->indices.end());
            for(auto i = indicesPos; i < indices.size(); ++i) {
                indices[i] += indexBase;
            }
            for(auto i = indexBase; i < vertices.size(); ++i) {
                vertices[i].instIndex = (uint32_t)cachedArgs.size();
            }
            cachedArgs.push_back(*renderData.args);
            if(cachedArgs.size() >= 512) {
                auto renderable = createRenderable((const uint8_t*)vertices.data(), vertices.size() * sizeof(image_vertex_t), indices.data(), indices.size());
                if (!renderable) { cachedArgs.clear(); indices.clear(); vertices.clear(); continue; }
                auto ubo = renderable->material()->descriptors()[0];
                auto sampler = renderable->material()->descriptors()[1];
                auto tex = renderable->material()->descriptors()[2];
                tex.res.imageView = texture->defaultView().handle;
                sampler.res.samplerState = ugi::sampler_state_t();
                renderable->material()->updateDescriptor(tex);
                renderable->material()->updateDescriptor(sampler);
                ui_render_batch_t* batch = new ui_render_batch_t { renderable, ubo, sampler, tex, std::move(cachedArgs), ugi::sampler_state_t{}, texture};
                batches.batches.push_back(batch);
                cachedArgs.clear();
                indices.clear();
                vertices.clear();
            }
        }
        if(cachedArgs.size()) {
            auto renderable = createRenderable((const uint8_t*)vertices.data(), vertices.size() * sizeof(image_vertex_t), indices.data(), indices.size());
            if (!renderable) return batches;
            auto ubo = renderable->material()->descriptors()[0];
            auto sampler = renderable->material()->descriptors()[1];
            auto tex = renderable->material()->descriptors()[2];
            tex.res.imageView = texture->defaultView().handle;
            sampler.res.samplerState = ugi::sampler_state_t();
            renderable->material()->updateDescriptor(tex);
            renderable->material()->updateDescriptor(sampler);
            ui_render_batch_t* batch = new ui_render_batch_t { renderable, ubo, sampler, tex, std::move(cachedArgs), ugi::sampler_state_t{}, texture};
            batches.batches.push_back(batch);
        }
        batches.type = UIMeshType::Image;
        return batches;
    }

    void UIImageRender::destroyRenderBatch(gui::ui_render_batches_t batches) {
        for(auto batch: batches.batches) {
            batch->renderable->release();
            delete batch;
        }
        batches.batches.clear();
    }

    bool ui_render_batches_t::prepared() const {
        for(auto batch: batches) {
            if(!batch->renderable->prepared()) {
                return false;
            }
        }
        return true;
    }


};