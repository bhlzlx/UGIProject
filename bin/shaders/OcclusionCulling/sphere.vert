#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;

out gl_PerVertex  {
    vec4 gl_Position;
};

layout (location = 0) out vec4 color;

layout( set = 0, binding = 0 ) uniform Argument {
	mat4 model[2];
    mat4 view;
    mat4 projection;
};

void main() {
    vec4 colors [2] = {
        vec4(1.0,1.0,1.0,1.0),
        vec4(1.0,0.0,1.0,1.0),
    };
	gl_Position = vec4(position, 1.0f) * model[gl_InstanceIndex]* view * projection;
    color = colors[gl_InstanceIndex];
}
