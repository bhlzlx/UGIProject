#pragma once
#include <hgl/math/Vector.h>

namespace ugi {

    namespace gdi {

            typedef uint32_t GeometryHandle;

            /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
            *  1. uniformElement 如果是0表明不旋转也不缩放
            *  2. color 这个属性实际上可以在shader里算最终颜色也可以提供一个颜色表来做映射（可能更快，目前先不这么做）
            *  3. position 的值如果想省CPU则可以放到GPU里作浮点运算（先放CPU里算）
            * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
            struct GeometryVertex {
                hgl::Vector2f   position;           // 位置
                uint32_t        color;              // 颜色索引 R(16) G(16) B(16) A(16) 每个8位
                uint32_t        uniformElement;     // uniform 元素的索引
            };

    }

}


        