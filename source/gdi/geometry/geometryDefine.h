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

                    /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
        *  uniform 里主要是存共用的值，比如旋转，缩放，
        *  一般画一个矩形或者多个复杂的形状缩放和角度都是相同的，这个值是可以复用的
        *  基本上等同于一个 3x3 矩阵
        *  变换过程，先 加一个负的 anchor 偏移，再缩放，再旋转，再加一个 anchor 偏移
        * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
        /*
            1 0 -x  |  cos -sin 0  |  a 0 0  |  1 0 x
            0 1 -y  |  sin cos  0  |  0 b 0  |  0 1 y
            0 0  1  |  0   0    1  |  0 0 1  |  0 0 1
            从左到右变换，从右往左乘
        */
        /*
       */
        /*
            a*cos | -a*sin  | -a*cos*x+a*sin*y+x
            b*sin | b*cos   | -b*sin*x-b*cos*y+y
            0     | 0       | 1
        */
        struct alignas(16) GeometryTransformArgument {
            // 基本上等同于一个 3x3 矩阵
            // 变换过程，先 加一个负的 anchor 偏移，再绽放，再旋转，再加一个 anchor 偏移
            /*  我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了，GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            alignas(4) hgl::Vector3f col1; alignas(4) uint32_t colorMask;
            alignas(4) hgl::Vector3f col2; alignas(4) uint32_t extra;
            //
            GeometryTransformArgument();
            GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor);
            GeometryTransformArgument& operator = ( const GeometryTransformArgument& arg) {
                memcpy(this, &arg, sizeof(arg));
                return *this;
            }
        };

        struct ContextInformation {
            hgl::Vector2f contextSize;
            hgl::Vector2f padding;
            hgl::Vector4f contextSicssor;
            GeometryTransformArgument transform;

//vec2    contextSize;            // 屏幕大小
//vec4    contextScissor;         // 
//vec4    globalTrasform[2];      // 全局变换
        };

    }

}


        