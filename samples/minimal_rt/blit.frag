#version 460
#include "rt_shared.h"
#include "gfx_bindless.h"

layout(push_constant) uniform PushConstant
{
    BlitConstants pc;
};

layout(location = 0) out vec4 out_color;

const float GAMMA = 1.5;

void main()
{
    const vec4 color = texelFetch(textures_2d[pc.image_index], ivec2(gl_FragCoord.xy), 0);
    out_color = vec4(pow(color.rgb, vec3(1.0 / GAMMA)), color.a);
}
