#pragma once
#include <core/ui/object.h>
#include <core/font_manager.h>
#include <string>
#include "IColorGear.h"

namespace gui {

    enum class AlignType : uint8_t { Left = 0, Center = 1, Right = 2 };
    enum class VertAlignType : uint8_t { Top = 0, Middle = 1, Bottom = 2 };

    struct TextFormat {
        std::string     font;
        float           fontSize        = 12.0f;
        Color4B         color           = {0xFFFFFFFF};  // packed RGBA
        AlignType       align           = AlignType::Left;
        VertAlignType   verticalAlign   = VertAlignType::Top;
        int16_t         lineSpacing     = 0;
        int16_t         letterSpacing   = 0;
        bool            underline       = false;
        bool            italic          = false;
        bool            bold            = false;
        bool            strikethrough   = false;
        Color4B         outlineColor    = {0xFFFFFFFF};  // packed RGBA
        float           outline         = 0;
        Color4B         shadowColor     = {0xFFFFFFFF};  // packed RGBA
        float           shadowOffsetX   = 0;
        float           shadowOffsetY   = 0;
    };

    class GTextField : public Object, public IOutlineColorGear {
    public:
        enum class AutoSize {
            None, Both, Height, Shrink
        };

    private:
        std::string     text_;
        int             fontID_     = -1;
        TextFormat      tf_;
        AutoSize        autoSize_   = AutoSize::None;
        bool            singleLine_ = true;
        float           textWidth_  = 0;
        float           textHeight_ = 0;

        DisplayObject   textDispobj_;

        void syncTextDesc();
        void applyAlignment();

    protected:
        void onSizeChanged() override;

    public:
        GTextField();
        ~GTextField();

        virtual void constructFromResource() override {}
        virtual void createDisplayObject() override;
        virtual void setupBeforeAdd(ByteBuffer& buffer, int startPos) override;
        virtual void setupAfterAdd(ByteBuffer& buffer, int startPos) override;

        DisplayObject textDisplayObject() const { return textDispobj_; }

        TextFormat const& textFormat() const { return tf_; }
        TextFormat& textFormat() { return tf_; }

        std::string const& text() const { return text_; }
        void setText(std::string const& val);

        void setColor(Color4B color) override;
        Color4B getColor() const override;
        void setOutlineColor(Color4B color) override;
        Color4B getOutlineColor() const override;

        int fontID() const { return fontID_; }
        void setFontID(int val);
        void setFontSize(float val) { tf_.fontSize = val; syncTextDesc(); }

        AutoSize autoSizeMode() const { return autoSize_; }
        void setAutoSize(AutoSize val);

        bool singleLine() const { return singleLine_; }
        void setSingleLine(bool val);

        float textWidth() const { return textWidth_; }
        float textHeight() const { return textHeight_; }
    };

}
