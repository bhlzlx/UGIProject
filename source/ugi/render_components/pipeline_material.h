#pragma once
#include <Pipeline.h>

namespace ugi {

    struct MaterialDescriptor {
        char const*             name; // name of the descriptor - written in shader
        ArgumentDescriptorType  type; // type of the descriptor
        ResourceDescriptor      descriptor; // the resouces descriptor
    };

    class Material {
        struct material_desc_t {
            uint32_t handle;
        };
    protected:
    public:
    };

}