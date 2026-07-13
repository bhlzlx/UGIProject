#include "fairy_gui_context.h"
#include "core/ui/stage.h"
#include "core/ui/ui_content_scaler.h"
#include "render/ui_render.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace gui {

    void FairyGUIContext::setScreenSize(float w, float h) {
        _screenW = w;
        _screenH = h;
        Stage::Instance()->onResize(w, h);
        buildPerspectiveCamera();
    }

    void FairyGUIContext::buildPerspectiveCamera(float fovy) {
        glm::vec3 camPos;
        camPos.z = (_screenH / 2) / tan(fovy / 2.f / 180.f * 3.1415926f);
        camPos.x = _screenW / 2;
        camPos.y = _screenH / 2;
        glm::mat4 vm = glm::lookAt(camPos, glm::vec3(camPos.x, camPos.y, 0.f), glm::vec3(0, 1, 0));
        glm::mat4 pm = glm::perspective(glm::radians(fovy), _screenW / _screenH, 0.1f, camPos.z * 2);
        setCamera(pm * vm, camPos);
    }

    UIRay FairyGUIContext::screenToRay(glm::vec2 screenPos) const {
        float ndcX = (2.0f * screenPos.x / _screenW) - 1.0f;
        float ndcY = (2.0f * screenPos.y / _screenH) - 1.0f;  // Vulkan NDC: -1=top, +1=bottom"
        glm::vec4 nearPoint = glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        glm::mat4 invVP = glm::inverse(_vp);
        glm::vec4 worldNear = invVP * nearPoint;
        if (worldNear.w != 0) worldNear /= worldNear.w;
        glm::vec3 dir = glm::normalize(glm::vec3(worldNear) - _camPos);
        return {_camPos, dir};
    }

    glm::vec2 FairyGUIContext::screenToUIPos(glm::vec2 screenPos) const {
        auto ray = screenToRay(screenPos);
        if (ray.dir.z == 0) return {};
        float t = -ray.origin.z / ray.dir.z;
        return {ray.origin.x + ray.dir.x * t, ray.origin.y + ray.dir.y * t};
    }

    void FairyGUIContext::onMouseDown(int x, int y) {
        Stage::Instance()->onMouseDown(screenToUIPos({(float)x, (float)y}));
    }

    void FairyGUIContext::onMouseUp(int x, int y) {
        Stage::Instance()->onMouseUp(screenToUIPos({(float)x, (float)y}));
    }

    void FairyGUIContext::onMouseMove(int x, int y) {
        Stage::Instance()->onMouseMove(screenToUIPos({(float)x, (float)y}));
    }

}
