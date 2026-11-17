#version 460
#include "pt_shared.h"
#include "gfx_bindless.h"

layout(push_constant) uniform PushConstant
{
    TonemapConstants pc;
};

layout(location = 0) out vec4 out_color;

vec3 TonemapAces(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
    const vec3 color = texelFetch(textures_2d[pc.radiance_index], ivec2(gl_FragCoord.xy), 0).rgb;
    out_color = vec4(TonemapAces(color * pc.exposure), 1.0);
}
