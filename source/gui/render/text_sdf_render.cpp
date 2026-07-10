#include "text_sdf_render.h"
#include "render_data.h"
#include "ugi_types.h"
#include <ugi/device.h>
#include <ugi/command_buffer.h>
#include <ugi/pipeline.h>
#include <ugi/texture.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/render_components/mesh.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_components/pipeline_material.h>
#include <cstring>

namespace gui {

    struct GlobalUBO_SDF {
        glm::mat4 vp;
        glm::mat4 batchWorld;
    };

    bool TextSDFRender::initialize_() {
        _globalMtl = _pipeline->createMaterial({"global"}, {});
        _globalMat = _globalMtl->descriptors()[0];

        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        _uboptor = material->descriptors()[0];
        _samptor = material->descriptors()[1];
        _texptor = material->descriptors()[2];
        delete material;

        _bufferAllocator = new ugi::MeshBufferAllocator();
        auto rst = _bufferAllocator->initialize(_device, 4096);
        return rst;
    }

    void TextSDFRender::initialize(ugi::Device* device, ugi::GraphicsPipeline* pipeline,
                                   ugi::MeshBufferAllocator* msalloc, ugi::UniformAllocator* uniformAllocator,
                                   ugi::GPUAsyncLoadManager* asyncLoaderManager) {
        _pipeline = pipeline;
        _bufferAllocator = msalloc;
        _uniformAllocator = uniformAllocator;
        _device = device;
        _asyncLoadManager = asyncLoaderManager;
        initialize_();
    }

    void TextSDFRender::setVP(glm::mat4 const& vp) {
        _vp = vp;
    }

    void TextSDFRender::setRasterization(ugi::raster_state_t rasterState) {
        _pipeline->setRasterizationState(rasterState);
    }

    void TextSDFRender::tick() {
        _pipeline->resetMaterials();
    }

    void TextSDFRender::bind(ugi::RenderCommandEncoder* encoder) {
        encoder->bindPipeline(_pipeline);
    }

    void TextSDFRender::draw(ugi::RenderCommandEncoder* enc, ugi::Renderable* renderable) {
        _pipeline->applyMaterial(renderable->material());
        _pipeline->flushMaterials(enc->commandBuffer());
        enc->draw(renderable->mesh(), renderable->mesh()->indexCount());
    }

    ugi::Renderable* TextSDFRender::createRenderable(uint8_t const* vd, uint32_t vdsize,
                                                      uint16_t const* id, uint32_t indexCount) {
        auto mesh = ugi::Mesh::CreateMesh(
            _device, _bufferAllocator, _asyncLoadManager,
            vd, vdsize, id, indexCount,
            _pipeline->desc().vertexLayout,
            _pipeline->desc().topologyMode,
            ugi::polygon_mode_t::Fill,
            [](void*, ugi::CommandBuffer* cb){}
        );
        if (!mesh) return nullptr;
        auto material = _pipeline->createMaterial({"args", "image_sampler", "image_tex"}, {});
        auto renderable = new ugi::Renderable(mesh, material, _pipeline, ugi::raster_state_t());
        return renderable;
    }

