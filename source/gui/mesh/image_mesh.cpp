#include "image_mesh.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/ui/ui_content_scaler.h"
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
// 顶点顺序: 左上→右上→右下→左下 (Y=0 在顶部, 与原 createImageItem 一致)
static void addQuad(
    image_item_t& item,
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

image_item_t createImageMesh(dispcomp::image_desc_t const& desc, dispcomp::basic_transform const& trans)
{
    image_item_t item;

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

// ============= updateImageMesh =============

void updateImageMesh()
{
    reg.view<dispcomp::mesh_dirty, dispcomp::final_visible, dispcomp::image_desc_t>().each([](auto ett, dispcomp::image_desc_t& imageDesc) {
        NGraphics* graphics = &reg.get_or_emplace<NGraphics>(ett);
        graphics->renderItem.type = RenderItemType::Image;
        if (graphics->renderItem.item) {
            delete (image_item_t*)graphics->renderItem.item;
        }
        auto& trans = reg.get<dispcomp::basic_transform>(ett);
        auto* item = new image_item_t(std::move(createImageMesh(imageDesc, trans)));
        graphics->renderItem.item = item;
        reg.remove<dispcomp::mesh_dirty>(ett);
    });
}
}
