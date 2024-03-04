#pragma once
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <string>
#include <vector>
#include <compiler_def.h>
#include <core/declare.h>

namespace gui {

    enum class GeneralBlock {
    };

    enum class PackageBlocks {
        Dependences = 0,
        Items = 1,
        Sprites = 2,
        HitTestData = 3,
        StringTable = 4,
        TextField = 5,
    };

    enum class ComponentBlocks {
        Props = 0,
        Controller = 1,
        Children = 2,
        Relations = 3,
        CustomData = 4,
        Transitions = 5,
        Ext, // button, combobox, progressbar, scrollbar, slider
        ScrollData = 7,
    };

    enum class ObjectBlocks {
        Props = 0,
        Extra = 1,
        Gears = 2,
        Relations = 3,
    };

    // 目测序列化FGUI是按小端存储的
    class ByteBuffer {
    public:
        int                         version;
    private:
        uint8_t*                    ptr_;
        int                         offset_;
        int                         length_;
        int                         position_;
        uint8_t                     ownBuffer_ : 1;
        std::vector<std::string>*   stringTable_;
    public:
        ByteBuffer() 
            : ptr_(nullptr)
            , offset_(0)
            , length_(0)
            , position_(0)
            , ownBuffer_(0)
            , stringTable_(nullptr)
        {
        }
        ByteBuffer(ByteBuffer const& buffer) {
            if(ownBuffer_ && ptr_) {
                delete []ptr_;
            }
            memcpy(this, &buffer, sizeof(buffer));
        }
        ByteBuffer(ByteBuffer && buffer) {
            if(ownBuffer_ && ptr_) {
                delete []ptr_;
            }
            memcpy(this, &buffer, sizeof(buffer));
            memset(&buffer, 0, sizeof(buffer));
        }
        ByteBuffer& operator = (ByteBuffer && buffer) {
            if(ownBuffer_ && ptr_) {
                delete []ptr_;
            }
            memcpy(this, &buffer, sizeof(buffer));
            memset(&buffer, 0, sizeof(buffer));
            return *this;
        }
        ByteBuffer(uint8_t* ptr, int len)
            : ptr_(ptr)
            , offset_(0)
            , length_(len)
            , position_(0)
            , ownBuffer_(0)
        {}

        ByteBuffer(int len)
            : ptr_(new uint8_t[len])
            , offset_(0)
            , length_(ptr_?len:0)
            , position_(0)
            , ownBuffer_(1)
        {
        }

        ByteBuffer clone() const {
            auto buff = ByteBuffer(length_);
            memcpy(buff.ptr_, ptr(), length_);
            buff.offset_ = 0;
            buff.stringTable_ = stringTable_;
            return buff;
        }

        uint8_t* ptr() const {
            return ptr_ + offset_;
        }

        ~ByteBuffer() {
            if(ownBuffer_ && ptr_) {
                delete []ptr_;
            }
        }

        int pos() const {
            return position_;
        }

        int length() const {
            return length_;
        }

        void setPos(int pos) {
            position_ = pos;
        }

        void skip(int offset) {
            if(offset + position_ < length_) {
                position_ += offset;
            }
        }

        template<class T>
        inline T read() {
            static_assert(std::is_trivial_v<T>, "must be pod type!");
            T val;
            #if LITTLE_ENDIAN // 这里需要注意一下大小端问题
            memcpy(&val, ptr_ + offset_ + position_, sizeof(T));
            #else
            uint8_t* dst = (uint8_t*)&val;
            for(size_t i = 0; i<sizeof(T); ++i) {
                dst[sizeof(T)-1-i] = ptr_[offset_+position_+i];
            }
            #endif
            position_ += sizeof(T);
            return val;
        }

        template<class T>
        requires std::is_enum_v<T>
        inline T read() {
            return (T)read<std::underlying_type<T>>();
        }

        template<>
        inline std::string read<std::string>() {
            std::string val;
            uint16_t len = read<uint16_t>();
            if(len) {
                val.resize(len);
                memcpy(&val[0], ptr_ + position_ + offset_, len);
                position_ += len;
            }
            return val;
        }

        /**
         * @brief 
         * 
         * @tparam  return a bytebuffer ref
         * @return ByteBuffer<GeneralBlock> 
         */
        template<>
        inline ByteBuffer read<ByteBuffer>() {
            int count = read<int>();
            ByteBuffer buffer(ptr() + position_, count);
            buffer.stringTable_ = this->stringTable_;
            buffer.version = this->version;
            position_ += count;
            return buffer;
        }

        void updateRefString(std::string const& str) {
            auto index = this->read<uint16_t>();
            if(stringTable_->size() > index) {
                stringTable_->at(index) = str;
            }
        }

        void readRefStringArray(std::vector<std::string>& vec, uint32_t count) {
            for(uint32_t i = 0; i<count; ++i) {
                vec.push_back(read<csref>());
            }
        }
        template<class BlockType>
        requires std::is_enum_v<BlockType>
        bool seekToBlock(int indexTablePos, BlockType blockIndex) {
            auto bak = position_;
            position_ = indexTablePos;
            auto blockCount = read<uint8_t>();
            if((int)blockIndex<blockCount) {
                bool indexIsInt16 = (read<uint8_t>() == 1); // index is int16_t or int32_t
                int newPos = 0;
                if(indexIsInt16) {
                    // read specified block index value
                    position_ += sizeof(int16_t) * (int)blockIndex; // locate the position that stores the block's offset
                    newPos = read<int16_t>();
                } else {
                    position_ += sizeof(int32_t) * (int)blockIndex;
                    newPos = read<int32_t>();
                }
                if(newPos>0) {
                    position_ = indexTablePos + newPos;
                    return true;
                }
            }
            // restore the state
            position_ = bak;
            return false;
        }

        inline static std::string EmptyString = "";

        template<>
        inline csref read<csref>() {
            auto index = this->read<uint16_t>();
            if(stringTable_->size() > index) {
                return stringTable_->at(index);
            }
            return EmptyString;
        }

        ByteBuffer readShortBuffer();

    };

}
