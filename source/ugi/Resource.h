#pragma once 

namespace ugi {

    class Device;

    class Resource {
    public:
        virtual void release( Device* _device ) = 0;
        virtual ~Resource() = default;
    };

}