#pragma once
#include <core/declare.h>
#include <string>
#include <vector>

namespace gui {

    namespace toolset {

        Color4B hexToColor(const char* str);

        template<class T>
        Rect<T> intersection( const Rect<T>& a, const Rect<T>& b);

        uint32_t findIndexStringArray(std::vector<std::string> const& array, std::string const& str);

    };

}