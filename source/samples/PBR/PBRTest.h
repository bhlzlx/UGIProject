#pragma once
#include <UGIApplication.h>
#include <glm/glm.hpp>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/render_components/mesh.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_components/pipeline_material.h>
#include <ugi/texture_util.h>
#include <io/archive.h>
#include <vector>

namespace ugi {

    struct PBRVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    struct alignas(16) PBRMat {
        glm::vec3 albedo;    float metallic;
        float roughness;     float ao;  float _pad[2];
    };

    struct alignas(16) SceneUBO {
        glm::mat4 viewProj;
        glm::vec3 cameraPos;
    };

    struct alignas(16) ModelUBO {
        glm::mat4 model;
        uint32_t  materialIndex;
        float     _pad[3];
    };

    struct alignas(16) LightUBO {
        glm::vec3 lightDir;     // 12
        float     _pad1;        // 4  = 16
        glm::vec3 lightColor;   // 12
        float     ambient;      // 4  = 16  (total 32)
    };

    struct alignas(16) MaterialUBO {
        PBRMat materials[8];
    };

    void CreatePBRSphere(int nstack, int nslice,
                         std::vector<PBRVertex>& v, std::vector<uint16_t>& idx);

    class PBRApp : public UGIApplication {
    private:
        StandardRenderContext*  _ctx;
        GraphicsPipeline*       _pipeline;
        MeshBufferAllocator*    _meshAllocator;
        Material*               _sharedMtl;
        std::vector<Renderable*> _spheres;
        res_descriptor_t        _descScene;
        res_descriptor_t        _descModel[8];
        res_descriptor_t        _descLight;
        res_descriptor_t        _descMaterial;
        MaterialUBO             _mats;
        ugi::Texture*           _envTex;
        res_descriptor_t        _descEnvSampler;
        res_descriptor_t        _descEnvTex;
        float                   _width, _height;
    public:
        virtual bool initialize(void* wnd, comm::IArchive* arch);
        virtual void resize(uint32_t w, uint32_t h);
        virtual void release();
        virtual void tick();
        virtual const char* title()       { return "PBR (Metallic-Roughness)"; }
        virtual uint32_t rendererType()   { return 0; }
    };
}
