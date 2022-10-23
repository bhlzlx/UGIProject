#pragma once
#include <UGITypes.h>


namespace ugi {

    /**
     * @brief 
     *  Mesh & Pipeline & Pipeline Material
     *
    */

    class Renderable {
    private:
        Mesh*                           mesh_;                  // mesh : vertices & vertex layout info & polygon & topology
        Material*                       material_;              // material : pipeline parameters
        GraphicsPipeline*               pipeline_;              // pipeline : the graphics pipeline that can render this mesh
        raster_state_t                  rasterState_;           // render state ： pipeline dynamic render state : depth/stencil/blend/cullmode/etc..
    public:
        Renderable();
        bool prepared() const;
        void draw(RenderCommandEncoder* encoder);
    };

}