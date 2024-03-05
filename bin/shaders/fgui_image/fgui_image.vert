#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in uint packed_color;
layout (location = 2) in vec2 uv;
layout (location = 3) in uint propIndex;

layout (location = 0) out vec2 frag_uv;
layout (location = 1) out vec4 frag_color;
layout (location = 2) out vec4 frag_props;


out gl_PerVertex  {
    vec4 gl_Position;
};

// 65536 / sizeof(instance_data_t) = 682

struct instance_data_t {
    mat4    transfrom;
    vec4    color;
    vec4    props; // gray， hdr
};

layout(set = 0, binding = 0) uniform args {
    instance_data_t image_datas[512];
};

void main() {
    uint idx = propIndex;
	gl_Position = image_datas[idx].transfrom * vec4(position,1.0f);
	frag_uv = uv;
    float a = float(((packed_color >> 24) & 0xff)) / 255.0f;
    float r = float(((packed_color >> 16) & 0xff)) / 255.0f;
    float g = float(((packed_color >> 8) & 0xff)) / 255.0f;
    float b = float(packed_color & 0xff) / 255.0f;
    frag_color = vec4(r,g,b,a);
    frag_props = image_datas[idx].props;
}
