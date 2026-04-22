#version 410 core

in vec2 t_Uv;

out vec4 o_Color;

uniform sampler2D u_Texture;
uniform vec3 u_Color;

void main() {
    if (texture(u_Texture, t_Uv).r < 0.5f) discard;
    o_Color = vec4(u_Color, 1.0f);
}