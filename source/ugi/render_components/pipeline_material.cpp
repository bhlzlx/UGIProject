#include "pipeline_material.h"

namespace ugi {

    void Material::updateDescriptor(res_descriptor_t const& descriptor) {
        for(auto& item: descriptors_) {
            if(item.handle == descriptor.handle) {
                item.res = descriptor.res;
                break;
            }
        }
    }

}