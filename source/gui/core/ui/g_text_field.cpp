#include "g_text_field.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/font_manager.h"
#include "render/render_data.h"

namespace gui {

    GTextField::GTextField() {
        type_ = ObjectType::Text;
    }

    GTextField::~GTextField() = default;

    void GTextField::syncTextDesc() {
        if (!dispobj_) return;
        auto& desc = reg.get_or_emplace<dispcomp::text_desc_t>(dispobj_);
        desc.text     = text_;
        desc.fontID   = fontID_;
        desc.fontSize = fontSize_;
        desc.color    = color_;
        reg.emplace_or_replace<dispcomp::mesh_dirty>(dispobj_);
        reg.emplace_or_replace<dispcomp::batch_dirty>(dispobj_);
    }

    void GTextField::createDisplayObject() {
        Object::createDisplayObject();
        auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        gfx.meshData.type = UIMeshType::Font;
        reg.emplace<dispcomp::text_desc_t>(dispobj_);
        syncTextDesc();
    }

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

    void GTextField::setFontSize(float val) {
        if (fontSize_ == val) return;
        fontSize_ = val;
        syncTextDesc();
    }

    void GTextField::setColor(uint32_t val) {
        color_ = val;
        if (!dispobj_) return;
        auto& gfx = reg.get<dispcomp::item_render_data>(dispobj_);
        gfx.args.color = glm::vec4(
            float((val >> 16) & 0xff) / 255.0f,
            float((val >> 8) & 0xff) / 255.0f,
            float(val & 0xff) / 255.0f,
            float((val >> 24) & 0xff) / 255.0f
        );
        auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(dispobj_);
        s.mask |= dispcomp::Asm_Color;
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

    void GTextField::onSizeChanged() {
        Object::onSizeChanged();
    }

}
