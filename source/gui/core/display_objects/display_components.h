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

        struct is_root {
        };

        struct owner {
            Handle val;
        };

        struct image_info {
            Color4B             color;
            float               fillAmount;
            FillMethod          fill;
            FillOrigin          fillOrig;
            FlipType            flip;
            bool                fillClockwise;
        };

        struct graphics {
            NGraphics graphics;
        };

    }
}