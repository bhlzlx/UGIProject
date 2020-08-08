#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in uint color;
layout (location = 2) in uint uniformIndex;

layout (location = 0) out vec4  fragColor;
layout (location = 1) out float fragGray;
layout (location = 2) out vec4  fragScreenScissor;
// layout (location = 3) out vec4  frag_localScissor;
// layout (location = 4) out vec4  fragScreenScissor;

out gl_PerVertex  {
    vec4 gl_Position;
};

// 这里为什么不用vec3 uint，因为vec3对齐占用vec4的大小能省则省
// 裁剪不打算放这里了，只给一个全局裁剪其实就够了
struct Transform {
    float col11; float col12; float col13; uint colorMask;  // mat3x3's col1 & color mask
    float col21; float col22; float col23; uint gray;       // mat3x3's col2 & gray
};

layout( set = 0, binding = 0 ) uniform GlobalInformation {
    vec2        contextSize;            // 屏幕大小
    vec4        contextScissor;         // 
    Transform   globalTrasform;         // 全局变换
};

vec4 uint32ToVec4( uint val ) {
    return vec4( (val>>24)/255.0f, (val>>16&0xff)/255.0f, (val>>8&0xff)/255.0f, (val&0xff)/255.0f );
}

// 我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了
// GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
layout( set = 0, binding = 1 ) uniform ElementInformation {
	Transform transform[1024];
};

void main() {
    // 局部变换
    float x = 0; float y = 0;
    // vec3 elementCol1, elementCol2;
    Transform elementTransform = transform[uniformIndex];
    vec3 col1 = vec3(elementTransform.col11,elementTransform.col12,elementTransform.col13);
    vec3 col2 = vec3(elementTransform.col21,elementTransform.col22,elementTransform.col23);
    vec3 gcol1 = vec3(globalTrasform.col11,globalTrasform.col12,globalTrasform.col13);
    vec3 gcol2 = vec3(globalTrasform.col21,globalTrasform.col22,globalTrasform.col23);

    x = dot( vec3(position,1.0f), col1 );
    y = dot( vec3(position,1.0f), col2 );
    vec3 elementPos = vec3(x, y, 1.0f);
    //
    vec4 col24 = uint32ToVec4(elementTransform.gray);   // 存灰度的参数
    vec4 gcol24 = uint32ToVec4(globalTrasform.gray);    // 存灰度的参数
    fragGray = col24.x;
    fragGray = 1 - (1 - gcol24.x) * (1-fragGray);       // 计算最终灰度
    // 全局变换
    x = dot( elementPos, gcol1 );
    y = dot( elementPos, gcol2 );
    //
    vec2 scissorCentor = vec2( (contextScissor.x+contextScissor.y)/2, (contextScissor.z+contextScissor.w)/2);
    vec2 scissorHalfSize = vec2(abs(contextScissor.x-contextScissor.y)/2,abs(contextScissor.z-contextScissor.w)/2);
    vec2 scissorDistance = vec2(x,y) - scissorCentor;
    fragScreenScissor.xy = scissorDistance;
    fragScreenScissor.zw = scissorHalfSize;
    // 转换为NDC
    x = ( x / contextSize.x * 2.0f) - 1.0f;
    y = ( y / contextSize.y * 2.0f) - 1.0f;
    // 生成gl_position
	gl_Position = vec4( x,y,0, 1.0f );
    // 实际上可以做成查表的形式
    vec4 vertexColor = uint32ToVec4(color);
    vec4 colorMask = uint32ToVec4(elementTransform.colorMask);
	fragColor = vertexColor*colorMask;
}
