#version 410 core

layout (location = 0) in vec2 l_Position;
layout (location = 1) in vec2 l_Uv;

out vec2 t_Uv;
out vec4 t_Bound;

uniform mat4 u_Projection;
uniform mat4 u_Model;

uniform vec2 u_Scale;
uniform vec2 u_Offset;
uniform int u_Flip;

void main() {
    gl_Position = u_Projection * u_Model * vec4(l_Position, 0.0f, 1.0f);

    vec2 c_Uv = l_Uv;
    if (u_Flip == 1) {
        c_Uv.x = 1.0f - c_Uv.x;
    }

    t_Uv = (c_Uv * u_Scale) + u_Offset;
    t_Bound = vec4(u_Offset, u_Offset + u_Scale);
}