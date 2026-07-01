#pragma once
#include <cstdint>

namespace gui {

/// <summary>
/// UI 内容缩放器 —— 移植自 FairyGUI UIContentScaler
/// 负责根据屏幕尺寸和设计分辨率计算全局缩放因子
/// </summary>
class UIContentScaler {
public:
    enum class ScaleMode : uint8_t {
        ConstantPixelSize,      // 固定像素大小
        ScaleWithScreenSize,    // 根据屏幕大小缩放
        ConstantPhysicalSize    // 根据物理尺寸缩放 (DPI)
    };

    enum class ScreenMatchMode : uint8_t {
        MatchWidthOrHeight,     // 匹配宽或高 (取较小值，保证全部可见)
        MatchWidth,             // 匹配宽度
        MatchHeight             // 匹配高度
    };

    ScaleMode       scaleMode = ScaleMode::ConstantPixelSize;
    ScreenMatchMode screenMatchMode = ScreenMatchMode::MatchWidthOrHeight;
    int             designResolutionX = 0;
    int             designResolutionY = 0;
    int             fallbackScreenDPI = 96;
    int             defaultSpriteDPI = 96;
    float           constantScaleFactor = 1.0f;
    bool            ignoreOrientation = false;

    float           scaleFactor = 1.0f;     // 全局缩放因子
    int             scaleLevel = 0;          // 缩放等级: 0=x1, 1=x2, 2=x3, 3=x4

    /// <summary>
    /// 根据当前屏幕尺寸重新计算 scaleFactor
    /// </summary>
    void applyChange(float screenWidth, float screenHeight);

    /// <summary>
    /// 根据 scaleFactor 更新 scaleLevel
    /// </summary>
    void updateScaleLevel();
};

}
