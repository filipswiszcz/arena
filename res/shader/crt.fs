#version 410 core

in vec2 t_Uv;

out vec4 o_Color;

uniform sampler2D u_Texture;

uniform uint u_Lines;
uniform float u_Bleed;
uniform float u_Vignette;
uniform float u_Grain;

uniform float u_Time;

float random(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    float R = texture(u_Texture, t_Uv + vec2(u_Bleed, 0.0f)).r;
    float G = texture(u_Texture, t_Uv).g;
    float B = texture(u_Texture, t_Uv - vec2(u_Bleed, 0.0f)).b;

    float c_Scanline = (sin(t_Uv.y * u_Lines) * 0.15f) + 0.85f;
    vec3 c_Color = vec3(R * c_Scanline, G * c_Scanline, B * c_Scanline);

    float c_Vignette = smoothstep(0.8f, 0.3f, distance(t_Uv, vec2(0.5f, 0.5f)));

    o_Color = vec4(mix(c_Color, c_Color * c_Vignette, u_Vignette) + ((random(t_Uv + vec2(u_Time, -u_Time)) - 0.5f) * u_Grain), 1.0f);
}