#pragma once
#include <Pipeline.h>

namespace ugi {

    struct MaterialDescriptor {
        char const*             name; // name of the descriptor - written in shader
        res_descriptor_type  type; // type of the descriptor
        res_descriptor_t      descriptor; // the resouces descriptor
    };

    class Material {
        struct material_desc_t {
            uint32_t handle;
        };
    protected:
    public:
    };

}