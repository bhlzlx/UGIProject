#pragma once
#include <cstdint>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <core/events/ui_events.h>

namespace gui {

    enum class ValueType {
        Int,
        Float,
        String,
        Pointer,
        MouseEvent,
        TouchEvent,
        None,
    };

    class Value {
    private:
        ValueType   type_;
        struct {
            union {
                int64_t ival;
                double  fval;
                void*   ptr;
            };
            std::string sval;
        };
    public:
        Value()
            : type_(ValueType::None)
        {}

        Value(TouchEvent evt)
            : type_(ValueType::TouchEvent)
            , ival((int64_t(evt)))
        {}
        Value(MouseEvent evt)
            : type_(ValueType::MouseEvent)
            , ival((int64_t(evt)))
        {}

        template<class T>
        requires std::integral<T>
        Value(T val)
            : type_(ValueType::Int)
            , ival((int64_t)val)
        {}

        template<class T>
        requires std::integral<T>
        operator T () const {
            return T(ival);
        }

        template<class T>
        requires std::floating_point<T>
        Value(T val)
            : type_(ValueType::Float)
            , fval((double)val)
        {}

        template<class T>
        requires std::floating_point<T>
        operator T () const {
            return T(fval);
        }

        template<class T>
        requires std::same_as<std::string, T>
        Value(T&& val)
            : type_(ValueType::String)
            , sval(std::forward<T>(val))
        {
        }

        std::string const& stringVal() const {
            return sval;
        }

        template<class T>
        requires std::is_pointer_v<T>
        Value(T val)
            : type_(ValueType::Pointer)
            , ptr((void*)val)
        {
        }

        template<class T>
        requires std::is_pointer_v<T>
        operator T () const {
            return T(ptr);
        }

        bool operator == (Value const& val) const {
            if(val.type_ != type_) {
                return false;
            }
            switch(val.type_) {
            case ValueType::Int:
            case ValueType::MouseEvent:
            case ValueType::TouchEvent:
                return val.ival == ival;
            case ValueType::Float:
                return val.fval == fval;
            case ValueType::String:
                return val.sval == sval;
            case ValueType::Pointer:
                return val.ptr == ptr;
            case ValueType::None:
                return true;
            }
            return false;
        }

    };

}