    ui_render_batches_t TextSDFRender::buildRenderBatch(std::vector<image_render_data_t> const& renderDatas,
                                                         ugi::Texture* texture) {
        std::vector<image_vertex_t> vertices;
        std::vector<uint16_t> indices;
        std::vector<item_args_t> cachedArgs;
        ui_render_batches_t batches;

        for (auto const& renderData : renderDatas) {
            auto indicesPos = indices.size();
            auto indexBase = vertices.size();
            vertices.insert(vertices.end(), renderData.item->vertices.begin(), renderData.item->vertices.end());
            indices.insert(indices.end(), renderData.item->indices.begin(), renderData.item->indices.end());
            for (auto i = indicesPos; i < indices.size(); ++i)
                indices[i] += indexBase;
            for (auto i = indexBase; i < vertices.size(); ++i)
                vertices[i].instIndex = (uint32_t)cachedArgs.size();
            cachedArgs.push_back(*renderData.args);
            if (cachedArgs.size() >= 512) {
                auto renderable = createRenderable((const uint8_t*)vertices.data(),
                    vertices.size() * sizeof(image_vertex_t), indices.data(), indices.size());
                if (!renderable) { cachedArgs.clear(); indices.clear(); vertices.clear(); continue; }
                auto ubo = renderable->material()->descriptors()[0];
                auto sampler = renderable->material()->descriptors()[1];
                auto tex = renderable->material()->descriptors()[2];
                tex.res.imageView = texture->defaultView().handle;
                sampler.res.samplerState = ugi::sampler_state_t{ .min = ugi::TextureFilter::Linear, .mag = ugi::TextureFilter::Linear };
                renderable->material()->updateDescriptor(tex);
                renderable->material()->updateDescriptor(sampler);
                ui_render_batch_t* batch = new ui_render_batch_t {
                    renderable, ubo, sampler, tex, std::move(cachedArgs),
                    ugi::sampler_state_t{ .min = ugi::TextureFilter::Linear, .mag = ugi::TextureFilter::Linear }, texture
                };
                batches.batches.push_back(batch);
                cachedArgs.clear(); indices.clear(); vertices.clear();
            }
        }
        if (cachedArgs.size()) {
            auto renderable = createRenderable((const uint8_t*)vertices.data(),
                vertices.size() * sizeof(image_vertex_t), indices.data(), indices.size());
            if (!renderable) return batches;
            auto ubo = renderable->material()->descriptors()[0];
            auto sampler = renderable->material()->descriptors()[1];
            auto tex = renderable->material()->descriptors()[2];
            tex.res.imageView = texture->defaultView().handle;
            sampler.res.samplerState = ugi::sampler_state_t{ .min = ugi::TextureFilter::Linear, .mag = ugi::TextureFilter::Linear };
            renderable->material()->updateDescriptor(tex);
            renderable->material()->updateDescriptor(sampler);
            ui_render_batch_t* batch = new ui_render_batch_t {
                renderable, ubo, sampler, tex, std::move(cachedArgs),
                ugi::sampler_state_t{ .min = ugi::TextureFilter::Linear, .mag = ugi::TextureFilter::Linear }, texture
            };
            batches.batches.push_back(batch);
        }
        batches.type = UIMeshType::Font;
        return batches;
    }

    void TextSDFRender::destroyRenderBatch(ui_render_batches_t batches) {
        for (auto batch : batches.batches) {
            batch->renderable->release();
            delete batch;
        }
        batches.batches.clear();
    }

    void TextSDFRender::drawBatch(ui_render_batches_t batches, glm::mat4 const& batchWorld,
                                   ugi::RenderCommandEncoder* encoder) {
        if (batches.type != UIMeshType::Font) return;

        // Global UBO: vp + batchWorld
        {
            auto ubo = _uniformAllocator->allocate(sizeof(GlobalUBO_SDF));
            auto* g = (GlobalUBO_SDF*)ubo.ptr;
            g->vp = _vp;
            g->batchWorld = batchWorld;
            _globalMat.res.buffer.buffer = ubo.buffer;
            _globalMat.res.buffer.offset = ubo.offset;
            _globalMat.res.buffer.size = ubo.size;
            _globalMtl->updateDescriptor(_globalMat);
            _pipeline->applyMaterial(_globalMtl);
        }

        for (auto batch : batches.batches) {
            auto ubo = _uniformAllocator->allocate(batch->cachedArgs.size() * sizeof(item_args_t));
            memcpy(ubo.ptr, batch->cachedArgs.data(), ubo.size);
            batch->argsDetor.res.buffer.buffer = ubo.buffer;
            batch->argsDetor.res.buffer.offset = ubo.offset;
            batch->argsDetor.res.buffer.size = ubo.size;
            batch->renderable->material()->updateDescriptor(batch->argsDetor);
            draw(encoder, batch->renderable);
        }
    }

}
