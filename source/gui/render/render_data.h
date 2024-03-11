#pragma once
#include <gui/core/declare.h>
#include <LightWeightCommon/utils/handle.h>
#include "ugi_declare.h"
#include "ugi_types.h"
#include <glm/glm.hpp>
#include <vector>

namespace gui {

    // shader vertex desc
    #pragma pack(push, 1)
    struct image_vertex_t {
        glm::vec3   position;
        uint32_t    packed_color;
        glm::vec2   uv;
        uint32_t    instIndex;
    };
    #pragma pack(pop)

    // shader uniform buffer desc
    struct image_inst_data_t {
        glm::mat4   transfrom;
        glm::vec4   color;// rgb, alpha
        glm::vec4   props; // gray, hdr
    };

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

    struct image_render_data_t {
        image_item_t const*         item;
        image_inst_data_t const*    args;
        // Texture*                    tex;
        // ugi::sampler_state_t        sampler;
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


}