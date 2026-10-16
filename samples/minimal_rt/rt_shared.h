#ifndef RT_SHARED_H
#define RT_SHARED_H

#include "gfx_shared.h"

GFX_BUFFER(Floats, float, values, 4)
GFX_BUFFER(Indices, uint, values, 4)

struct Mesh
{
    vec4 albedo;
    Floats positions;
    Floats normals;
    Floats uvs;
    Indices indices;
    uint albedo_index;
    float alpha_cutoff;
};
GFX_LAYOUT(Mesh, GFX_MEMBERS(albedo, positions, normals, uvs, indices, albedo_index, alpha_cutoff))
GFX_BUFFER(Meshes, Mesh, values, 16)

struct FrameData
{
    vec4 position;
    vec4 forward;
    vec4 right;
    vec4 up;
    vec4 light_direction;
    float pixel_spread;
};
GFX_LAYOUT(FrameData, GFX_MEMBERS(position, forward, right, up, light_direction, pixel_spread))

struct TraceConstants
{
    Meshes meshes;
    uint tlas_index;
    uint image_index;
};
GFX_LAYOUT(TraceConstants, GFX_MEMBERS(meshes, tlas_index, image_index))

struct BlitConstants
{
    uint image_index;
};
GFX_LAYOUT(BlitConstants, GFX_MEMBERS(image_index))

#ifndef __cplusplus

uvec3 ReadTriangle(Mesh mesh, uint primitive)
{
    const uint first = primitive * 3;
    return uvec3(mesh.indices.values[first], mesh.indices.values[first + 1], mesh.indices.values[first + 2]);
}

vec3 ReadVec3(Floats values, uint index)
{
    return vec3(values.values[index * 3], values.values[index * 3 + 1], values.values[index * 3 + 2]);
}

vec2 ReadVec2(Floats values, uint index)
{
    return vec2(values.values[index * 2], values.values[index * 2 + 1]);
}

vec3 Weights(vec2 barycentrics)
{
    return vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
}

#endif

#endif
