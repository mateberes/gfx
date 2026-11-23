#ifndef GI_SHARED_H
#define GI_SHARED_H

#include "gfx_shared.h"

GFX_BUFFER(Floats, float, values, 4)
GFX_BUFFER(Indices, uint, values, 4)

struct Mesh
{
    vec4 albedo;
    vec4 emissive;
    Floats positions;
    Floats normals;
    Floats uvs;
    Indices indices;
    uint albedo_index;
    float alpha_cutoff;
    float metallic;
    float roughness;
};
GFX_LAYOUT(Mesh, GFX_MEMBERS(albedo, emissive, positions, normals, uvs, indices, albedo_index, alpha_cutoff, metallic, roughness))
GFX_BUFFER(Meshes, Mesh, values, 16)

struct FrameData
{
    vec4 position;
    vec4 forward;
    vec4 right;
    vec4 up;
    vec4 prev_position;
    mat4 prev_view_projection;
    vec4 sun_direction;
    vec4 sun_color;
    vec4 pixel_size;
    uint frame_index;
};
GFX_LAYOUT(FrameData, GFX_MEMBERS(position, forward, right, up, prev_position, prev_view_projection, sun_direction, sun_color, pixel_size, frame_index))

struct TraceConstants
{
    Meshes meshes;
    uint tlas_index;
    uint direct_index;
    uint diffuse_albedo_index;
    uint specular_albedo_index;
    uint diffuse_index;
    uint specular_index;
    uint motion_index;
    uint normal_depth_index;
};
GFX_LAYOUT(TraceConstants, GFX_MEMBERS(meshes, tlas_index, direct_index, diffuse_albedo_index, specular_albedo_index, diffuse_index, specular_index, motion_index, normal_depth_index))

struct TemporalConstants
{
    uint diffuse_index;
    uint specular_index;
    uint specular_albedo_index;
    uint motion_index;
    uint normal_depth_index;
    uint prev_normal_depth_index;
    uint diffuse_history_index;
    uint prev_diffuse_history_index;
    uint specular_history_index;
    uint prev_specular_history_index;
    uint moments_history_index;
    uint prev_moments_history_index;
    uint diffuse_output_index;
    uint specular_output_index;
    uint history_limit;
    uint reset;
};
GFX_LAYOUT(TemporalConstants, GFX_MEMBERS(diffuse_index, specular_index, specular_albedo_index, motion_index, normal_depth_index, prev_normal_depth_index, diffuse_history_index, prev_diffuse_history_index, specular_history_index, prev_specular_history_index, moments_history_index, prev_moments_history_index, diffuse_output_index, specular_output_index, history_limit, reset))

struct SvgfConstants
{
    uint diffuse_input_index;
    uint specular_input_index;
    uint diffuse_output_index;
    uint specular_output_index;
    uint normal_depth_index;
    uint step_size;
};
GFX_LAYOUT(SvgfConstants, GFX_MEMBERS(diffuse_input_index, specular_input_index, diffuse_output_index, specular_output_index, normal_depth_index, step_size))

struct CompositeConstants
{
    uint direct_index;
    uint diffuse_albedo_index;
    uint specular_albedo_index;
    uint diffuse_index;
    uint specular_index;
    float exposure;
};
GFX_LAYOUT(CompositeConstants, GFX_MEMBERS(direct_index, diffuse_albedo_index, specular_albedo_index, diffuse_index, specular_index, exposure))

#ifndef __cplusplus
layout(set = 1, binding = 0) uniform Globals
{
    FrameData frame;
};

const vec3 LUMINANCE_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

float Luminance(vec3 color)
{
    return dot(color, LUMINANCE_WEIGHTS);
}

bool IsInside(ivec2 pixel, ivec2 size)
{
    return all(greaterThanEqual(pixel, ivec2(0))) && all(lessThan(pixel, size));
}

vec3 CameraRay(vec2 pixel_center)
{
    const vec2 ndc = pixel_center * frame.pixel_size.xy * 2.0 - 1.0;
    return normalize(frame.forward.xyz + ndc.x * frame.right.xyz - ndc.y * frame.up.xyz);
}

vec3 ReconstructPosition(ivec2 pixel, float distance)
{
    return frame.position.xyz + CameraRay(vec2(pixel) + 0.5) * distance;
}
#endif

#endif
