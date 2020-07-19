#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in uint color;
layout (location = 2) in uint uniformIndex;

layout (location = 0) out vec4  fragColor;
layout (location = 1) out float fragAlpha;
layout (location = 2) out float fragGray;
layout (location = 3) out vec4  fragScreenScissor;
// layout (location = 3) out vec4  frag_localScissor;
// layout (location = 4) out vec4  fragScreenScissor;

out gl_PerVertex  {
    vec4 gl_Position;
};

// alpha
struct ElementArguemnt {
    vec4 col1;          // mat3x3's col1 & alpha
    vec4 col2;          // mat3x3's col2 & gray
    // 裁剪不打算放这里了，只给一个全局裁剪其实就够了
};

layout( set = 0, binding = 0 ) uniform GlobalInformation {
    vec2    contextSize;            // 屏幕大小
    vec4    contextScissor;         // 
    vec4    globalTrasform[2];      // 全局变换
};

// 我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了
// GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
layout( set = 0, binding = 1 ) uniform ElementInformation {
	ElementArguemnt transform[1024];
};

void main() {
    // 局部变换
    float x = 0; float y = 0;
    // vec3 elementCol1, elementCol2;
    vec3 globalCol1, globalCol2;
    ElementArguemnt eleArg = transform[uniformIndex];
    // 
    //elementCol1 = eleArg.col1.xyz;
    //elementCol2 = eleArg.col2.xyz;
    //
    x = dot( vec3(position,1.0f), eleArg.col1.xyz );
    y = dot( vec3(position,1.0f), eleArg.col2.xyz );
    vec3 elementPos = vec3(x, y, 1.0f);
    //
    fragAlpha = eleArg.col1.w;
    fragGray = eleArg.col2.w;
    fragAlpha *= globalTrasform[0].w;
    fragGray = 1 - (1 - globalTrasform[1].w) * (1-fragGray);
    // 全局变换
    globalCol1 = globalTrasform[0].xyz;
    globalCol2 = globalTrasform[1].xyz;
    //
    x = dot( elementPos, globalCol1 );
    y = dot( elementPos, globalCol2 );
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
	fragColor = vec4(
        (color>>24)/255.0f, 
        (color>>16&0xff)/255.0f, 
        (color>>8&0xff)/255.0f,
        (color&0xff)/255.0f
    );
}
