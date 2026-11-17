#ifndef PT_SHARED_H
#define PT_SHARED_H

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
    vec4 sun_direction;
    vec4 sun_color;
    vec4 pixel_size;
    uint frame_index;
    uint sample_count;
};
GFX_LAYOUT(FrameData, GFX_MEMBERS(position, forward, right, up, sun_direction, sun_color, pixel_size, frame_index, sample_count))

struct TraceConstants
{
    Meshes meshes;
    uint tlas_index;
    uint radiance_index;
    uint history_limit;
};
GFX_LAYOUT(TraceConstants, GFX_MEMBERS(meshes, tlas_index, radiance_index, history_limit))

struct TonemapConstants
{
    uint radiance_index;
    float exposure;
};
GFX_LAYOUT(TonemapConstants, GFX_MEMBERS(radiance_index, exposure))

#endif
