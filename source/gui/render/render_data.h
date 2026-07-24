#pragma once
#include <gui/core/declare.h>
#include <LightWeightCommon/utils/handle.h>
#include <core/data_types/ui_types.h>
#include <core/n_texture.h>
#include "ugi_declare.h"
#include "ugi_types.h"
#include <glm/glm.hpp>
#include <algorithm>
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
    struct image_mesh_t { 
        std::vector<image_vertex_t> vertices;
        std::vector<uint16_t>       indices;
    };

    inline uint32_t packColorRGBA(glm::vec4 const& color) {
        auto clampByte = [](float value) -> uint32_t {
            return static_cast<uint32_t>(std::clamp(int(std::round(value * 255.0f)), 0, 255));
        };
        return (clampByte(color.a) << 24)
            | (clampByte(color.b) << 16)
            | (clampByte(color.g) << 8)
            | clampByte(color.r);
    }

    inline uint32_t packColorRGBA(glm::vec3 const& color, float alpha = 1.0f) {
        return packColorRGBA(glm::vec4(color, alpha));
    }

    inline glm::vec4 unpackColorRGBA(uint32_t packed) {
        return glm::vec4(
            float(packed & 0xFFu) / 255.0f,
            float((packed >> 8) & 0xFFu) / 255.0f,
            float((packed >> 16) & 0xFFu) / 255.0f,
            float((packed >> 24) & 0xFFu) / 255.0f);
    }

    // shader uniform buffer desc
    struct item_args_t {
        glm::mat4   transfrom;
        uint32_t    colorPacked = 0xFFFFFFFF; // packed RGBA 8-bit per channel (white/opaque default)
        uint32_t    packedProps = 0; // low 8 bits: gray (0..255), next 8 bits: hdr (0..255), rest reserved
        // packed outline / shadow / effect params — placed here so image items can ignore them
        uint32_t    outlineColorPacked = 0; // packed RGBA 8-bit per channel
        uint32_t    packedParamSDF = 0; // low 8 bits: outlineWidth, next 8 bits: shadowOffsetX, next 8 bits: shadowOffsetY, top 8 bits: effectType

        void setGray(float gray) {
            packedProps = (packedProps & 0xFFFFFF00u) | (uint32_t)(std::clamp(int(std::round(gray * 255.0f)), 0, 255));
        }
        void setHDR(float hdr) {
            packedProps = (packedProps & 0xFFFF00FFu) | ((uint32_t)(std::clamp(int(std::round(hdr * 255.0f)), 0, 255)) << 8);
        }
        void setEffectType(int effect) {
            packedParamSDF = (packedParamSDF & 0x00FFFFFFu) | ((uint32_t)(effect & 0xFFu) << 24);
        }
    };

    struct image_render_data_t {
        image_mesh_t const*         item;
        item_args_t const*          args;
    };

    struct ui_render_batch_t {
        ugi::Renderable*                renderable;
        ugi::res_descriptor_t           argsDetor;
        ugi::res_descriptor_t           samplerDetor;
        ugi::res_descriptor_t           textureDetor;
        std::vector<item_args_t>        cachedArgs;   // args 缓存，build 时从 registry 拷贝，draw 时直接 memcpy
        ugi::sampler_state_t            sampler;
        Handle                          texture; // 是 raw texture，原生的，不是NTexture
    };

    enum class UIMeshType {
        None,
        Image,
        Font,
        SubBatch,
    };

    struct ui_render_batches_t {
        UIMeshType                  type;
        std::vector<ui_render_batch_t*> batches;
        entt::entity                    batchNode;
        bool prepared() const;
    };

    struct font_render_data_t {
    };

    struct opaque_item_mesh_t {
        UIMeshType      type;
        void*           item;
    };

}