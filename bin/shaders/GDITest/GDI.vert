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

// 我们要往shader里传 mat3x3 但是第三行总是 (0,0,1)所以就可以不传了
// GLSL里有限制内存布局是vec3占用空间也是vec4，所以我们就传两个vec4来代替mat3x3
layout( set = 0, binding = 0 ) uniform Transform {
	vec4 transform[1024*2];
};

layout( set = 0, binding = 1 ) uniform ContextInfo {
    float screenWidth;
    float screenHeight;
};

void main() {
    vec3 col1 = transform[uniformIndex*2].xyz;
    vec3 col2 = transform[uniformIndex*2+1].xyz;
    float x = dot( vec3(position,1.0f), col1 );
    float y = dot( vec3(position,1.0f), col2 );
    // 转换为NDC
    x = ( x / screenWidth * 2.0f) - 1.0f;
    y = ( y / screenHeight * 2.0f) - 1.0f;
	gl_Position = vec4( x,y,0, 1.0f );
	frag_color = vec4(
        (color>>24)/255.0f, 
        (color>>16&0xff)/255.0f, 
        (color>>8&0xff)/255.0f,
        (color&0xff)/255.0f
    );
}
