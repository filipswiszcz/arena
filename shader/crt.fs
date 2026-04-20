#version 410 core

in vec2 t_Uv;

out vec4 o_Color;

uniform sampler2D u_Texture;

void main() {
    o_Color = texture(u_Texture, t_Uv);
}