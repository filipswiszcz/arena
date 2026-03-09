#ifndef __LIBMATH__
#define __LIBMATH__

#define PI 3.14159265358979323846

// VEC2

#define vec2(x, y) ((vec2_t) {x, y})
#define vec2_add(a, b) (m_vec2_add(a, b))
#define vec2_sub(a, b) (m_vec2_sub(a, b))
#define vec2_mul(a, b) (m_vec2_mul(a, b))
#define vec2_dot(a, b) (m_vec2_dot(a, b))
#define vec2_cross(a, b) (m_vec2_cross(a, b))

typedef struct {
    float x, y;
} vec2_t;

static inline vec2_t m_vec2_add(vec2_t a, vec2_t b) {
    return (vec2_t) {a.x + b.x, a.y + b.y};
}

static inline vec2_t m_vec2_sub(vec2_t a, vec2_t b) {
    return (vec2_t) {a.x - b.x, a.y - b.y};
}

static inline vec2_t m_vec2_mul(vec2_t v, float s) {
    return (vec2_t) {(v).x * (s), (v).y * (s)};
}

static inline float m_vec2_dot(vec2_t a, vec2_t b) {
    return (a.x * b.x + a.y * b.y);
}

static inline float m_vec2_cross(vec2_t a, vec2_t b) {
    return (a.x * b.y - a.y * b.x);
}

// VEC3

#define vec3(x, y, z) ((vec3_t) {x, y, z})
#define vec3_add(a, b) (m_vec3_add(a, b))
#define vec3_sub(a, b) (m_vec3_sub(a, b))
#define vec3_mul(a, b) (m_vec3_mul(a, b))
#define vec3_dot(a, b) (m_vec3_dot(a, b))
#define vec3_cross(a, b) (m_vec3_cross(a, b))

typedef struct {
    float x, y, z;
} vec3_t;

static inline vec3_t m_vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t) {a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline vec3_t m_vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t) {a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline vec3_t m_vec3_mul(vec3_t v, float s) {
    return (vec3_t) {(v).x * (s), (v).y * (s), (v).z * (s)};
}

static inline float m_vec3_dot(vec3_t a, vec3_t b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

static inline vec3_t m_vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t) {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// VEC4

#define vec4(x, y, z, w) ((vec4_t) {x, y, z, w})
#define vec4_add(a, b) (m_vec4_add(a, b))
#define vec4_sub(a, b) (m_vec4_sub(a, b))
#define vec4_mul(a, b) (m_vec4_mul(a, b))
#define vec4_dot(a, b) (m_vec4_dot(a, b))

typedef struct {
    float x, y, z, w;
} vec4_t;

static inline vec4_t m_vec4_add(vec4_t a, vec4_t b) {
    return (vec4_t) {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static inline vec4_t m_vec4_sub(vec4_t a, vec4_t b) {
    return (vec4_t) {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static inline vec4_t m_vec4_mul(vec4_t v, float s) {
    return (vec4_t) {(v).x * (s), (v).y * (s), (v).z * (s), (v).w * (s)};
}

static inline float m_vec4_dot(vec4_t a, vec4_t b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
}

// MAT2

#define mat2(v) ((mat2_t) {.m = { \
    {v, 0}, \
    {0, v} \
}})
#define mat2_add(a, b) (m_mat2_add(a, b))

typedef struct {
    float m[2][2];
} mat2_t;

static inline mat2_t m_mat2_add(mat2_t a, mat2_t b) {
    return (mat2_t) {.m = {
        {a.m[0][0] + b.m[0][0], a.m[0][1] + b.m[0][1]},
        {a.m[1][0] + b.m[1][0], a.m[1][1] + b.m[1][1]}
    }};
}

// MAT3

#define mat3(v) ((mat3_t) {.m = { \
    {v, 0, 0}, \
    {0, v, 0}, \
    {0, 0, v} \
}})
#define mat3_add(a, b) (m_mat3_add(a, b))

typedef struct {
    float m[3][3];
} mat3_t;

static inline mat3_t m_mat3_add(mat3_t a, mat3_t b) {
    return (mat3_t) {.m = {
        {a.m[0][0] + b.m[0][0], a.m[0][1] + b.m[0][1], a.m[0][2] + b.m[0][2]},
        {a.m[1][0] + b.m[1][0], a.m[1][1] + b.m[1][1], a.m[1][2] + b.m[1][2]},
        {a.m[2][0] + b.m[2][0], a.m[2][1] + b.m[2][1], a.m[2][2] + b.m[2][2]}
    }};
}

// MAT4

#define mat4(v) ((mat4_t) {.m = { \
    {v, 0, 0, 0}, \
    {0, v, 0, 0}, \
    {0, 0, v, 0}, \
    {0, 0, 0, v} \
}})
#define mat4_add(a, b) (m_mat4_add(a, b))
#define mat3_ortho(l, r, t, b, zn, zf) (m_mat3_ortho(l, r, t, b, zn, zf))

typedef struct {
    float m[4][4];
} mat4_t;

static inline mat4_t m_mat4_add(mat4_t a, mat4_t b) {
    return (mat4_t) {.m = {
        {a.m[0][0] + b.m[0][0], a.m[0][1] + b.m[0][1], a.m[0][2] + b.m[0][2], a.m[0][3] + b.m[0][3]},
        {a.m[1][0] + b.m[1][0], a.m[1][1] + b.m[1][1], a.m[1][2] + b.m[1][2], a.m[1][3] + b.m[1][3]},
        {a.m[2][0] + b.m[2][0], a.m[2][1] + b.m[2][1], a.m[2][2] + b.m[2][2], a.m[2][3] + b.m[2][3]},
        {a.m[3][0] + b.m[3][0], a.m[3][1] + b.m[3][1], a.m[3][2] + b.m[3][2], a.m[3][3] + b.m[3][3]}
    }};
}

static inline mat4_t m_mat4_ortho(float l, float r, float t, float b, float zn, float zf) {
    mat4_t m = mat4(0.0f);
    m.m[0][0] = 2.0f / (r - l);
    m.m[1][1] = 2.0f / (t - b);
    m.m[2][2] = -2.0f / (zf / zn);
    m.m[3][0] = -(r + l) / (r - l);
    m.m[3][1] = -(t + b) / (t - b);
    m.m[3][2] = -(zf + zn) / (zf - zn);
    m.m[3][3] = 1.0f;
    return m;
}

// QUAT

#define quat(x, y, z, w) {(quat_t) {x, y, z, w}}

typedef struct {
    float x, y, z, w;
} quat_t;

#endif // !__LIBMATH__