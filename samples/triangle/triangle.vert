#version 460

const vec2 POSITIONS[3] = vec2[](vec2(0.0, -0.6), vec2(0.6, 0.6), vec2(-0.6, 0.6));
const vec3 COLORS[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

layout(location = 0) out vec3 out_color;

void main()
{
    gl_Position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);
    out_color = COLORS[gl_VertexIndex];
}
