#version 410 core

layout (location = 0) in vec2 l_Position;
layout (location = 1) in vec2 l_Uv;

out vec2 t_Uv;

uniform mat4 u_Projection;
uniform mat4 u_Model;

uniform vec2 u_Scale;
uniform vec2 u_Offset;

void main() {
    gl_Position = u_Projection * u_Model * vec4(l_Position, 0.0f, 1.0f);
    t_Uv = (l_Uv * u_Scale) + u_Offset;
}