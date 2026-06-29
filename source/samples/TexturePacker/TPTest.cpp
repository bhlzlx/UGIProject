#include "TPTest.h"
#include <ugi/device.h>
#include <ugi/command_buffer.h>
#include <ugi/render_pass.h>
#include <ugi/pipeline.h>
#include <ugi/descriptor_binder.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/render_context.h>
#include <io/archive.h>
#include <tweeny.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/glm.hpp>

namespace ugi {


    SDFRenderParameter sdfParam = {
        1024, 4, 32, 64, 8, 8
    };

    static glm::vec2 anchor3;

    bool TPTest::initialize(void* wnd, comm::IArchive* arch) {
        device_descriptor_t dd;
        dd.apiType = GraphicsAPIType::VULKAN;
        dd.deviceType = GraphicsDeviceType::DISCRETE;
        dd.debugLayer = 1;
        dd.graphicsQueueCount = 1;
        dd.transferQueueCount = 1;
        dd.wnd = wnd;
        _ctx = StandardRenderContext::Instance();
        _ctx->initialize(wnd, dd, arch);

        _fontRenderer = new SDFFontRenderer();
        _fontRenderer->initialize(_ctx->device(), arch, _ctx->asyncLoadManager(), sdfParam);

        char16_t textArray[][64] = {
            u".找不到路径，", u"因为该路径不存在。", u"全英雄选择", u"testgoogle..."
        };
        Style styleArray[5] = {
            { 0xff5555ff, 0xffffffff, 0x00000000 },
            { 0x55ff55ff, 0xffffffff, 0x00000001 },
            { 0x5555ffff, 0xffffffff, 0x00000002 },
            { 0xff0000ff, 0xffffffff, 0x00000003 },
            { 0x005555ff, 0xffffffff, 0x00000002 },
        };

        _fontRenderer->beginBuild();
        glm::vec2 rcPos(0), rcSize(0);

        auto makeSDFChars = [](const char16_t* txt, std::vector<SDFChar>& out) {
            out.clear();
            for (auto* p = txt; *p; ++p) {
                out.push_back({ 0, 36U, (uint32_t)*p });
            }
        };

        Transform identity = { glm::vec3(1,0,0), glm::vec3(0,1,0)};

        std::vector<SDFChar> vecChar;
        makeSDFChars(textArray[0], vecChar);
        RectScope2f rect;
        _h1 = _fontRenderer->appendText(32, 360, vecChar.data(), vecChar.size(), identity, styleArray[0], rect);

        makeSDFChars(textArray[1], vecChar);
        _h2 = _fontRenderer->appendTextReuseTransform(rect.GetRight(), 360, vecChar.data(), vecChar.size(), _h1, styleArray[1], rect);

        makeSDFChars(textArray[2], vecChar);
        _h3 = _fontRenderer->appendTextReuseStyle(rect.GetRight(), 360, vecChar.data(), vecChar.size(), _h2, identity, rect);
        anchor3 = glm::vec2(rect.GetCenterX(), 360);

        makeSDFChars(textArray[3], vecChar);
        _h4 = _fontRenderer->appendTextReuse(rect.GetRight(), 360, vecChar.data(), vecChar.size(), _h3, rect);

        _drawData = _fontRenderer->endBuild();
        return true;
    }

    void TPTest::tick() {
        if (!_ctx->onPreTick()) return;

        auto* dev = _ctx->device();
        auto* rp  = _ctx->mainFramebuffer();
        auto* cmd = _ctx->primaryQueue()->createCommandBuffer(dev, CmdbufType::Transient);
        dev->cycleInvoker().postCallable([this, dev, cmd]() {
            _ctx->primaryQueue()->destroyCommandBuffer(dev, cmd);
        });

        static auto t = tweeny::from(1.5).to(4.0).during(60);
        if (t.progress() <= 0.0f) t = t.forward();
        else if (t.progress() >= 1.0f) t = t.backward();
        auto scale = t.step(1);

        auto trans = Transform::createTransform(anchor3, glm::vec2(scale));
        _drawData->updateTransform(_h3, trans);

        cmd->beginEncode();
        {
            auto* re = cmd->resourceCommandEncoder();
            _fontRenderer->tickResource(re);
            _fontRenderer->prepareResource(re, &_drawData, 1, _ctx->uniformAllocator());
            re->endEncode();

            renderpass_clearval_t cv;
            cv.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f };
            cv.depth = 1.0f; cv.stencil = 0;
            rp->setClearValues(cv);

            auto* renc = cmd->renderCommandEncoder(rp);
            renc->setViewport(0, 0, _width, _height, 0, 1.0f);
            renc->setScissor(0, 0, _width, _height);
            _fontRenderer->draw(renc, &_drawData, 1);
            renc->endEncode();
        }
        cmd->endEncode();

        _ctx->submitCommand({ {cmd}, {_ctx->mainFramebufferAvailSemaphore()}, {_ctx->renderCompleteSemephore()} });
        _ctx->onPostTick();
    }

    void TPTest::resize(uint32_t w, uint32_t h) {
        _ctx->onResize(w, h);
        _width = w; _height = h;
        _fontRenderer->resize(w, h);
    }

    void TPTest::release() {}

}
ugi::TPTest theApp;
UGIApplication* GetApplication() { return &theApp; }
