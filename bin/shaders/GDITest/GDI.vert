#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in uint color;
layout (location = 2) in uint uniformIndex;

layout (location = 0) out vec4 frag_color;

out gl_PerVertex  {
    vec4 gl_Position;
};

layout( set = 0, binding = 1 ) uniform GlobalInformation {
    vec2    screenSize;             // 屏幕大小
    vec4    globalTrasform[2];      // 全局变换
};

// 我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了
// GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
layout( set = 0, binding = 0 ) uniform ElementInformation {
	vec4 transform[1024*2];
};

void main() {
    // 局部变换
    float x = 0; float y = 0;
    vec3 elementCol1, elementCol2;
    vec3 globalCol1, globalCol2;
    // 
    elementCol1 = transform[uniformIndex*2].xyz;
    elementCol2 = transform[uniformIndex*2+1].xyz;
    x = dot( vec3(position,1.0f), elementCol1 );
    y = dot( vec3(position,1.0f), elementCol2 );
    vec3 elementPos = vec3(x, y, 1.0f);
    // 全局变换
    globalCol1 = globalTrasform[0].xyz;
    globalCol2 = globalTrasform[1].xyz;
    x = dot( elementPos, globalCol1 );
    y = dot( elementPos, globalCol2 );
    // 转换为NDC
    x = ( x / screenSize.x * 2.0f) - 1.0f;
    y = ( y / screenSize.y * 2.0f) - 1.0f;
    // 生成gl_position
	gl_Position = vec4( x,y,0, 1.0f );
    // 实际上可以做成查表的形式
	frag_color = vec4(
        (color>>24)/255.0f, 
        (color>>16&0xff)/255.0f, 
        (color>>8&0xff)/255.0f,
        (color&0xff)/255.0f
    );
}
