#include "PBRTest.h"
#include <ugi/device.h>
#include <ugi/command_buffer.h>
#include <ugi/pipeline.h>
#include <ugi/render_pass.h>
#include <ugi/texture.h>
#include <ugi/descriptor_binder.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/mesh_buffer_allocator.h>
#include <ugi/render_components/mesh.h>
#include <ugi/asyncload/gpu_asyncload_manager.h>
#include <ugi/render_components/pipeline_material.h>
#include <ugi/render_components/renderable.h>
#include <ugi/render_context.h>
#include <ugi/command_queue.h>
#include <ugi/helper/pipeline_helper.h>
#include <io/archive.h>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ugi {

    void CreatePBRSphere(int nstack, int nslice,
                         std::vector<PBRVertex>& verts, std::vector<uint16_t>& idx)
    {
        int mid = (nstack - 1) / 2, msc = mid + nslice;
        for (int stk = 0; stk < nstack; ++stk) {
            int sc = (nstack & 1) ? msc - abs(mid - stk)
                                  : msc - abs((stk > mid) ? mid - stk + 1 : mid - stk);
            float ya = 3.141592654f / (nstack - 1) * stk;
            float y = cos(ya), xzr = sin(ya), da = 6.283185307f / (sc - 1);
            for (int slc = 0; slc < sc; ++slc) {
                PBRVertex v;
                v.position = glm::vec3(cos(da*slc)*xzr, y, sin(da*slc)*xzr);
                v.normal   = glm::normalize(v.position);
                v.uv       = glm::vec2(1.0f - 1.0f/(sc-1)*slc, 1.0f - (y+1.0f)*0.5f);
                verts.push_back(v);
            }
        }
        size_t cnt = verts.size(), ctr = 0;
        if (nstack & 1) {
            for (int stk = 0; stk < mid; ++stk) {
                int sc = msc - abs(mid - stk);
                for (int ct = 1; ct <= sc; ++ct) {
                    uint16_t t[6] = { (uint16_t)ctr, (uint16_t)(ctr+sc), (uint16_t)(ctr+sc+1),
                        (uint16_t)(cnt-ctr-1), (uint16_t)(cnt-ctr-sc-1), (uint16_t)(cnt-ctr-sc-2) };
                    for (auto x : t) idx.push_back(x);
                    if (stk != 0 && ct != sc) {
                        uint16_t t2[6] = { (uint16_t)ctr, (uint16_t)(ctr+sc+1), (uint16_t)(ctr+1),
                            (uint16_t)(cnt-ctr-1), (uint16_t)(cnt-ctr-sc-2), (uint16_t)(cnt-ctr-2) };
                        for (auto x : t2) idx.push_back(x);
                    }
                    ++ctr;
                }
            }
        } else {
            for (int stk = 0; stk < mid; ++stk) {
                int sc = (stk > mid) ? msc - abs(mid - stk + 1) : msc - abs(mid - stk);
                for (int ct = 1; ct <= sc; ++ct) {
                    uint16_t t[6] = { (uint16_t)ctr, (uint16_t)(ctr+sc), (uint16_t)(ctr+sc+1),
                        (uint16_t)(cnt-ctr-1), (uint16_t)(cnt-ctr-sc-1), (uint16_t)(cnt-ctr-sc-2) };
                    for (auto x : t) idx.push_back(x);
                    if (stk != 0 && ct != sc) {
                        uint16_t t2[6] = { (uint16_t)ctr, (uint16_t)(ctr+sc+1), (uint16_t)(ctr+1),
                            (uint16_t)(cnt-ctr-1), (uint16_t)(cnt-ctr-sc-2), (uint16_t)(cnt-ctr-2) };
                        for (auto x : t2) idx.push_back(x);
                    }
                    ++ctr;
                }
            }
            for (int ct = 0; ct < msc - 1; ++ct) {
                uint16_t t[6] = { (uint16_t)ctr, (uint16_t)(ctr+msc), (uint16_t)(ctr+1),
                    (uint16_t)(cnt-ctr-1), (uint16_t)(cnt-ctr-msc-1), (uint16_t)(cnt-ctr-2) };
                for (auto x : t) idx.push_back(x);
                ++ctr;
            }
        }
    }

    bool PBRApp::initialize(void* wnd, comm::IArchive* arch) {

        auto pf = arch->openIStream("/shaders/pbr/pipeline.bin", { comm::ReadFlag::binary });
        if (!pf) { printf("pipeline.bin not found!\n"); return false; }
        PipelineHelper ppl = PipelineHelper::FromIStream(pf);
        pf->close();
        auto pdesc = ppl.desc();
        pdesc.topologyMode = topology_mode_t::TriangleList;
        pdesc.renderState.cullMode = cull_mode_t::None;
        pdesc.renderState.blendState.enable = false;

        device_descriptor_t dd;
        dd.apiType = GraphicsAPIType::VULKAN;
        dd.deviceType = GraphicsDeviceType::DISCRETE;
        dd.debugLayer = 1;
        dd.graphicsQueueCount = 1;
        dd.transferQueueCount = 1;
        dd.wnd = wnd;
        _ctx = StandardRenderContext::Instance();
        _ctx->initialize(wnd, dd, arch);
        auto* dev = _ctx->device();

        _pipeline = dev->createGraphicsPipeline(pdesc);

        _meshAllocator = new MeshBufferAllocator();
        _meshAllocator->initialize(dev, 1024 * 1024);

        // 几何
        std::vector<PBRVertex> verts; std::vector<uint16_t> idx;
        CreatePBRSphere(32, 32, verts, idx);

        // 材质数据 (静态)
        MaterialUBO mats;
        // 两排: 上排 metallic 0→1 (roughness=0.4), 下排 roughness 0.1→0.9 (metallic=1.0)
        for (int i = 0; i < 4; ++i) {
            float m = i / 3.0f;                    // metallic: 0, 0.33, 0.67, 1.0
            float  r = 0.4f;
            glm::vec3 c = glm::mix(glm::vec3(0.9,0.2,0.2), glm::vec3(0.9,0.7,0.3), m);  // red → gold
            mats.materials[i] = { c, m, r, 1.0f, {0,0} };
        }
        for (int i = 0; i < 4; ++i) {
            float m = 1.0f;
            float r = 0.1f + i / 3.0f * 0.8f;     // roughness: 0.1, 0.37, 0.63, 0.9
            glm::vec3 c = glm::mix(glm::vec3(0.9,0.7,0.3), glm::vec3(0.5,0.5,0.5), i/3.0f);  // gold → grey
            mats.materials[i+4] = { c, m, r, 1.0f, {0,0} };
        }
        _mats = mats;

        // 8 个球体, 每个独立 Material + Renderable
        for (int i = 0; i < 8; ++i) {
            auto mesh = Mesh::CreateMesh(
                dev, _meshAllocator, _ctx->asyncLoadManager(),
                (uint8_t const*)verts.data(), verts.size() * sizeof(PBRVertex),
                idx.data(), idx.size(),
                pdesc.vertexLayout, pdesc.topologyMode, polygon_mode_t::Fill,
                [](void*, CommandBuffer*){}
            );

            // 4 个 descriptor: SceneData(0), ModelData(1), LightData(2), MaterialData(3)
            auto* mtl = _pipeline->createMaterial(
                {"SceneData", "ModelData", "LightData", "MaterialData", "envSampler", "envTex"}, {}
            );

            _spheres.push_back(new Renderable(mesh, mtl, _pipeline, raster_state_t()));
        }

        // 模板 descriptor — 每帧填数据
        _descScene    = _spheres[0]->material()->descriptors()[0];
        _descLight    = _spheres[0]->material()->descriptors()[2];
        _descMaterial = _spheres[0]->material()->descriptors()[3];
        _descEnvSampler = _spheres[0]->material()->descriptors()[4];
        _descEnvTex     = _spheres[0]->material()->descriptors()[5];

        // 加载环境贴图
        {
            auto envFile = arch->openIStream("image/island.png", { comm::ReadFlag::binary });
            size_t sz = envFile->size();
            uint8_t* buf = (uint8_t*)malloc(sz);
            envFile->read(buf, sz);
            envFile->close();
            _envTex = CreateTexturePNG(dev, buf, (uint32_t)sz, _ctx->asyncLoadManager(),
                [](void* res, CommandBuffer* cmd) {
                    auto* tex = (Texture*)res;
                    tex->generateMipmap(cmd);
                    auto* re = cmd->resourceCommandEncoder();
                    re->imageTransitionBarrier(tex, ResourceAccessType::ShaderRead,
                        pipeline_stage_t::Bottom, StageAccess::Write,
                        pipeline_stage_t::FragmentShading, StageAccess::Read, nullptr);
                    re->endEncode();
                    tex->markAsUploaded();
                }
            );
            _descEnvSampler.res.samplerState = {};
            _descEnvTex.res.imageView = _envTex->defaultView().handle;
            for (auto* s : _spheres) {
                s->material()->updateDescriptor(_descEnvSampler);
                s->material()->updateDescriptor(_descEnvTex);
            }
        }

        return true;
    }

    void PBRApp::tick() {
        if (!_ctx->onPreTick()) return;
        _pipeline->resetMaterials();
        auto* dev = _ctx->device();
        auto* rp  = _ctx->mainFramebuffer();
        auto* cmd = _ctx->primaryQueue()->createCommandBuffer(dev, CmdbufType::Transient);
        dev->cycleInvoker().postCallable([this, dev, cmd]() {
            _ctx->primaryQueue()->destroyCommandBuffer(dev, cmd);
        });

        static float angle = 0; angle += 0.01f;
        auto ua = _ctx->uniformAllocator();

        // 固定相机
        SceneUBO scene;
        scene.cameraPos = glm::vec3(0, 2.5f, 6.0f);
        auto view = glm::lookAt(scene.cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        scene.viewProj = glm::perspective(glm::radians(45.0f), _width / _height, 0.1f, 50.0f) * view;
        ua->allocateForDescriptor(_descScene, &scene);

        // 旋转光源
        LightUBO lights;
        lights.lightDir   = glm::normalize(glm::vec3(sin(angle * 1.5f), 1.0f, cos(angle * 1.5f)));
        lights.lightColor = glm::vec3(3.0f, 2.8f, 2.5f);
        lights.ambient    = 0.12f;
        ua->allocateForDescriptor(_descLight, &lights);
        {
            auto ubo = ua->allocate(sizeof(MaterialUBO));
            memcpy(ubo.ptr, &_mats, sizeof(MaterialUBO));
            _descMaterial.res.buffer.buffer = ubo.buffer;
            _descMaterial.res.buffer.offset = ubo.offset;
            _descMaterial.res.buffer.size   = (uint32_t)sizeof(MaterialUBO);
        }

        cmd->beginEncode();
        {
            // 每帧更新每个球体的 UBO
            for (int i = 0; i < 8; ++i) {
                auto* mtl = _spheres[i]->material();
                auto  descs = mtl->descriptors();
                res_descriptor_t descModel = descs[1];

                float x = (float)(i % 4) * 2.5f - 3.75f;
                float y = (i < 4) ? 1.5f : -1.5f;
                ModelUBO mdl;
                mdl.model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0));
                mdl.materialIndex = (uint32_t)i;
                for (int j = 0; j < 3; ++j) mdl._pad[j] = 0;
                ua->allocateForDescriptor(descModel, &mdl);

                mtl->updateDescriptor(_descScene);
                mtl->updateDescriptor(descModel);
                mtl->updateDescriptor(_descLight);
                mtl->updateDescriptor(_descMaterial);
            }

            renderpass_clearval_t cv;
            cv.colors[0] = { 0.05f, 0.05f, 0.08f, 1.0f };
            cv.depth = 1.0f; cv.stencil = 0;
            rp->setClearValues(cv);

            auto* renc = cmd->renderCommandEncoder(rp);
            renc->setViewport(0, 0, _width, _height, 0, 1.0f);
            renc->setScissor(0, 0, _width, _height);
            renc->bindPipeline(_pipeline);
            for (int i = 0; i < 8; ++i) {
                _pipeline->applyMaterial(_spheres[i]->material());
                _pipeline->flushMaterials(cmd);
                renc->draw(_spheres[i]->mesh(), _spheres[i]->mesh()->indexCount());
            }
            renc->endEncode();
        }
        cmd->endEncode();

        // MSVC vector explicit ctor: 必须显式构造
        queue_submit_t submit;
        submit.commandBuffers_ = { cmd };
        submit.semaphoresWaits_ = { _ctx->mainFramebufferAvailSemaphore() };
        submit.semaphoresSignals_ = { _ctx->renderCompleteSemephore() };
        _ctx->submitCommand(std::move(submit));
        _ctx->onPostTick();
    }

    void PBRApp::resize(uint32_t w, uint32_t h) {
        _ctx->onResize(w, h); _width = w; _height = h;
    }

    void PBRApp::release() {}
}

ugi::PBRApp theApp;
UGIApplication* GetApplication() { return &theApp; }
