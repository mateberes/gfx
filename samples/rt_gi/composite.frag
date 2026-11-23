#version 460
#include "gi_shared.h"
#include "gfx_bindless.h"

layout(push_constant) uniform PushConstant
{
    CompositeConstants pc;
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
    const ivec2 pixel = ivec2(gl_FragCoord.xy);
    const vec3 direct = texelFetch(textures_2d[pc.direct_index], pixel, 0).rgb;
    const vec3 diffuse_albedo = texelFetch(textures_2d[pc.diffuse_albedo_index], pixel, 0).rgb;
    const vec3 specular_albedo = texelFetch(textures_2d[pc.specular_albedo_index], pixel, 0).rgb;
    const vec3 diffuse = texelFetch(textures_2d[pc.diffuse_index], pixel, 0).rgb;
    const vec3 specular = texelFetch(textures_2d[pc.specular_index], pixel, 0).rgb;
    out_color = vec4(TonemapAces((direct + diffuse_albedo * diffuse + specular_albedo * specular) * pc.exposure), 1.0);
}
