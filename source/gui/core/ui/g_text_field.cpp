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

    void GTextField::createDisplayObject() {
        Object::createDisplayObject();
        auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        gfx.meshData.type = UIMeshType::Font;
    }

    // ===== Setters =====

    // 设置文本属性后统一调一次
    static void markTextRebuild(GTextField* tf) {
        tf->updateTextMesh();
        // 通知父 batch 节点重建
        if (tf->getDisplayObject()) {
            reg.emplace_or_replace<dispcomp::batch_dirty>(tf->getDisplayObject());
        }
    }

    void GTextField::setText(std::string const& val) {
        if (text_ == val) return;
        text_ = val;
        textDirty_ = true;
        markTextRebuild(this);
    }

    void GTextField::setFontID(int val) {
        if (fontID_ == val) return;
        fontID_ = val;
        textDirty_ = true;
        markTextRebuild(this);
    }

    void GTextField::setFontSize(float val) {
        if (fontSize_ == val) return;
        fontSize_ = val;
        textDirty_ = true;
        markTextRebuild(this);
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
        textDirty_ = true;
        markTextRebuild(this);
    }

    void GTextField::setSingleLine(bool val) {
        if (singleLine_ == val) return;
        singleLine_ = val;
        textDirty_ = true;
        markTextRebuild(this);
    }

    // ===== Size / Layout =====

    void GTextField::onSizeChanged() {
        Object::onSizeChanged();
        // TODO: 如果开启了 shrink 模式，需要重新排版
    }

    // ===== Text Mesh Generation =====

    void GTextField::updateTextMesh() {
        if (!textDirty_) return;
        textDirty_ = false;

        auto* fm = FontManager::Instance();
        if (!fm || fontID_ < 0 || text_.empty()) return;

        // 字体度量
        auto metrics = fm->getMetrics(fontID_, fontSize_);
        float sfScale = metrics.scale;  // 将 EM 单位转为像素的系数
        float sdfSourceSize = (float)fm->config().sdfSourceSize;

        auto& gfx = reg.get<dispcomp::item_render_data>(dispobj_);

        // 构建 mesh
        image_mesh_t mesh;
        float penX = 0;
        float penY = (float)metrics.ascent * sfScale;  // 基线 Y
        float lineH = (float)(metrics.ascent - metrics.descent + metrics.lineGap) * sfScale;

        float minX = 0, maxX = 0, minY = 0, maxY = 0;

        for (size_t i = 0; i < text_.size(); ++i) {
            uint32_t ch = (uint8_t)text_[i];
            // UTF-8 多字节处理
            if ((ch & 0x80u) && i + 1 < text_.size()) {
                uint32_t ch2 = (uint8_t)text_[i+1];
                if ((ch & 0xE0u) == 0xC0u) {
                    ch = ((ch & 0x1Fu) << 6) | (ch2 & 0x3Fu);
                    i += 1;
                } else if ((ch & 0xF0u) == 0xE0u && i + 2 < text_.size()) {
                    uint32_t ch3 = (uint8_t)text_[i+2];
                    ch = ((ch & 0x0Fu) << 12) | ((ch2 & 0x3Fu) << 6) | (ch3 & 0x3Fu);
                    i += 2;
                }
            }

            // 换行
            if (ch == '\n' && !singleLine_) {
                penX = 0;
                penY += lineH;
                continue;
            }
            if (ch == '\r' || ch == '\n') continue;

            auto gi = fm->getGlyph(fontID_, ch);
            if (gi.bitmapWidth == 0) continue;  // 空格或加载中

            // 缩放系数：源 SDF 大小 → 目标字号
            float gScale = fontSize_ / (gi.SDFScale * sdfSourceSize);

            float qx = penX + gi.bitmapBearingX * gScale;
            float qy = penY + gi.bitmapBearingY * gScale;
            float qw = (float)gi.bitmapWidth  * gScale;
            float qh = (float)gi.bitmapHeight * gScale;

            // quad (左上→右上→右下→左下, Y轴向下)
            uint16_t base = (uint16_t)mesh.vertices.size();
            mesh.vertices.push_back({{qx, qy, 0}, color_, {gi.texU, gi.texV}, 0});
            mesh.vertices.push_back({{qx+qw, qy, 0}, color_, {gi.texU+gi.texW, gi.texV}, 0});
            mesh.vertices.push_back({{qx+qw, qy+qh, 0}, color_, {gi.texU+gi.texW, gi.texV+gi.texH}, 0});
            mesh.vertices.push_back({{qx, qy+qh, 0}, color_, {gi.texU, gi.texV+gi.texH}, 0});
            mesh.indices.insert(mesh.indices.end(), {base, (uint16_t)(base+1), (uint16_t)(base+2),
                                                      base, (uint16_t)(base+2), (uint16_t)(base+3)});

            if (qx < minX) minX = qx;
            if (qx+qw > maxX) maxX = qx+qw;
            if (qy < minY) minY = qy;
            if (qy+qh > maxY) maxY = qy+qh;

            penX += gi.bitmapAdvance * gScale;
        }

        textWidth_  = maxX - minX;
        textHeight_ = maxY - minY;

        // 存储 mesh
        if (gfx.meshData.item) {
            delete (image_mesh_t*)gfx.meshData.item;
        }
        gfx.meshData.item = new image_mesh_t(std::move(mesh));
        gfx.meshData.type = UIMeshType::Font;

        // 绑定 SDF 纹理
        gfx.texture = fm->sdfTexture()->handle();

        // SDF 平滑参数 → props.x
        gfx.args.props.x = 0.025f;  // smooth range

        // 如需要 autoSize，更新自身大小
        if (autoSize_ == AutoSize::Both) {
            setSize({textWidth_, textHeight_});
        } else if (autoSize_ == AutoSize::Height) {
            setHeight(textHeight_);
        }
    }

}
