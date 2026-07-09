#pragma once
#include "render/render_data.h"
#include "core/display_objects/display_components.h"

namespace gui {

    /// <summary>
    /// 根据 image_desc 和控件变换信息生成 mesh 数据
    /// 支持普通/九宫/平铺三种模式，flip UV 已内置
    /// </summary>
    image_mesh_t createImageMesh(dispcomp::image_desc_t const& desc, dispcomp::basic_transform const& trans);
    image_mesh_t createTextMesh(dispcomp::text_desc_t const& desc);

    void updateImageMesh();

}
