#pragma  once

#include <LightWeightCommon/utils/handle.h>
#include "core/data_types/ui_types.h"
#include "core/declare.h"
// #include "display_object.h"
#include "render/render_data.h"
#include <vector>

#include <entt/src/entt/entt.hpp>

namespace gui {

    extern entt::registry reg;

    namespace dispcomp {

        struct children {
            std::vector<entt::entity> val;
        };

        struct parent {
            entt::entity val;
        };

        struct parent_batch {
            entt::entity    val; // 父batch
            int             instIndex; // 此元件的inst数据索引
        };

        struct batch_node {
            std::vector<entt::entity> children;
            std::vector<entt::entity> batchNodes;
        };

        struct batch_data {
            std::vector<ui_render_batches_t> batches;
        };

        struct mesh_dirty {};

        struct image_ext {
            Color4B             color;
            FlipType            flip;
            float               fillAmount;
            FillMethod          fill;
            FillOrigin          fillOrig;
            bool                fillClockwise;
        };

        struct image_mesh {
            image_desc_t desc;
            Rect<float>* grid9;
            bool         scaleByTile;
            image_ext    ext;
        };

        struct font_mesh {};

        struct polygon_mesh {};

        struct is_root {};

        struct transform_dirty {}; // 变换更新

        struct batch_node_dirty {}; // batch node 需要更新树结构了

        struct batch_dirty {}; // 需要重新构建 batch 信息

        struct owner {
            Handle val;
        };

        struct visible {}; // 控件本身的可见性
        struct final_visible {}; // 最终提交相关的可见性
        struct visible_dirty {}; // 可见性是不是需要重计算

        struct graphics {
            NGraphics graphics;
        };

        // 因为基础变换基本所有的控件都会有，所以组合成一起了
        struct basic_transfrom {
            glm::vec2 position;
            glm::vec2 size;
            glm::vec2 pivot;
        };

        // 而 skew/scale/rotation 并不是所有的控件都有
        struct skew {
            glm::vec2 val;
        };

        struct scale {
            glm::vec2 val;
        };

        struct rotation {
            float val;
        };

    }
}