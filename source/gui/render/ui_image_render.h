#pragma  once

#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <io/archive.h>
#include <gui/render/render_data.h>
#include "command_encoder/render_cmd_encoder.h"
#include "core/display_objects/display_components.h"
#include "render_components/pipeline_material.h"
#include "render_data.h"
#include "texture.h"
#include "utils/singleton.h"

namespace gui {

    class UIImageRender : public comm::Singleton<UIImageRender> {
    private:
        ugi::GraphicsPipeline*          _pipeline;
        ugi::MeshBufferAllocator*       _bufferAllocator;
        ugi::UniformAllocator*          _uniformAllocator;
        ugi::Device*                    _device;
        ugi::res_descriptor_t           _uboptor;                                       // matrices
        ugi::res_descriptor_t           _texptor;
        ugi::res_descriptor_t           _samptor;
        ugi::Material*                  _globalMtl;
        ugi::res_descriptor_t           _globalMat;
        ugi::GPUAsyncLoadManager*       _asyncLoadManager;
        //
        glm::mat4                       _vp;
    private:
        bool initialize_();
    public:
        UIImageRender()
            : _pipeline(nullptr)
            , _bufferAllocator(nullptr)
            , _uniformAllocator(nullptr)
            , _device(nullptr)
            , _asyncLoadManager(nullptr)
        {}

        void initialize(ugi::Device* device, ugi::GraphicsPipeline* pipeline, ugi::MeshBufferAllocator* msalloc, ugi::UniformAllocator* uniformAllocator, ugi::GPUAsyncLoadManager* asyncLoaderManager) {
            _pipeline = pipeline;
            _pipeline = pipeline;
            _bufferAllocator = msalloc;
            _uniformAllocator = uniformAllocator;
            _device = device;
            _asyncLoadManager = asyncLoaderManager;
            //
            initialize_();
        }
        ugi::Material* createMaterial(std::vector<std::string>const & params);

        ugi::Renderable* createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount);

        void setVP(glm::mat4 const& vp);

        void setUBO(ugi::Renderable* renderable, uint8_t* data);
        void setSampler(ugi::Renderable* renderable, ugi::sampler_state_t sampler);
        void setTexture(ugi::Renderable* renderable, ugi::image_view_t imageView);
        void setRasterization(ugi::raster_state_t rasterState);
        void bind(ugi::RenderCommandEncoder* encoder);
        void draw(ugi::RenderCommandEncoder* enc, ugi::Renderable* renderable);

        image_item_t* createImageItem(image_desc_t const& desc);
        image_item_t* createImageItem(image_9grid_desc_t const& desc);

        gui::ui_render_batches_t buildImageRenderBatch(std::vector<image_render_data_t> const& renderDatas, ugi::Texture* textur);

        void destroyRenderBatch(gui::ui_render_batches_t batches);

        void drawBatch(ui_render_batches_t batches, ugi::RenderCommandEncoder* encoder);

        void tick();
    };

}