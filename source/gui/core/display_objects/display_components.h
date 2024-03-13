#pragma  once

#include <LightWeightCommon/utils/handle.h>
#include "core/data_types/ui_types.h"
#include "core/declare.h"
#include "display_object.h"
#include "render/render_data.h"
#include <vector>

#include <entt/src/entt/entt.hpp>

namespace gui {

    extern entt::registry reg;

    namespace dispcomp {

        struct children {
            std::vector<DisplayObject> val;
        };

        struct parent {
            entt::entity val;
        };

        struct parent_batch {
            entt::entity val;
        };

        struct batch_node {
            std::vector<DisplayObject> children;
            std::vector<DisplayObject> batchNodes;
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

        struct batch_dirty {};

        struct owner {
            Handle val;
        };


        struct visible {};
        struct final_visible {};
        struct visible_changed { bool visible; };

        struct graphics {
            NGraphics graphics;
        };

    }
}