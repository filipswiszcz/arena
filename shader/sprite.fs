#version 410 core

in vec2 t_Uv;

out vec4 o_Color;

uniform sampler2D u_Texture;
uniform vec3 u_Color;

void main() {
    o_Color = vec4(u_Color, 1.0f) * texture(u_Texture, t_Uv);
}