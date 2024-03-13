#pragma once
#include <ugi_types.h>


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
        raster_state_t                  rasterState_;           // depth bias/front face/ cull mode/ polygon mode ...
    public:
        Renderable(Mesh* mesh, Material* mtr, GraphicsPipeline* pipeline, raster_state_t const& rss);
        bool prepared() const;
        Mesh const* mesh() const;
        Material* material() const;
        raster_state_t rasterState() const;
        GraphicsPipeline const* pipeline() const;
        //
        void draw(RenderCommandEncoder* encoder);

        ~Renderable();
    };

}