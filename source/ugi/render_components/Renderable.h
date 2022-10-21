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
        GraphicsPipeline*             pipeline_;
    public:
        Renderable();
    };

}