#pragma once

#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include "render_data.h"
#include "utils/singleton.h"

namespace gui {

    /// SDF Text 渲染器，结构仿照 UIImageRender
    class TextSDFRender : public comm::Singleton<TextSDFRender> {
    private:
        ugi::GraphicsPipeline*          _pipeline           = nullptr;
        ugi::MeshBufferAllocator*       _bufferAllocator    = nullptr;
        ugi::UniformAllocator*          _uniformAllocator   = nullptr;
        ugi::Device*                    _device             = nullptr;
        ugi::GPUAsyncLoadManager*       _asyncLoadManager   = nullptr;
        ugi::res_descriptor_t           _uboptor;
        ugi::res_descriptor_t           _samptor;
        ugi::res_descriptor_t           _texptor;
        ugi::Material*                  _globalMtl          = nullptr;
        ugi::res_descriptor_t           _globalMat;
        glm::mat4                       _vp;

        bool initialize_();

    public:
        TextSDFRender() = default;

        void initialize(ugi::Device* device, ugi::GraphicsPipeline* pipeline,
                        ugi::MeshBufferAllocator* msalloc, ugi::UniformAllocator* uniformAllocator,
                        ugi::GPUAsyncLoadManager* asyncLoaderManager);

        ugi::Renderable* createRenderable(uint8_t const* vd, uint32_t vdsize,
                                          uint16_t const* id, uint32_t indexCount);

        ui_render_batches_t buildRenderBatch(std::vector<image_render_data_t> const& renderDatas,
                                             ugi::Texture* texture);

        void destroyRenderBatch(ui_render_batches_t batches);

        void drawBatch(ui_render_batches_t batches, glm::mat4 const& batchWorld,
                       ugi::RenderCommandEncoder* encoder);

        void setVP(glm::mat4 const& vp);
        void setRasterization(ugi::raster_state_t rasterState);

        void bind(ugi::RenderCommandEncoder* encoder);
        void draw(ugi::RenderCommandEncoder* enc, ugi::Renderable* renderable);

        void tick();
    };

}
