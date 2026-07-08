#pragma once
#include <core/ui/object.h>
#include <core/font_manager.h>
#include <string>

namespace gui {

    /// SDF 文字渲染控件
    class GTextField : public Object {
    public:
        enum class AutoSize {
            None,       // 不自动调整大小
            Both,       // 根据文字内容调整宽高
            Height,     // 只自动调整高度
            Shrink,     // 缩小字号以适应宽度
        };

    private:
        std::string     text_;
        int             fontID_     = -1;   // FontManager 中的字体索引
        float           fontSize_   = 12.0f;
        uint32_t        color_      = 0xffffffff;
        AutoSize        autoSize_   = AutoSize::None;
        bool            singleLine_ = true;
        float           textWidth_  = 0;    // 排版结果的实际像素宽
        float           textHeight_ = 0;    // 排版结果的实际像素高

        bool            textDirty_  = true;

    protected:
        void onSizeChanged() override;

    public:
        GTextField();

        /// 重建文字 mesh (外部可调用)
        void updateTextMesh();
        ~GTextField();

        virtual void constructFromResource() override {}
        virtual void createDisplayObject() override;

        // 属性
        std::string const& text() const { return text_; }
        void setText(std::string const& val);

        int fontID() const { return fontID_; }
        void setFontID(int val);

        float fontSize() const { return fontSize_; }
        void setFontSize(float val);

        uint32_t color() const { return color_; }
        void setColor(uint32_t val);

        AutoSize autoSizeMode() const { return autoSize_; }
        void setAutoSize(AutoSize val);

        bool singleLine() const { return singleLine_; }
        void setSingleLine(bool val);

        float textWidth() const { return textWidth_; }
        float textHeight() const { return textHeight_; }
    };

}
