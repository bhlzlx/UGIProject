#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in uint packed_color;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec2 frag_uv;
layout (location = 1) out vec4 frag_color;

out gl_PerVertex  {
    vec4 gl_Position;
};

struct 

layout( set = 0, binding = 0 ) uniform args {
	mat4 vp;
};

void main() {
	gl_Position = vec4(position,1.0f) * vp;
	frag_uv = uv;
    float a = float(((packed_color >> 24) & 0xff)) / 255.0f;
    float r = float(((packed_color >> 16) & 0xff)) / 255.0f;
    float g = float(((packed_color >> 8) & 0xff)) / 255.0f;
    float b = float(packed_color & 0xff) / 255.0f;
    frag_color = vec4(r,g,b,a);
}
