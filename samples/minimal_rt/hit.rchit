#version 460
#extension GL_EXT_ray_tracing : require

#define GFX_BINDLESS_RAY_TRACING
#include "rt_shared.h"
#include "gfx_bindless.h"

layout(set = 1, binding = 0) uniform Globals
{
    FrameData frame;
};

layout(push_constant) uniform PushConstant
{
    TraceConstants pc;
};

layout(location = 0) rayPayloadInEXT vec3 color;
layout(location = 1) rayPayloadEXT bool shadowed;

hitAttributeEXT vec2 barycentrics;

const float AMBIENT = 0.2;
const float SHADOW_BIAS = 0.01;
const float MIN_COSINE = 0.1;
const float MIN_AREA = 1e-12;

vec4 Albedo(Mesh mesh, uvec3 triangle, vec3 weights, vec3 normal)
{
    const vec2 uv0 = ReadVec2(mesh.uvs, triangle.x);
    const vec2 uv1 = ReadVec2(mesh.uvs, triangle.y);
    const vec2 uv2 = ReadVec2(mesh.uvs, triangle.z);
    const vec2 uv = uv0 * weights.x + uv1 * weights.y + uv2 * weights.z;

    const vec3 p0 = gl_ObjectToWorldEXT * vec4(ReadVec3(mesh.positions, triangle.x), 1.0);
    const vec3 p1 = gl_ObjectToWorldEXT * vec4(ReadVec3(mesh.positions, triangle.y), 1.0);
    const vec3 p2 = gl_ObjectToWorldEXT * vec4(ReadVec3(mesh.positions, triangle.z), 1.0);
    const float world_area = max(length(cross(p1 - p0, p2 - p0)), MIN_AREA);

    const vec2 size = vec2(textureSize(textures_2d[nonuniformEXT(mesh.albedo_index)], 0));
    const vec2 edge1 = (uv1 - uv0) * size;
    const vec2 edge2 = (uv2 - uv0) * size;
    const float texel_area = max(abs(edge1.x * edge2.y - edge1.y * edge2.x), MIN_AREA);

    const float cone_width = gl_HitTEXT * frame.pixel_spread;
    const float cosine = max(abs(dot(normal, gl_WorldRayDirectionEXT)), MIN_COSINE);
    const float lod = 0.5 * log2(texel_area / world_area) + log2(cone_width / cosine);
    return mesh.albedo * textureLod(textures_2d[nonuniformEXT(mesh.albedo_index)], uv, lod);
}

void main()
{
    Mesh mesh = pc.meshes.values[gl_InstanceCustomIndexEXT];
    const uvec3 triangle = ReadTriangle(mesh, gl_PrimitiveID);
    const vec3 weights = Weights(barycentrics);

    const vec3 object_normal = ReadVec3(mesh.normals, triangle.x) * weights.x + ReadVec3(mesh.normals, triangle.y) * weights.y + ReadVec3(mesh.normals, triangle.z) * weights.z;
    vec3 normal = normalize(vec3(object_normal * gl_WorldToObjectEXT));
    if (dot(normal, gl_WorldRayDirectionEXT) > 0.0)
        normal = -normal;

    const vec3 albedo = Albedo(mesh, triangle, weights, normal).rgb;

    const vec3 light = frame.light_direction.xyz;
    const vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + normal * SHADOW_BIAS;
    float diffuse = max(dot(normal, light), 0.0);
    if (diffuse > 0.0)
    {
        shadowed = true;
        const uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
        traceRayEXT(tlas[pc.tlas_index], flags, 0xFF, 0, 0, 1, position, 0.0, light, 10000.0, 1);
        if (shadowed)
            diffuse = 0.0;
    }

    color = albedo * (AMBIENT + (1.0 - AMBIENT) * diffuse);
}
