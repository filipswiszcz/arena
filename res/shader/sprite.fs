#version 410 core

in vec2 t_Uv;
in vec4 t_Bound;

out vec4 o_Color;

uniform sampler2D u_Texture;

uniform vec3 u_Color;

uniform float u_Dissolve;
uniform float u_GlitchTime;
uniform float u_GlitchIntensity;

float random(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {    
    vec2 c_Uv = t_Uv;

    float c_TimeStep = floor(u_GlitchTime * 15.0f);
    float c_Slice = floor(c_Uv.y * 30.0f);
    float c_Noise = random(vec2(c_TimeStep, c_Slice));

    if (c_Noise < u_GlitchIntensity * 0.4f) {
        c_Uv.x += (random(vec2(c_TimeStep, c_Slice)) - 0.5f) * 0.1f * u_GlitchIntensity;
    }

    float c_Split = 0.015 * u_GlitchIntensity;

    vec2 c_UvR = clamp(c_Uv + vec2(c_Split, 0.0f), (t_Bound.xy), (t_Bound.zw));
    vec2 c_UvB = clamp(c_Uv - vec2(c_Split, 0.0f), (t_Bound.xy), (t_Bound.zw));

    float R = texture(u_Texture, c_UvR).r;
    vec4 S = texture(u_Texture, c_Uv);
    float B = texture(u_Texture, c_UvB).b;

    vec4 c_Color = vec4(vec3(R, S.g, B) * u_Color, S.a);

    if (u_Dissolve > 0.0f) {
        float c_Dissolve = random(c_Uv * 32.0f + vec2(u_GlitchTime));
        if (c_Dissolve < u_Dissolve) discard;
        if (c_Dissolve < u_Dissolve + 0.25f) c_Color.rgb = vec3(0.0f, 1.0f, 1.0f);
    }

    o_Color = c_Color;

    // o_Color = vec4(vec3(R, S.g, B) * u_Color, S.a);
}