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
#include "pbr_ubo.h"

namespace ugi {
    struct PBRVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };
    void CreatePBRSphere(int nstack, int nslice, std::vector<PBRVertex>& v, std::vector<uint16_t>& idx);
    class PBRApp : public UGIApplication {
    private:
        StandardRenderContext*  _ctx;
        GraphicsPipeline*       _pipeline;
        MeshBufferAllocator*    _meshAllocator;
        std::vector<Renderable*> _spheres;
        res_descriptor_t        _descScene;
        res_descriptor_t        _descModel[8];
        res_descriptor_t        _descLight;
        res_descriptor_t        _descMaterial;
        MaterialUBO             _mats;
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
