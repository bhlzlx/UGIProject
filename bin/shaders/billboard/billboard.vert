#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

out gl_PerVertex {
    vec4 gl_Position;   
};

layout( set = 0, binding = 0 ) uniform Argument1 {
    vec4 col1;
    vec4 col2;
};