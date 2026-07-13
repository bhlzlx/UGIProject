#pragma once
#include <utils/singleton.h>
#include <glm/glm.hpp>

namespace gui {

    struct UIRay {
        glm::vec3 origin;
        glm::vec3 dir;
    };

    /// FairyGUI 运行上下文 — 屏幕参数、相机投影、射线检测、输入分发、渲染提交
    class FairyGUIContext : public comm::Singleton<FairyGUIContext> {
        friend class comm::Singleton<FairyGUIContext>;
    private:
        float       _screenW    = 0;
        float       _screenH    = 0;
        glm::mat4   _vp         = glm::mat4(1.0f);
        glm::vec3   _camPos     = {};
    private:
        FairyGUIContext() = default;
    public:
        float screenWidth()  const { return _screenW; }
        float screenHeight() const { return _screenH; }
        glm::mat4 const& vp() const { return _vp; }
        glm::vec3 camPos() const { return _camPos; }

        void setScreenSize(float w, float h);
        void setCamera(glm::mat4 const& vp, glm::vec3 camPos) { _vp = vp; _camPos = camPos; }

        /// 构建透视相机 VP 矩阵并缓存 (内部用 _screenW/_screenH)
        void buildPerspectiveCamera(float fovy = 45.0f);

        UIRay screenToRay(glm::vec2 screenPos) const;
        glm::vec2 screenToUIPos(glm::vec2 screenPos) const;

        void onMouseDown(int x, int y);
        void onMouseUp(int x, int y);
        void onMouseMove(int x, int y);
    };

}
