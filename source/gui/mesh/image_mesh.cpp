#include "image_mesh.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/display_objects/display_object_utility.h"
#include "core/ui/ui_content_scaler.h"
#include "core/font_manager.h"
#include "render/render_data.h"
#include "render/ui_render.h"
// #include "ui_image_render.h"
#include <cassert>
#include <cmath>

namespace gui {

// ---- helper: apply flip to a single UV ----
static void flipUV(glm::vec2& uv, glm::vec2 const& uvMin, glm::vec2 const& uvMax, FlipType flip)
{
    switch (flip) {
    case FlipType::Hori:
        uv.x = uvMin.x + uvMax.x - uv.x;
        break;
    case FlipType::Vert:
        uv.y = uvMin.y + uvMax.y - uv.y;
        break;
    case FlipType::Both:
        uv.x = uvMin.x + uvMax.x - uv.x;
        uv.y = uvMin.y + uvMax.y - uv.y;
        break;
    default:
        break;
    }
}

// ---- build a single quad and append to item ----
// 顶点顺序: 左上→右上→右下→左下 (Y=0 在顶部)
static void addQuad(
    image_mesh_t& item,
    glm::vec2 pos, glm::vec2 size,
    glm::vec2 uv0, glm::vec2 uv1,
    uint32_t color, FlipType flip,
    uint32_t instIndex)
{
    // 四个角在控件空间的坐标 (Y 轴向下)
    glm::vec2 tl = pos; // top-left
    glm::vec2 tr = { pos.x + size.x, pos.y }; // top-right
    glm::vec2 br = { pos.x + size.x, pos.y + size.y }; // bottom-right
    glm::vec2 bl = { pos.x, pos.y + size.y }; // bottom-left

    glm::vec2 u0 = uv0, u1 = uv1;
    flipUV(u0, uv0, uv1, flip);
    flipUV(u1, uv0, uv1, flip);

    uint16_t base = (uint16_t)item.vertices.size();
    // 0:top-left     uv(top-left)
    item.vertices.push_back({ { tl.x, tl.y, 0 }, color, { u0.x, u0.y }, instIndex });
    // 1:top-right    uv(top-right)
    item.vertices.push_back({ { tr.x, tr.y, 0 }, color, { u1.x, u0.y }, instIndex });
    // 2:bottom-right uv(bottom-right)
    item.vertices.push_back({ { br.x, br.y, 0 }, color, { u1.x, u1.y }, instIndex });
    // 3:bottom-left  uv(bottom-left)
    item.vertices.push_back({ { bl.x, bl.y, 0 }, color, { u0.x, u1.y }, instIndex });
    item.indices.insert(item.indices.end(), { base, (uint16_t)(base + 1), (uint16_t)(base + 2), base, (uint16_t)(base + 2), (uint16_t)(base + 3) });
}

// ============= createImageMesh =============

image_mesh_t createImageMesh(dispcomp::image_desc_t const& desc, dispcomp::basic_transform const& trans)
{
    image_mesh_t item;

    float ctrlW = trans.size.x;
    float ctrlH = trans.size.y;
    float texW = desc.textureBlock.size.x;
    float texH = desc.textureBlock.size.y;

    glm::vec2 uv0 = desc.textureBlock.uv[0];
    glm::vec2 uv1 = desc.textureBlock.uv[1];
    uint32_t color = desc.ext.color;
    FlipType flip = desc.ext.flip;

    // ---- case 1: scaleByTile ----
    if (desc.scaleByTile && texW > 0 && texH > 0) {
        for (float y = 0; y < ctrlH; y += texH) {
            float tileH = (std::min)(texH, ctrlH - y);
            for (float x = 0; x < ctrlW; x += texW) {
                float tileW = (std::min)(texW, ctrlW - x);
                float u1x = uv0.x + (uv1.x - uv0.x) * (tileW / texW);
                float u1y = uv0.y + (uv1.y - uv0.y) * (tileH / texH);
                addQuad(item, { x, y }, { tileW, tileH },
                    uv0, { u1x, u1y }, color, flip, 0);
            }
        }
        return item;
    }

    // ---- case 2: 9-slice ----
    // grid9: left  = 左角宽度(px), right = 左+中宽度(px)
    //        top   = 上角高度(px), bottom= 上+中高度(px)
    if (desc.grid9 != nullptr && texW > 0 && texH > 0) {
        auto const& g = *desc.grid9;

        float sf = UIContentScaler::Instance()->scaleFactor;

        // 四角在纹理中的像素大小 (跟随全局缩放)
        float wL = (std::max)(0.0f, g.left()) * sf;               // 左角宽
        float wR = (texW - (std::min)(texW, g.right())) * sf;     // 右角宽
        float hT = (std::max)(0.0f, g.top()) * sf;                // 上角高
        float hB = (texH - (std::min)(texH, g.bottom())) * sf;    // 下角高

        // 纹理空间的切片边界 (UV 用，不受缩放影响)
        float texL = wL / sf;            // 左切片边界 (恢复纹理像素)
        float texR = texW - wR / sf;     // 右切片边界
        float texT = hT / sf;            // 上切片边界
        float texB = texH - hB / sf;     // 下切片边界

        // ---- 控件空间 16 点 ----
        glm::vec2 pos[4][4] = {
            {{0,0},         {wL,0},             {ctrlW - wR,0},             {ctrlW,0}},
            {{0,hT},        {wL,hT},            {ctrlW - wR,hT},            {ctrlW,hT}},
            {{0,ctrlH - hB},{wL,ctrlH - hB},    {ctrlW - wR,ctrlH - hB},    {ctrlW,ctrlH - hB}},
            {{0,ctrlH},     {wL,ctrlH},         {ctrlW - wR,ctrlH},         {ctrlW,ctrlH}},
        };

        // ---- UV 空间 16 点 ----
        float du = uv1.x - uv0.x;
        float dv = uv1.y - uv0.y;
        glm::vec2 uvs[4][4] = {
            {{uv0.x, uv0.y},                       {uv0.x + du*(texL/texW), uv0.y},                       {uv0.x + du*(texR/texW), uv0.y},                       {uv1.x, uv0.y}},
            {{uv0.x, uv0.y + dv*(texT/texH)},      {uv0.x + du*(texL/texW), uv0.y + dv*(texT/texH)},      {uv0.x + du*(texR/texW), uv0.y + dv*(texT/texH)},      {uv1.x, uv0.y + dv*(texT/texH)}},
            {{uv0.x, uv0.y + dv*(texB/texH)},      {uv0.x + du*(texL/texW), uv0.y + dv*(texB/texH)},      {uv0.x + du*(texR/texW), uv0.y + dv*(texB/texH)},      {uv1.x, uv0.y + dv*(texB/texH)}},
            {{uv0.x, uv1.y},                       {uv0.x + du*(texL/texW), uv1.y},                       {uv0.x + du*(texR/texW), uv1.y},                       {uv1.x, uv1.y}},
        };

        // ---- 16点 → 9 quad ----
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                glm::vec2 qPos  = pos[row][col];
                glm::vec2 qSize = pos[row+1][col+1] - qPos;
                if (qSize.x <= 0 || qSize.y <= 0) continue;
                addQuad(item, qPos, qSize,
                        uvs[row][col], uvs[row+1][col+1],
                        color, flip, 0);
            }
        }
        return item;
    }

    // ---- case 3: plain image ----
    addQuad(item, { 0, 0 }, { ctrlW, ctrlH }, uv0, uv1, color, flip, 0);
    return item;
}

    // ============= createTextMesh =============

    image_mesh_t createTextMesh(dispcomp::text_desc_t const& desc,
                                 dispcomp::basic_transform const& trans,
                                 float& outWidth, float& outHeight) {
        image_mesh_t mesh;
        outWidth = 0; outHeight = 0;

        auto* fm = FontManager::Instance();
        if (!fm || desc.fontID < 0 || desc.text.empty()) return mesh;

        auto metrics = fm->getMetrics(desc.fontID, desc.fontSize);
        float sfScale = metrics.scale;
        float sdfSrcSize = (float)fm->config().sdfSourceSize;

        float penX = 0;
        float penY = (float)metrics.ascent * sfScale;
        float lineH = (float)(metrics.ascent - metrics.descent + metrics.lineGap) * sfScale;
        uint32_t color = 0xffffffff;// desc.color;
        float minX = 0, maxX = 0, minY = 0, maxY = 0;

        for (size_t i = 0; i < desc.text.size(); ++i) {
            uint32_t ch = (uint8_t)desc.text[i];
            if ((ch & 0x80u) && i + 1 < desc.text.size()) {
                uint32_t ch2 = (uint8_t)desc.text[i+1];
                if ((ch & 0xE0u) == 0xC0u) { ch = ((ch & 0x1Fu) << 6) | (ch2 & 0x3Fu); i++; }
                else if ((ch & 0xF0u) == 0xE0u && i + 2 < desc.text.size()) {
                    uint32_t ch3 = (uint8_t)desc.text[i+2];
                    ch = ((ch & 0x0Fu) << 12) | ((ch2 & 0x3Fu) << 6) | (ch3 & 0x3Fu); i += 2;
                }
            }
            if (ch == '\n') { penX = 0; penY += lineH; continue; }
            if (ch == '\r') continue;

            auto gi = fm->getGlyph(desc.fontID, ch);
            if (gi.bitmapWidth == 0) continue;

            float gScale = desc.fontSize / (gi.SDFScale * sdfSrcSize);
            float qx = penX + gi.bitmapBearingX * gScale;
            float qy = penY + gi.bitmapBearingY * gScale;
            float qw = (float)gi.bitmapWidth  * gScale;
            float qh = (float)gi.bitmapHeight * gScale;

            uint16_t base = (uint16_t)mesh.vertices.size();
            mesh.vertices.push_back({{qx, qy, 0}, color, {gi.texU, gi.texV}, 0});
            mesh.vertices.push_back({{qx+qw, qy, 0}, color, {gi.texU+gi.texW, gi.texV}, 0});
            mesh.vertices.push_back({{qx+qw, qy+qh, 0}, color, {gi.texU+gi.texW, gi.texV+gi.texH}, 0});
            mesh.vertices.push_back({{qx, qy+qh, 0}, color, {gi.texU, gi.texV+gi.texH}, 0});
            mesh.indices.insert(mesh.indices.end(), {base, (uint16_t)(base+1), (uint16_t)(base+2),
                                                      base, (uint16_t)(base+2), (uint16_t)(base+3)});
            if (qx < minX) minX = qx;
            if (qx+qw > maxX) maxX = qx+qw;
            if (qy < minY) minY = qy;
            if (qy+qh > maxY) maxY = qy+qh;
            penX += gi.bitmapAdvance * gScale;
        }
        outWidth  = maxX - minX;
        outHeight = maxY - minY;
        return mesh;
    }

    // ============= updateImageMesh =============

    void updateImageMesh()
    {
        // Image
        reg.view<dispcomp::mesh_dirty, dispcomp::final_visible, dispcomp::image_desc_t>().each([](entt::entity ett, dispcomp::image_desc_t& imageDesc) {
            dispcomp::item_render_data& graphics = reg.get_or_emplace<dispcomp::item_render_data>(ett);
            graphics.meshData.type = UIMeshType::Image;
            if (graphics.meshData.item) {
                delete (image_mesh_t*)graphics.meshData.item;
            }
            auto& trans = reg.get<dispcomp::basic_transform>(ett);
            auto* mesh = new image_mesh_t(std::move(createImageMesh(imageDesc, trans)));
            graphics.meshData.item = mesh;
            graphics.args.transfrom = glm::mat4(1.0f);
            auto& sync = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
            sync.mask |= dispcomp::Asm_Transform;
            reg.remove<dispcomp::mesh_dirty>(ett);
        });

        // Text
        reg.view<dispcomp::mesh_dirty, dispcomp::final_visible, dispcomp::text_desc_t>().each([](entt::entity ett, dispcomp::text_desc_t& textDesc) {
            dispcomp::item_render_data& graphics = reg.get_or_emplace<dispcomp::item_render_data>(ett);
            graphics.meshData.type = UIMeshType::Font;
            if (graphics.meshData.item) {
                delete (image_mesh_t*)graphics.meshData.item;
                graphics.meshData.item = nullptr;
            }
            auto* fm = FontManager::Instance();
            float sdfSrcSize = fm ? (float)fm->config().sdfSourceSize : 64.0f;
            if (fm) {
                graphics.texture = fm->sdfTexture()->handle();
            }
            auto& textTrans = reg.get<dispcomp::basic_transform>(ett);
            float textW, textH;
            image_mesh_t mesh = createTextMesh(textDesc, textTrans, textW, textH);
            if(mesh.vertices.size()) {
                auto* meshItem = new image_mesh_t(std::move(mesh));
                graphics.meshData.item = meshItem;
                // TODO: SDF smoothing params (baseSmoothing, sizeScale) need a proper packing location;
                // currently vertex shader hardcodes props to (0,0,0,0), so smoothing defaults to 0.03.
                auto& bounds = reg.get_or_emplace<dispcomp::text_bounds>(ett);
                bounds.width  = textW;
                bounds.height = textH;
            }
            reg.remove<dispcomp::mesh_dirty>(ett);
        });

        // Shape (GGraph)
        reg.view<dispcomp::mesh_dirty, dispcomp::final_visible, dispcomp::shape_desc_t>().each([](entt::entity ett, dispcomp::shape_desc_t& shapeDesc) {
            dispcomp::item_render_data& graphics = reg.get_or_emplace<dispcomp::item_render_data>(ett);
            graphics.meshData.type = UIMeshType::Image;
            if (graphics.meshData.item) {
                delete (image_mesh_t*)graphics.meshData.item;
                graphics.meshData.item = nullptr;
            }
            auto& trans = reg.get<dispcomp::basic_transform>(ett);
            auto mesh = createShapeMesh(shapeDesc, trans);
            if (mesh.vertices.size()) {
                graphics.meshData.item = new image_mesh_t(std::move(mesh));
            }
            graphics.args.transfrom = glm::mat4(1.0f);
            auto& sync = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
            sync.mask |= dispcomp::Asm_Transform;
            reg.remove<dispcomp::mesh_dirty>(ett);
        });
    }

    // ============= createShapeMesh =============

    image_mesh_t createShapeMesh(dispcomp::shape_desc_t const& desc, dispcomp::basic_transform const& trans) {
        image_mesh_t m;
        float w = trans.size.x, h = trans.size.y;
        if (w <= 0 || h <= 0) return m;
        uint32_t fillC = desc.fillColor;
        uint32_t lineC = desc.lineColor;
        float    ls    = desc.lineSize;

        auto addQuad = [&](float x0, float y0, float x1, float y1,
                           float x2, float y2, float x3, float y3, uint32_t c) {
            uint16_t base = (uint16_t)m.vertices.size();
            m.vertices.insert(m.vertices.end(), {
                {{x0, y0, 0}, c, {0, 0}, 0},
                {{x1, y1, 0}, c, {0, 0}, 0},
                {{x2, y2, 0}, c, {0, 0}, 0},
                {{x3, y3, 0}, c, {0, 0}, 0},
            });
            m.indices.insert(m.indices.end(), {base, (uint16_t)(base+1), (uint16_t)(base+2),
                                               base, (uint16_t)(base+2), (uint16_t)(base+3)});
        };

        switch (desc.type) {
        case dispcomp::shape_desc_t::Type::None:
            break;
        case dispcomp::shape_desc_t::Type::Rect: {
            if (ls > 0) {
                float hw = ls * 0.5f;
                float iw = w - ls;
                float ih = h - ls;
                // center fill
                if (iw > 0 && ih > 0) {
                    addQuad(hw, hw, w - hw, hw, w - hw, h - hw, hw, h - hw, fillC);
                }
                // 4 border strips (avoid corner overlap)
                if (ls > 0) {
                    addQuad(0, 0,  w, 0,  w, hw,  0, hw,       lineC);          // top
                    addQuad(0, h-hw, w, h-hw, w, h,  0, h,       lineC);          // bottom
                    addQuad(0, hw,  hw, hw,  hw, h-hw, 0, h-hw, lineC);          // left
                    addQuad(w-hw, hw, w, hw,  w, h-hw, w-hw, h-hw, lineC);       // right
                }
            } else {
                addQuad(0, 0, w, 0, w, h, 0, h, fillC);
            }
            break;
        }
        case dispcomp::shape_desc_t::Type::Ellipse: {
            float cx = w * 0.5f, cy = h * 0.5f;
            float rx = cx, ry = cy;
            constexpr int segs = 32;

            if (ls > 0 && rx > ls && ry > ls) {
                // ring: quads between outer and inner ellipse
                float irx = rx - ls, iry = ry - ls;
                for (int i = 0; i < segs; ++i) {
                    float a0 = (float)i       / (float)segs * 3.14159265f * 2.0f;
                    float a1 = (float)(i + 1) / (float)segs * 3.14159265f * 2.0f;
                    float cos0 = cosf(a0), sin0 = sinf(a0);
                    float cos1 = cosf(a1), sin1 = sinf(a1);
                    // outer ring (lineColor), inner ring (fillColor) — use lineColor for entire ring
                    addQuad(
                        cx + cos0 * rx, cy + sin0 * ry,
                        cx + cos1 * rx, cy + sin1 * ry,
                        cx + cos1 * irx, cy + sin1 * iry,
                        cx + cos0 * irx, cy + sin0 * iry,
                        lineC);
                }
                // center fill
                uint16_t ci = (uint16_t)m.vertices.size();
                m.vertices.push_back({{cx, cy, 0}, fillC, {0, 0}, 0});
                for (int i = 0; i <= segs; ++i) {
                    float a = (float)i / (float)segs * 3.14159265f * 2.0f;
                    m.vertices.push_back({{cx + cosf(a) * irx, cy + sinf(a) * iry, 0}, fillC, {0, 0}, 0});
                }
                for (int i = 0; i < segs; ++i)
                    m.indices.insert(m.indices.end(), {ci, (uint16_t)(ci+i+1), (uint16_t)(ci+i+2)});
            } else {
                // solid ellipse
                uint16_t ci = (uint16_t)m.vertices.size();
                m.vertices.push_back({{cx, cy, 0}, fillC, {0, 0}, 0});
                for (int i = 0; i <= segs; ++i) {
                    float a = (float)i / (float)segs * 3.14159265f * 2.0f;
                    m.vertices.push_back({{cx + cosf(a) * rx, cy + sinf(a) * ry, 0}, fillC, {0, 0}, 0});
                }
                for (int i = 0; i < segs; ++i)
                    m.indices.insert(m.indices.end(), {ci, (uint16_t)(ci+i+1), (uint16_t)(ci+i+2)});
            }
            break;
        }
        case dispcomp::shape_desc_t::Type::RegularPolygon: {
            int n = desc.sides;
            if (n < 3) break;
            float cx = w * 0.5f, cy = h * 0.5f;
            float radius = (std::min)(cx, cy);
            float startRad = desc.startAngle * 3.14159265f / 180.0f;
            float angleDelta = 3.14159265f * 2.0f / (float)n;
            bool hasDist = desc.distances.size() >= (size_t)n;

            if (ls > 0 && radius > ls) {
                // ring: quads between outer (dist*radius) and inner (dist*radius - ls)
                for (int i = 0; i < n; ++i) {
                    float d0 = hasDist ? desc.distances[i] : 1.0f;
                    float d1 = hasDist ? desc.distances[(i+1)%n] : 1.0f;
                    float a0 = startRad + (float)i     * angleDelta;
                    float a1 = startRad + (float)(i+1) * angleDelta;
                    float r0o = d0 * radius;
                    float r1o = d1 * radius;
                    float r0i = (std::max)(0.0f, r0o - ls);
                    float r1i = (std::max)(0.0f, r1o - ls);
                    float cos0 = cosf(a0), sin0 = sinf(a0);
                    float cos1 = cosf(a1), sin1 = sinf(a1);
                    addQuad(
                        cx + cos0 * r0o, cy + sin0 * r0o,
                        cx + cos1 * r1o, cy + sin1 * r1o,
                        cx + cos1 * r1i, cy + sin1 * r1i,
                        cx + cos0 * r0i, cy + sin0 * r0i,
                        lineC);
                }
                // center fill: fan from center to inner vertices
                uint16_t ci = (uint16_t)m.vertices.size();
                m.vertices.push_back({{cx, cy, 0}, fillC, {0, 0}, 0});
                for (int i = 0; i <= n; ++i) {
                    float d = hasDist ? desc.distances[i % n] : 1.0f;
                    float r = (std::max)(0.0f, d * radius - ls);
                    float a = startRad + (float)(i % n) * angleDelta;
                    m.vertices.push_back({{cx + cosf(a) * r, cy + sinf(a) * r, 0}, fillC, {0, 0}, 0});
                }
                for (int i = 0; i < n; ++i)
                    m.indices.insert(m.indices.end(), {ci, (uint16_t)(ci+i+1), (uint16_t)(ci+i+2)});
            } else {
                // solid regular polygon: fan from center
                uint16_t ci = (uint16_t)m.vertices.size();
                m.vertices.push_back({{cx, cy, 0}, fillC, {0, 0}, 0});
                for (int i = 0; i <= n; ++i) {
                    float d = hasDist ? desc.distances[i % n] : 1.0f;
                    float a = startRad + (float)(i % n) * angleDelta;
                    m.vertices.push_back({{cx + cosf(a) * d * radius, cy + sinf(a) * d * radius, 0}, fillC, {0, 0}, 0});
                }
                for (int i = 0; i < n; ++i)
                    m.indices.insert(m.indices.end(), {ci, (uint16_t)(ci+i+1), (uint16_t)(ci+i+2)});
            }
            break;
        }
        case dispcomp::shape_desc_t::Type::Polygon: {
            auto const& pts = desc.polygonPoints;
            if (pts.size() < 3) break;
            size_t n = pts.size();

            if (ls > 0) {
                // border: quads along each edge, offset inward/outward by half lineSize
                float hw = ls * 0.5f;
                for (size_t i = 0; i < n; ++i) {
                    size_t j = (i + 1) % n;
                    glm::vec2 p0 = pts[i];
                    glm::vec2 p1 = pts[j];
                    glm::vec2 edge = p1 - p0;
                    float len = glm::length(edge);
                    if (len < 0.0001f) continue;
                    glm::vec2 dir = edge / len;
                    glm::vec2 nrm(-dir.y, dir.x);  // perpendicular
                    // outer vertices offset outward
                    float ox0 = p0.x + nrm.x * hw, oy0 = p0.y + nrm.y * hw;
                    float ox1 = p1.x + nrm.x * hw, oy1 = p1.y + nrm.y * hw;
                    // inner vertices offset inward
                    float ix0 = p0.x - nrm.x * hw, iy0 = p0.y - nrm.y * hw;
                    float ix1 = p1.x - nrm.x * hw, iy1 = p1.y - nrm.y * hw;
                    addQuad(ox0, oy0, ox1, oy1, ix1, iy1, ix0, iy0, lineC);
                }
                // center fill (original polygon, shrunk by half lineSize)
                std::vector<glm::vec2> innerPts(n);
                for (size_t i = 0; i < n; ++i) {
                    // simple inward offset: move each vertex along its angle bisector
                    size_t prev = (i + n - 1) % n;
                    size_t next = (i + 1) % n;
                    glm::vec2 e1 = glm::normalize(pts[i] - pts[prev]);
                    glm::vec2 e2 = glm::normalize(pts[next] - pts[i]);
                    glm::vec2 bisector = e1 + e2;
                    float bisLen = glm::length(bisector);
                    if (bisLen < 0.0001f) {
                        // collinear edges: use perpendicular
                        bisector = glm::vec2(-e1.y, e1.x);
                        bisLen = 1.0f;
                    }
                    glm::vec2 inNrm = bisector / bisLen;
                    // dot with edge normal to determine sign
                    glm::vec2 edgeNrm(-e1.y, e1.x);
                    float scale = hw / glm::dot(inNrm, edgeNrm);
                    innerPts[i] = pts[i] - inNrm * scale;
                }
                uint16_t base = (uint16_t)m.vertices.size();
                for (auto& p : innerPts)
                    m.vertices.push_back({{p.x, p.y, 0}, fillC, {0, 0}, 0});
                for (size_t i = 1; i + 1 < n; ++i)
                    m.indices.insert(m.indices.end(), {base, (uint16_t)(base+i), (uint16_t)(base+i+1)});
            } else {
                // solid polygon
                uint16_t base = (uint16_t)m.vertices.size();
                for (auto& p : pts)
                    m.vertices.push_back({{p.x, p.y, 0}, fillC, {0, 0}, 0});
                for (size_t i = 1; i + 1 < n; ++i)
                    m.indices.insert(m.indices.end(), {base, (uint16_t)(base+i), (uint16_t)(base+i+1)});
            }
            break;
        }
        }
        return m;
    }

    // ============= updateTextAlignment =============

    void updateTextAlignment() {
        // 遍历有 text_bounds 和 text_desc_t 的实体，根据对齐方式调整其显示位置
        reg.view<dispcomp::text_bounds, dispcomp::text_desc_t>().each([](entt::entity ett,
                                                                         dispcomp::text_bounds& bounds,
                                                                         dispcomp::text_desc_t& desc) {
            auto parent = getParent(ett);
            if (!parent) return;
            auto& pTrans = reg.get<dispcomp::basic_transform>(parent.entity());

            float ctrlW = pTrans.size.x;
            float ctrlH = pTrans.size.y;
            float textW = bounds.width;
            float textH = bounds.height;

            float ox = 0, oy = 0;
            if (desc.align == 1)       ox = (ctrlW - textW) * 0.5f;
            else if (desc.align == 2)  ox = ctrlW - textW;
            if (desc.verticalAlign == 1)   oy = (ctrlH - textH) * 0.5f;
            else if (desc.verticalAlign == 2) oy = ctrlH - textH;

            auto& trans = reg.get<dispcomp::basic_transform>(ett);
            if (trans.position.x != ox || trans.position.y != oy) {
                trans.position.x = ox;
                trans.position.y = oy;
                reg.emplace_or_replace<dispcomp::transform_dirty>(ett);
                auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(ett);
                s.mask |= dispcomp::Asm_Transform;
            }
        });
    }
}
