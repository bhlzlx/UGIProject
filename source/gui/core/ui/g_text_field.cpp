#include "g_text_field.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/font_manager.h"
#include "utils/byte_buffer.h"
#include "render/render_data.h"

namespace gui {

    GTextField::GTextField() {
        type_ = ObjectType::Text;
    }

    GTextField::~GTextField() = default;

    void GTextField::syncTextDesc() {
        auto ett = textDispobj_.entity();
        if (ett == entt::null) return;
        auto& desc = reg.get_or_emplace<dispcomp::text_desc_t>(ett);
        desc.text          = text_;
        desc.fontID        = fontID_;
        desc.fontSize      = tf_.fontSize;
        desc.color         = tf_.color;
        desc.align         = (uint8_t)tf_.align;
        desc.verticalAlign = (uint8_t)tf_.verticalAlign;
        setColor(tf_.color);
        reg.emplace_or_replace<dispcomp::mesh_dirty>(ett);
        reg.emplace_or_replace<dispcomp::batch_dirty>(ett);
        auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
        s.mask |= dispcomp::Asm_Transform | dispcomp::Asm_Color;
        s.mask |= dispcomp::Asm_Transform;
        textWidth_  = 0;
        textHeight_ = 0;
    }

    void GTextField::applyAlignment() {
        auto ett = textDispobj_.entity();
        if (ett == entt::null) return;
        float textW = textWidth_, textH = textHeight_;
        if (reg.any_of<dispcomp::text_bounds>(ett)) {
            auto& b = reg.get<dispcomp::text_bounds>(ett);
            textW = b.width; textH = b.height;
        }
        float ox = 0, oy = 0;
        float ctrlW = width(), ctrlH = height();
        switch ((AlignType)tf_.align) {
        case AlignType::Center: ox = (ctrlW - textW) * 0.5f; break;
        case AlignType::Right:  ox = ctrlW - textW;           break;
        default: break;
        }
        switch ((VertAlignType)tf_.verticalAlign) {
        case VertAlignType::Middle: oy = (ctrlH - textH) * 0.5f; break;
        case VertAlignType::Bottom: oy = ctrlH - textH;           break;
        default: break;
        }
        textDispobj_.setPosition({ox, oy});
    }

    void GTextField::createDisplayObject() {
        Object::createDisplayObject();
        textDispobj_ = DisplayObject::createDisplayObject();
        auto ett = textDispobj_.entity();
        auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(ett);
        gfx.meshData.type = UIMeshType::Font;
        reg.emplace<dispcomp::text_desc_t>(ett);
        reg.emplace_or_replace<dispcomp::visible>(ett);
        reg.emplace_or_replace<dispcomp::visible_dirty>(ett);
        dispobj_.addChild(textDispobj_);
        syncTextDesc();
    }

    // ===== Setters =====

    void GTextField::setText(std::string const& val) {
        if (text_ == val) return;
        text_ = val;
        syncTextDesc();
    }

    void GTextField::setFontID(int val) {
        if (fontID_ == val) return;
        fontID_ = val;
        syncTextDesc();
    }

    void GTextField::setAutoSize(AutoSize val) {
        if (autoSize_ == val) return;
        autoSize_ = val;
        syncTextDesc();
    }

    void GTextField::setSingleLine(bool val) {
        if (singleLine_ == val) return;
        singleLine_ = val;
        syncTextDesc();
    }


    void GTextField::setColor(glm::vec3 const& color) {
        tf_.color = color;
        auto ett = textDispobj_.entity();
        if (ett != entt::null && reg.any_of<dispcomp::item_render_data>(ett)) {
            auto& gfx = reg.get<dispcomp::item_render_data>(ett);
            gfx.args.color.x = color.x;
            gfx.args.color.y = color.y;
            gfx.args.color.z = color.z;
            gfx.args.color.w = 1.0f;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
            s.mask |= dispcomp::Asm_Color;
        }
    }
    glm::vec3 GTextField::getColor() const {
        return tf_.color;
    }
    void GTextField::setOutlineColor(glm::vec3 const& color) {
        tf_.outlineColor = color;
        auto ett = textDispobj_.entity();
        if (ett != entt::null && reg.any_of<dispcomp::item_render_data>(ett)) {
            auto& gfx = reg.get<dispcomp::item_render_data>(ett);
            // gfx.args.outlineColor.x = color.x;
            // gfx.args.outlineColor.y = color.y;
            // gfx.args.outlineColor.z = color.z;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
            s.mask |= dispcomp::Asm_Color;
        }
    }
    glm::vec3 GTextField::getOutlineColor() const {
        return tf_.outlineColor;
    }

    void GTextField::onSizeChanged() {
        Object::onSizeChanged();
    }

    // ---- 从 .fui 二进制读取文本属性 ----

    void GTextField::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);

        buffer.seekToBlock(startPos, TextFieldBlocks::Style);

        tf_.font          = buffer.read<csref>();
        fontID_           = 0;  // TODO: font name → fontID 查找
        tf_.fontSize      = (float)buffer.read<int16_t>();
        Color4B color4b   = buffer.read<uint32_t>();
        tf_.color         = color4b;
        tf_.align         = (gui::AlignType)buffer.read<uint8_t>();
        tf_.verticalAlign = (gui::VertAlignType)buffer.read<uint8_t>();
        tf_.lineSpacing   = buffer.read<int16_t>();
        tf_.letterSpacing = buffer.read<int16_t>();
        /*ubbEnabled*/    buffer.read<bool>();
        autoSize_         = (AutoSize)buffer.read<uint8_t>();
        tf_.underline     = buffer.read<bool>();
        tf_.italic        = buffer.read<bool>();
        tf_.bold          = buffer.read<bool>();
        singleLine_       = buffer.read<bool>();

        if (buffer.read<bool>()) {
            Color4B outlineColor4b = buffer.read<uint32_t>();
            tf_.outlineColor = outlineColor4b;
            tf_.outline      = buffer.read<float>();
        }

        if (buffer.read<bool>()) {
            Color4B shadowColor4b = buffer.read<uint32_t>();
            tf_.shadowColor   = shadowColor4b;
            tf_.shadowOffsetX = buffer.read<float>();
            tf_.shadowOffsetY = buffer.read<float>();
        }

        if (buffer.read<bool>())
            buffer.read<uint16_t>(); // templateVars count

        if (buffer.version >= 3) {
            tf_.strikethrough = buffer.read<bool>();
            buffer.skip(12); // faceDilate/outlineSoftness/underlaySoftness
        }
    }

    void GTextField::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Object::setupAfterAdd(buffer, startPos);
        buffer.seekToBlock(startPos, TextFieldBlocks::Text);
        auto str = buffer.read<csref>();
        if (str.size()) setText(std::string(str));
    }

}
