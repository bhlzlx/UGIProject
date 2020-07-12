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

layout( set = 0, binding = 0 ) uniform Transform {
	mat3 transform[1024];
};

layout( set = 0, binding = 1 ) uniform ContextInfo {
    float screenWidth;
    float screenHeight;
};

void main() {
    float x = (position.x / screenWidth * 2.0f) - 1.0f;
    float y = (position.y / screenHeight * 2.0f) - 1.0f;
	gl_Position = vec4( x, y, 0.0f, 1.0f );
	frag_color = vec4(
        (color>>24)/255.0f, 
        (color>>16&0xff)/255.0f, 
        (color>>8&0xff)/255.0f,
        (color&0xff)/255.0f
    );
}
