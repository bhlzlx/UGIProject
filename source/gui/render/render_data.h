#pragma once
#include <gui/core/declare.h>
#include <LightWeightCommon/utils/handle.h>
#include <core/data_types/ui_types.h>
#include <core/n_texture.h>
#include "ugi_declare.h"
#include "ugi_types.h"
#include <glm/glm.hpp>
#include <vector>
#include <entt/entt.hpp>

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

    struct texture_block_t {
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
    struct item_args_t {
        glm::mat4   transfrom;
        glm::vec4   color;// rgb, alpha
        glm::vec4   props; // gray, hdr
        void setGray(float gray) {
            props.x = gray;
        }
        void setHDR(float hdr) {
            props.y = hdr;
        }
    };

    struct image_render_data_t {
        image_item_t const*         item;
        item_args_t const*       args;
        entt::entity                entity;     // 所属 ECS entity，draw 时查 registry 读 args
    };

    struct ui_render_batch_t {
        ugi::Renderable*                renderable;
        ugi::res_descriptor_t           argsDetor;
        ugi::res_descriptor_t           samplerDetor;
        ugi::res_descriptor_t           textureDetor;
        std::vector<entt::entity>       argEntities;  // args 所属 entity，draw 时从 registry 读
        ugi::sampler_state_t            sampler;
        Handle                          texture; // 是 raw texture，原生的，不是NTexture
    };

    enum class RenderItemType {
        None,
        Image,
        Font,
        SubBatch,
    };

    struct ui_render_batches_t {
        RenderItemType                  type;
        std::vector<ui_render_batch_t*> batches;
        entt::entity                    batchNode;
        bool prepared() const;
    };

    struct font_render_data_t {
    };

    struct opaque_item_mesh_t {
        RenderItemType  type;
        void*           item;
    };

    struct item_resource_t { // 它其实是一个控件持有的渲染数据
        opaque_item_mesh_t      meshData;       // mesh data
        item_args_t             args;           // 一个控件的参数，矩阵，透明度，灰度等
        Handle                  texture;        // texture
    };

}