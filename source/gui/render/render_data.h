#pragma once
#include <cstddef>
#include <gui/core/declare.h>
#include <LightWeightCommon/utils/handle.h>
#include <core/data_types/ui_types.h>
#include <core/n_texture.h>
#include "ugi_declare.h"
#include "ugi_types.h"
#include <glm/glm.hpp>
#include <vector>

namespace gui {

    class UIImageRender;

    // shader vertex desc
    #pragma pack(push, 1)
    struct image_vertex_t {
        glm::vec3   position;
        uint32_t    packed_color;
        glm::vec2   uv;
        uint32_t    instIndex;
    };
    #pragma pack(pop)

    struct image_desc_t {
        glm::vec2   size;
        glm::vec2   uv[2];
    };

    struct image_9grid_desc_t {
        glm::vec2   size;
        glm::vec2   uv[2];
        glm::vec4   grid9;
    };

    /**
     * @brief 
     * 这里也应该是内存数据，其实就是mesh数据
     */
    struct image_item_t { 
        std::vector<image_vertex_t> vertices;
        std::vector<uint16_t>       indices;
    };

    // shader uniform buffer desc
    struct image_inst_data_t {
        glm::mat4   transfrom;
        glm::vec4   color;// rgb, alpha
        glm::vec4   props; // gray, hdr
    };


    struct image_render_data_t {
        image_item_t const*         item;
        image_inst_data_t const*    args;
    };

    struct image_render_batch_t {
        ugi::Renderable*                renderable;
        ugi::res_descriptor_t           argsDetor;
        ugi::res_descriptor_t           samplerDetor;
        ugi::res_descriptor_t           textureDetor;
        std::vector<image_inst_data_t>  args;
        ugi::sampler_state_t            sampler;
        Texture*                        texture; // 以后改成安全的
    };

    struct image_render_batches_t {
        std::vector<image_render_batch_t*> batches;
        bool prepared() const;
    };

    struct font_render_data_t {
    };

    enum class RenderDataType {
        Image,
        Font,
    };

    struct opaque_render_data_t {
        RenderDataType  type;
        void*           renderData;
    };

    struct NGraphics { // 它其实是一个控件持有的渲染数据
        opaque_render_data_t    renderData;     // mesh data
        NTexture                texture;        // texture
        image_inst_data_t       args;           // 现在是image，其实font都是通用的
    };

}