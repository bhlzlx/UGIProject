#include "geometryDefine.h"

namespace ugi {

    namespace gdi {

        GeometryTransformArgument::GeometryTransformArgument()
            : col1( hgl::Vector3f(1.0f, 0.0f, 0.0f))
            , colorMask(0xffffffff)
            , col2( hgl::Vector3f(0.0f, 1.0f, 0.0f))
            , extra(0x00ffffff)
        {
        }

        GeometryTransformArgument::GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor) {
            float c = cos(rad); float s = sin(rad);
            float a = scale.x; float b = scale.y;
            float dx = anchor.x; float dy = anchor.y;
            /*  我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了，GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            col1 = hgl::Vector3f(a*c, -a*s, (1-a*c)*dx + a*s*dy);
            col2 = hgl::Vector3f(b*s, b*c,  (1-b*c)*dy - b*s*dx);
            colorMask = 0xffffffff;
            extra = 0x00ffffff;
        }

    }

}