#pragma once

#include "core/declare.h"
#include "utils/byte_buffer.h"
namespace gui {

    class ScrollPane {
    private:
    public:
        ScrollPane()
        {}

        void setup(ByteBuffer const& buff);
    };

}