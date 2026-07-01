#pragma once

#include "utils/singleton.h"
#include <core/ui/root.h>
#include <core/ui/ui_content_scaler.h>
#include <set>

namespace gui {

    class Stage : public comm::Singleton<Stage> {
        friend class Singleton<Stage>;
    private:
        Root*                   ui2dRoot_;
        std::set<Root*>         ui3dRoots_;
        UIContentScaler         contentScaler_;
        float                   screenWidth_ = 0;
        float                   screenHeight_ = 0;
        //
        Stage()
            : ui2dRoot_(nullptr)
            , ui3dRoots_{}
            , contentScaler_{}
        {
        }
    public:
        void initialize();

        Root* defaultRoot() const {
            return ui2dRoot_;
        }

        /// <summary>
        /// 获取 UIContentScaler 的引用，用于配置缩放模式
        /// </summary>
        UIContentScaler& contentScaler() { return contentScaler_; }

        /// <summary>
        /// 设置屏幕/窗口尺寸，触发 scaleFactor 重算并应用到 UI
        /// </summary>
        void setScreenSize(uint32_t width, uint32_t height);

        /// <summary>
        /// 获取当前屏幕宽度
        /// </summary>
        float screenWidth() const { return screenWidth_; }

        /// <summary>
        /// 获取当前屏幕高度
        /// </summary>
        float screenHeight() const { return screenHeight_; }

        /// <summary>
        /// 根据 contentScaler 的 scaleFactor 调整 root 的大小和缩放
        /// 参考 FairyGUI GRoot.ApplyContentScaleFactor
        /// </summary>
        void applyContentScaleFactor();

        static glm::vec2 RayHitTest(glm::vec3 eye, glm::vec3 dir, glm::vec3 triangle[3]);

    };

}