#pragma once
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
        glm::vec3   image_color;
        float       hdr;
        float       gray;
        float       alpha;
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


    struct image_render_batch {

    };

}