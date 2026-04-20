#version 410 core

layout (location = 0) in vec2 l_Position;
layout (location = 1) in vec2 l_Uv;

out vec2 t_Uv;

void main() {
    t_Uv = l_Uv;
    gl_Position = vec4(vec2((l_Position * 2.0f) - 1.0f), 0.0f, 1.0f);
}