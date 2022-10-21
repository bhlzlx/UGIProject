#pragma once
#include <UGITypes.h>
#include <LightWeightCommon/string/name.h>
#include <unordered_map>

namespace ugi {

    struct MaterialDescriptor {
        char const*             name; // name of the descriptor - written in shader
        res_descriptor_type     type; // type of the descriptor
        res_descriptor_t        descriptor; // the resouces descriptor
    };

    class MaterialLayout {
    private:
        std::unordered_map<comm::Name, uint32_t>     _handleMap;
    public:
        MaterialLayout();
    };

    class Material {
        struct material_desc_t {
            uint32_t handle;
        };
    protected:
    public:
    };

}