#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 color;

void main()
{
    color = mix(vec3(0.9, 0.95, 1.0), vec3(0.3, 0.5, 0.9), clamp(gl_WorldRayDirectionEXT.y, 0.0, 1.0));
}
