#pragma once

#include "core/data_types/ui_types.h"

namespace gui {

    class FillMesh {
    private:
        FillMethod          fillMethod_;
        int                 fillOrigin_; 
        float               amount_;
        bool                clockwise_;
    public:
        FillMesh()
            : fillMethod_(FillMethod::Hori)
            , fillOrigin_(0)
            , amount_(1.f)
        {}
    };

}