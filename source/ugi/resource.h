#pragma once 

#include <LightWeightCommon/utils/handle.h>

namespace ugi {

    class Device;

    class Resource : public comm::Handle {
    public:
        Resource()
            : Handle(this)
        {}
        virtual void release(Device* _device) = 0;
        virtual ~Resource() = default;
    };

}