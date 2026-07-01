#include "ui_content_scaler.h"
#include <algorithm>
#include <cmath>

namespace gui {

    void UIContentScaler::applyChange(float screenWidth, float screenHeight) {
        if (scaleMode == ScaleMode::ScaleWithScreenSize) {
            if (designResolutionX == 0 || designResolutionY == 0)
                return;

            int dx = designResolutionX;
            int dy = designResolutionY;

            // 如果不忽略方向，当屏幕方向与设计分辨率方向不一致时交换宽高
            if (!ignoreOrientation &&
                ((screenWidth > screenHeight && dx < dy) ||
                 (screenWidth < screenHeight && dx > dy))) {
                int tmp = dx;
                dx = dy;
                dy = tmp;
            }

            if (screenMatchMode == ScreenMatchMode::MatchWidthOrHeight) {
                float s1 = (float)screenWidth / dx;
                float s2 = (float)screenHeight / dy;
                scaleFactor = std::min(s1, s2);
            } else if (screenMatchMode == ScreenMatchMode::MatchWidth) {
                scaleFactor = (float)screenWidth / dx;
            } else { // MatchHeight
                scaleFactor = (float)screenHeight / dy;
            }
        } else if (scaleMode == ScaleMode::ConstantPhysicalSize) {
            // 在 C++ 环境中通常没有 DPI 概念，使用 fallback
            float dpi = (float)fallbackScreenDPI;
            if (dpi == 0) dpi = 96;
            scaleFactor = dpi / (defaultSpriteDPI == 0 ? 96 : defaultSpriteDPI);
        } else { // ConstantPixelSize
            scaleFactor = constantScaleFactor;
        }

        if (scaleFactor > 10.0f)
            scaleFactor = 10.0f;

        updateScaleLevel();
    }

    void UIContentScaler::updateScaleLevel() {
        if (scaleFactor > 3.0f)
            scaleLevel = 3; // x4
        else if (scaleFactor > 2.0f)
            scaleLevel = 2; // x3
        else if (scaleFactor > 1.0f)
            scaleLevel = 1; // x2
        else
            scaleLevel = 0; // x1
    }

}
