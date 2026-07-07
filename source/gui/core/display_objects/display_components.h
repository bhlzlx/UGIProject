#pragma  once

#include <LightWeightCommon/utils/handle.h>
#include "core/data_types/ui_types.h"
#include "core/declare.h"
// #include "display_object.h"
#include "render/render_data.h"
#include <vector>
#include <memory>
#include <entt/src/entt/entt.hpp>

namespace gui {

    extern entt::registry reg;

    namespace dispcomp {

        // component 有这个组件
        struct children {
            std::vector<entt::entity> val;
        };

        // 除root以外所有控件都有这个组件
        struct parent {
            entt::entity val;
        };

        // batch节点有这个组件（batch节点是一种特殊的节点，子级的mesh合批的数据存到它这里）
        // 真正显示用来简化遍历的数据结构
        struct batch_node {
            std::vector<entt::entity> children;         // 按显示顺序排列的子节点，遇batch_node就停止向下遍历，只存子batch_node本身
            std::vector<entt::entity> batchNodes;       // 子batch_node节点
        };

        // batch节点有这个组件
        struct batch_data {
            std::vector<ui_render_batches_t> batches;
        };

        // batch节点需要更新构建mesh了（原因有很多，比如子级重排，mesh更新）
        struct batch_need_rebuild {}; 

        // 一般是item的mesh需要重新构建
        struct batch_dirty {}; 

        // 普通可显示的item有这个组件
        struct item_batch_info {
            entt::entity    batchEntity;    // 所属batch节点
            int             batchIdx;       // 第几个 ui_render_batch_t (512个inst切一次)
            int             instIndex;      // batch 内 args 的第几个位置
        };

        // root 节点有这个组件
        struct is_root {};

        // 所属object的引用，用来获取object指针
        struct owner {
            Handle val;
        };

        // 普通可显示的item有这个组件
        // image/font/shape 都有这个组件
        struct mesh_dirty {};

        struct image_ext {
            Color4B             color;
            FlipType            flip;
            float               fillAmount;
            FillMethod          fill;
            FillOrigin          fillOrig;
            bool                fillClockwise;
        };

        struct image_desc_t {
            texture_block_t     textureBlock;
            Rect<float> const*  grid9;
            bool                scaleByTile;
            image_ext           ext;
        };

        struct transform_dirty {}; // 变换更新
        struct args_dirty {};      // args 参数更新（transform/alpha/gray等），需同步到 batch cache

        struct visible {}; // 控件本身的可见性
        struct final_visible {}; // 最终提交相关的可见性
        struct visible_dirty {}; // 可见性是不是需要重计算

        // 因为基础变换基本所有的控件都会有，所以组合成一起了
        struct basic_transform {
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

        struct font_mesh {};

        struct polygon_mesh {};


    }
}