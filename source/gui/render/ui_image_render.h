#pragma  once

#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <io/archive.h>
#include "core/n_texture.h"
#include "render_data.h"

namespace gui {

    class UIImageRender {
    private:
        ugi::GraphicsPipeline*          _pipeline;
        ugi::MeshBufferAllocator*       _bufferAllocator;
        ugi::UniformAllocator*          _uniformAllocator;
        ugi::Device*                    _device;
        ugi::res_descriptor_t           _uboptor;                                       // matrices
        ugi::res_descriptor_t           _texptor;
        ugi::res_descriptor_t           _samptor;
    public:
        UIImageRender(ugi::Device* device, ugi::GraphicsPipeline* pipeline, ugi::MeshBufferAllocator* msalloc, UniformAllocator* uniformAllocator)
            : _pipeline(pipeline)
            , _bufferAllocator(msalloc)
            , _uniformAllocator(uniformAllocator)
            , _device(device)
        {
        }

        ugi::Material* createMaterial(std::vector<std::string>const & params);

        ugi::Renderable* createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount, ugi::GPUAsyncLoadManager* asyncLoadManager);

        void setUBO(ugi::Renderable* renderable, uint8_t* data);
        void setSampler(ugi::Renderable* renderable, ugi::sampler_state_t sampler);
        void setTexture(ugi::Renderable* renderable, ugi::image_view_t imageView);
        void setRasterization(ugi::raster_state_t rasterState);
        void bind(ugi::RenderCommandEncoder* encoder);
        void draw(ugi::RenderCommandEncoder* enc, ugi::Renderable* renderable);

        static image_item_t* createImageItem(image_desc_t const& desc);
        static image_item_t* createImageItem(image_9grid_desc_t const& desc);

        static image_render_batch* createImageRenderBatch(std::vector<image_item_t*> const& items, std::vector<image_inst_data_t>const& args, Handle texture);


        bool initialize();

        void tick();
    };


}