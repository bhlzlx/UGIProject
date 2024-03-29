#version 450
#define VULKAN 100
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2   _position;
layout (location = 1) in vec2   _uv;

vec4 uint32ToVec4( uint val ) {
    return vec4( (val>>24)/255.0f, (val>>16&0xff)/255.0f, (val>>8&0xff)/255.0f, (val&0xff)/255.0f );
}

struct EffectImp {
    uint    colorMask;     // 颜色参数
    uint    effectColor;   // 效果颜色
    uint    type;          // 类型 : 加粗、描边、阴影、内发光、无
    uint    padding;       //
    // uint gray;          // 灰度
};

struct TransformImp {
    float col11; float col12; float col13;  // 矩阵变换 第一行
    float col21; float col22; float col23;  // 矩阵变换 第二行
};

// 一个批次最多能渲染8192个字
layout( set = 0, binding = 0 ) uniform Indices {
    ivec4 indices[512];
};

// 一个批次最多支持512种样式
layout( set = 0, binding = 1) uniform Effects {
    EffectImp effects[256];
};

// 一个批次最多支持512种变换
layout( set = 0, binding = 2) uniform Transforms {
    TransformImp transforms[256];
};

layout( set = 0, binding = 3) uniform Context {
    float contextWidth;
    float contextHeight;
};
//
layout( location = 0) out vec3  uvw_;
layout( location = 1) out vec4  color_;
layout( location = 2) out flat uint  type_;
layout( location = 3) out vec4  effectColor_;
// layout( location = 3) out float gray;

out gl_PerVertex  {
    vec4 gl_Position;   
};

void main() {
    uint tableIndex = gl_VertexIndex/4; // 四索引一个四边形
    uint indexerIndex = tableIndex/2;
    uint indexerInnerIndex = tableIndex%2;
    
    uint styleTrans = indices[indexerIndex][indexerInnerIndex*2];
    uint layerReserved = indices[indexerIndex][indexerInnerIndex*2+1];

    uint styleIndex = styleTrans&0xffff;
    uint transformIndex = styleTrans>>16;
    uint layerIndex = layerReserved&0xff;

    EffectImp effect = effects[styleIndex];
    TransformImp transform = transforms[transformIndex];

    // == transform
    vec3 col1 = vec3( transform.col11, transform.col12, transform.col13 );
    vec3 col2 = vec3( transform.col21, transform.col22, transform.col23 );
    float x = dot( vec3( _position.x, _position.y, 1.0f ), col1);
    float y = dot( vec3( _position.x, _position.y, 1.0f ), col2);
    // 转换NDC
    float ndcX = (x / contextWidth) * 2.0f - 1.0f;
    float ndcY = (y / contextHeight) * 2.0f - 1.0f;
    //
	gl_Position = vec4( ndcX, ndcY, 1.0f, 1.0f);
    // == effect output
    color_ = uint32ToVec4( effect.colorMask );
    effectColor_ = uint32ToVec4( effect.effectColor );
    type_ = effect.type&0xff;
    // == _uv output 
    float w = layerIndex;
	uvw_ = vec3( _uv, w );
}