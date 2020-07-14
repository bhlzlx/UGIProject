#include "geometryDefine.h"

namespace ugi {

    namespace gdi {

        GeometryTransformArgument::GeometryTransformArgument()
        {
        }

        GeometryTransformArgument::GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor)
        {
            float cosValue = cos(rad); float sinValue = sin(rad);
            float a = scale.x; float b = scale.y; float x = anchor.x; float y = anchor.y;
            /*  我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了，GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector4f(a*cosValue, -a*sinValue, -a*cosValue*x + a*sinValue*y + x, 0.0f);
            data[1] = hgl::Vector4f(b*sinValue, b*cosValue,  -b*sinValue*x - b*cosValue*y + y, 0.0f);
        }

    }

}