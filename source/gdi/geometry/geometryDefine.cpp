#include "geometryDefine.h"

namespace ugi {

    namespace gdi {

        GeometryTransformArgument::GeometryTransformArgument() {
            data[0] = hgl::Vector4f( 1.0f, 0.0f, 0.0f, 1.0f);
            data[1] = hgl::Vector4f( 0.0f, 1.0f, 0.0f, 0.0f);
            // data[2] = hgl::Vector4f( 11.0f, 256.0f, 11.0f, 256.0f);
        }

        GeometryTransformArgument::GeometryTransformArgument(float rad, const hgl::Vector2f& scale, const hgl::Vector2f& anchor)
        {
            float c = cos(rad); float s = sin(rad);
            float a = scale.x; float b = scale.y;
            float dx = anchor.x; float dy = anchor.y;
            /*  我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了，GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
                a*cos -a*sin -a*cos*x+a*sin*y+x
                b*sin b*cos  -b*sin*x-b*cos*y+y
            */
            data[0] = hgl::Vector4f(a*c, -a*s, (1-a*c)*dx + a*s*dy, 0.0f);
            data[1] = hgl::Vector4f(b*s, b*c,  (1-b*c)*dy - b*s*dx, 0.0f);
        }

    }

}