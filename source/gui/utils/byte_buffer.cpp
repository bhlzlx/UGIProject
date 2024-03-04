#include "byte_buffer.h"

namespace gui {

    static std::string EmptyString = "";

    ByteBuffer ByteBuffer::readShortBuffer() {
        int count = read<uint16_t>();
        ByteBuffer buffer(ptr() + position_, count);
        buffer.stringTable_ = this->stringTable_;
        buffer.version = this->version;
        position_ += count;
        return buffer;
    }

}