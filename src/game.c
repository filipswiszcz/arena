#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libmath/math.h>
#include <libmem/mem.h>
#include <libml/ml.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #define GLFW_INCLUDE_GLCOREARB
    #include <GLFW/glfw3.h>
#else
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_NAME "BattleArena 2D (Build v0.1.4)"

#define LOG(_m, ...) ((void) 0)
#define ASSERT(_e) ((_e) ? 1 : (LOG("%s,%d: Assertion '%s' failed\n", __FILE__, __LINE__, #_e), 0))

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef vec2_t v2;
typedef vec3_t v3;
typedef vec4_t v4;
typedef mat2_t m2;
typedef mat3_t m3;
typedef mat4_t m4;

#ifndef STATIC_ASSERT
    #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
        #define STATIC_ASSERT(_e, _m) _Static_assert(_e, _m)
    #elif defined(_MSC_VER)
        #define STATIC_ASSERT(_e, _m) static_assert(_e, _m)
    #else
        #define STATIC_ASSERT(_e, _m) \
            typedef char static_assert_##__LINE__[(_e) ? 1 : -1]
    #endif
#endif

STATIC_ASSERT(sizeof(f32) == 4, "Float must be exactly 4 bytes");
STATIC_ASSERT(sizeof(f64) == 8, "Float must be exactly 8 bytes");
STATIC_ASSERT(sizeof(v2) == 8, "Vec2 must be exactly 8 bytes");
STATIC_ASSERT(sizeof(v3) == 12, "Vec3 must be exactly 12 bytes");
STATIC_ASSERT(sizeof(v4) == 16, "Vec4 must be exactly 16 bytes");
STATIC_ASSERT(sizeof(m2) == 16, "Mat2 must be exactly 16 bytes");
STATIC_ASSERT(sizeof(m3) == 36, "Mat3 must be exactly 36 bytes");
STATIC_ASSERT(sizeof(m4) == 64, "Mat4 must be exactly 64 bytes");

// UTIL

// static inline f32 random(void) {
//     static i8 init = 0;
//     if (!init) {srand((unsigned) time(NULL)); init = 1;}
//     return (f32) rand() / ((f32) RAND_MAX + 1.0f);
// }

// SHADER

#define SHADER_MAX_SIZE 8192

typedef enum {
    SHADER_STATUS_SUCCESS = 0,
    SHADER_STATUS_FILE_NOT_FOUND,
    SHADER_STATUS_COMPILATION_FAILED,
    SHADER_STATUS_LINK_FAILED,
    SHADER_STATUS_CREATION_FAILED,
    SHADER_STATUS_INITIALIZATION_FAILED,
    SHADER_STATUS_INVALID_PARAMETER
} shader_status_t;

typedef struct {
    u32 ids[2];
    u32 program;
} shader_t;

shader_status_t _shader_read(char *buffer, const char *path) {
    if (!ASSERT(buffer != NULL)) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }
    if (!ASSERT(path != NULL)) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    FILE *file = fopen((const char*) path, "rb");
    if (file == NULL) {
        return SHADER_STATUS_FILE_NOT_FOUND;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    long size = ftell(file);
    if (size < 0 || (size_t) size >= SHADER_MAX_SIZE) {
        fclose(file);
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    rewind(file);

    size_t code = fread(buffer, 1, (size_t) size, file);
    if (code != (size_t) size) {
        fclose(file);
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    buffer[size] = '\0';
    fclose(file);

    return SHADER_STATUS_SUCCESS;
}

shader_status_t _shader_compile(u32 *id, const uint32_t type, char *code) {
    if (!ASSERT(id != NULL)) {
        return SHADER_STATUS_CREATION_FAILED;
    }
    if (!ASSERT(code != NULL)) {
        return SHADER_STATUS_CREATION_FAILED;
    }

    *id = glCreateShader(type);
    if (*id == 0) {
        return SHADER_STATUS_CREATION_FAILED;
    }

    const char *source = code;
    glShaderSource(*id, 1, (const char**) &source, NULL);
    
    glCompileShader(*id);

    i32 params;
    glGetShaderiv(*id, GL_COMPILE_STATUS, &params);
    if (params == 0) {
        // print err in debug

        glDeleteShader(*id);
        *id = 0;

        return SHADER_STATUS_COMPILATION_FAILED;
    }

    return SHADER_STATUS_SUCCESS;
}

shader_status_t _shader_link(const u32 program) {
    if (!ASSERT(program != 0)) {
        return SHADER_STATUS_LINK_FAILED;
    }

    glLinkProgram(program);

    i32 params;
    glGetProgramiv(program, GL_LINK_STATUS, &params);
    if (params == 0) {
        // print err in debug
        return SHADER_STATUS_LINK_FAILED;
    }

    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_init(shader_t *shader, const char *paths[2]) {
    if (!ASSERT(shader != NULL)) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }
    if (!ASSERT(paths != NULL) || !ASSERT(paths[0] != NULL) || !ASSERT(paths[1] != NULL)) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    char codes[2][SHADER_MAX_SIZE];

    if (_shader_read(codes[0], paths[0]) != SHADER_STATUS_SUCCESS) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }
    if (_shader_read(codes[1], paths[1]) != SHADER_STATUS_SUCCESS) {
        return SHADER_STATUS_INITIALIZATION_FAILED;
    }

    shader_status_t vscs = _shader_compile(&shader->ids[0], GL_VERTEX_SHADER, codes[0]);
    if (vscs != SHADER_STATUS_SUCCESS) {
        return vscs;
    }
    shader_status_t fscs = _shader_compile(&shader->ids[1], GL_FRAGMENT_SHADER, codes[1]);
    if (fscs != SHADER_STATUS_SUCCESS) {
        return fscs;
    }

    shader->program = glCreateProgram();
    if (shader->program == 0) {
        return SHADER_STATUS_CREATION_FAILED;
    }

    glAttachShader(shader->program, shader->ids[0]);
    glAttachShader(shader->program, shader->ids[1]);

    return _shader_link(shader->program);
}

shader_status_t shader_use(const shader_t *shader) {
    if (!ASSERT(shader != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUseProgram(shader->program);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_destroy(shader_t *shader) {
    if (!ASSERT(shader != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }

    glDeleteShader(shader->ids[0]);
    glDeleteShader(shader->ids[1]);
    glDeleteProgram(shader->program);

    shader->ids[0] = 0;
    shader->ids[1] = 0;
    shader->program = 0;

    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_int(const shader_t *shader, const char *name, const i32 val) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniform1i(glGetUniformLocation(shader->program, (const char*) name), val);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_uint(const shader_t *shader, const char *name, const u32 val) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniform1ui(glGetUniformLocation(shader->program, name), val);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_float(const shader_t *shader, const char *name, const f32 val) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniform1f(glGetUniformLocation(shader->program, name), val);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_vec2(const shader_t *shader, const char *name, const v2 vec) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniform2f(glGetUniformLocation(shader->program, name), vec.x, vec.y);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_vec3(const shader_t *shader, const char *name, const v3 vec) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniform3f(glGetUniformLocation(shader->program, name), vec.x, vec.y, vec.z);
    return SHADER_STATUS_SUCCESS;
}

shader_status_t shader_set_mat4(const shader_t *shader, const char *name, const m4 mat) {
    if (!ASSERT(shader != NULL) || !ASSERT(name != NULL)) {
        return SHADER_STATUS_INVALID_PARAMETER;
    }
    glUniformMatrix4fv(glGetUniformLocation(shader->program, name), 1, GL_FALSE, &mat.m[0][0]);
    return SHADER_STATUS_SUCCESS;
}

// TEXTURE

typedef enum {
    TEXTURE_STATUS_SUCCESS = 0,
    TEXTURE_STATUS_FILE_NOT_FOUND,
    TEXTURE_STATUS_INVALID_PARAMETER
} texture_status_t;

typedef struct {
    u32 id;
    i32 width, height;
    i32 format;
} texture_t;

texture_status_t texture_init(texture_t *texture, char *path) {
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(1);
    
    i32 channels;
    unsigned char *pixels = stbi_load(path, &texture->width, &texture->height, &channels, 0);
    if (!ASSERT(pixels != NULL)) {
        return TEXTURE_STATUS_FILE_NOT_FOUND;
    }

    switch (channels) {
        case 1: {texture->format = GL_RED; break;}
        case 3: {texture->format = GL_RGB; break;}
        case 4: {texture->format = GL_RGBA; break;}
    }

    glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width, texture->height, 0, texture->format, GL_UNSIGNED_BYTE, pixels);
    // ?

    stbi_image_free(pixels);
}

texture_status_t texture_bind(texture_t *texture) {
    if (!ASSERT(texture != NULL)) {
        return TEXTURE_STATUS_INVALID_PARAMETER;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture->id);

    return TEXTURE_STATUS_SUCCESS;
}

texture_status_t texture_destroy(texture_t *texture) { // it should destroy whole array instead of one (i assume)
    if (!ASSERT(texture != NULL)) {
        return TEXTURE_STATUS_INVALID_PARAMETER;
    }

    glDeleteTextures(1, &texture->id);

    return TEXTURE_STATUS_SUCCESS;
}

// SPRITE

typedef struct {
    char **name;

    texture_t *texture;

    struct {
        v2 scale, offset;
    } uv;

    v2 pos, size; // local pos
    f32 rot;

    v3 color;

    u8 zorder;
    u8 flip;
} sprite_t;

void sprite_init(sprite_t *sprite, texture_t *texture, v2 scale, v2 offset, v2 pos, v2 size, f32 rot, v3 color, u8 zorder, u8 flip) {
    sprite->name = NULL;
    sprite->texture = texture;
    sprite->uv.scale = scale;
    sprite->uv.offset = offset;
    sprite->pos = pos;
    sprite->size = size;
    sprite->rot = rot;
    sprite->color = color;
    sprite->zorder = zorder;
    sprite->flip = flip;
}

// TEXT

#define FONT_WIDTH 6
#define FONT_HEIGHT 6

static char GLYPHS[128][FONT_WIDTH][FONT_HEIGHT] = {
    ['A'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
    },
    // ['B'] = {
    //     {1, 0, 0, 0, 0},
    //     {1, 1, 1, 0, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 1, 1, 0, 0},
    // },
    // ['C'] = {
    //     {0, 0, 0, 0, 0},
    //     {0, 1, 1, 0, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 0, 0},
    //     {1, 0, 0, 1, 0},
    //     {0, 1, 1, 0, 0},
    // },
    ['D'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
    },
    ['E'] = {
        {1, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
    },
    ['F'] = {
        {1, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    ['G'] = {
        {0, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {1, 0, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0},
    },
    // ['H'] = {
    //     {1, 0, 0, 0, 0},
    //     {1, 1, 1, 0, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    // },
    ['I'] = {
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
    },
    ['J'] = {
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
    },
    ['K'] = {
        {1, 0, 0, 1, 0},
        {1, 0, 1, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 1, 0, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 0, 0, 1, 0},
    },
    ['L'] = {
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
    },
    ['M'] = {
        {1, 0, 0, 0, 1},
        {1, 1, 0, 1, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
    },
    ['N'] = {
        {1, 0, 0, 1, 0},
        {1, 1, 0, 1, 0},
        {1, 1, 0, 1, 0},
        {1, 0, 1, 1, 0},
        {1, 0, 1, 1, 0},
        {1, 0, 0, 1, 0},
    },
    ['O'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['P'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    // ['Q'] = {
    //     {0, 1, 1, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {1, 0, 0, 1, 0},
    //     {0, 1, 1, 1, 0},
    //     {0, 0, 0, 1, 0},
    //     {0, 0, 0, 1, 0},
    // },
    ['R'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
    },
    ['S'] = {
        {0, 1, 1, 1, 1},
        {1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 0},
    },
    ['T'] = {
        {1, 1, 1, 1, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
    },
    ['U'] = {
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['V'] = {
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 0, 1, 0},
        {0, 1, 0, 1, 0},
        {0, 0, 1, 0, 0},
    },
    ['W'] = {
        {1, 0, 0, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 1, 0, 1, 1},
    },
    // ['X'] = {
    //     {0, 0, 0, 0, 0},
    //     {1, 0, 1, 0, 0},
    //     {1, 0, 1, 0, 0},
    //     {0, 1, 0, 0, 0},
    //     {1, 0, 1, 0, 0},
    //     {1, 0, 1, 0, 0},
    // },
    ['Y'] = {
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {0, 1, 1, 0, 0},
        {0, 1, 1, 0, 0},
    },
    // ['Z'] = {
    //     {0, 0, 0, 0, 0},
    //     {1, 1, 1, 1, 0},
    //     {0, 0, 0, 1, 0},
    //     {0, 1, 1, 0, 0},
    //     {1, 0, 0, 0, 0},
    //     {1, 1, 1, 1, 0},
    // },
    ['0'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['1'] = {
        {0, 0, 1, 0, 0},
        {0, 1, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0},
    },
    ['2'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 1, 0},
    },
    ['3'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['4'] = {
        {0, 0, 1, 1, 0},
        {0, 1, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 1, 0},
    },
    ['5'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['6'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['7'] = {
        {1, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
    },
    ['8'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},

    },
    ['9'] = {
        {0, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 1, 0, 0},
    },
    ['['] = {
        {0, 0, 1, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 1, 0},
    },
    [']'] = {
        {1, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {1, 1, 0, 0, 0},
    },
    ['/'] = {
        {0, 0, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 1, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    [':'] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
    },
    ['\''] = {
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
    },
    ['\"'] = {
        {0, 1, 0, 1, 0},
        {0, 1, 0, 1, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
    }
};

// RENDERER

#define RENDERER_COMMAND_ARRAY_SIZE 64
#define RENDERER_TEXT_LENGTH 64

typedef enum {
    COMMAND_TYPE_SPRITE,
    COMMAND_TYPE_TEXT
} command_type_t;

typedef struct command {
    command_type_t type;

    union {
        struct {
            texture_t *texture;
            struct {
                v2 scale, offset;
            } uv;
        } sprite;

        struct {
            char content[64];
            f32 scale;
        } text;
    } data;

    v2 pos, size;
    f32 rot;

    v3 color;

    // temp
    f32 dissolve;
    // temp

    u8 zorder;
    u8 flip;
} command_t;

typedef struct renderer {
    struct {
        command_t *commands;
        u32 counter;
    } frame;
    
    shader_t *shader;
    u32 vao, vbo;

    struct {
        shader_t *shader;
        texture_t texture;
        u32 vao, vbo;
    } text;

    struct {
        shader_t *shaders[2]; // crt, glitch
        texture_t textures[2];
        u32 fbos[2];
    } postprocessing;

    f64 time;
} renderer_t;

void _renderer_text_init(renderer_t *renderer) {
    u8 bitmap[(FONT_WIDTH * 16) * (FONT_HEIGHT * 8)];
    memset(bitmap, 0, sizeof(bitmap));
    
    for (u32 i = 0; i < 128; i++) {
        u32 cpx = (i % 16) * FONT_WIDTH;
        u32 cpy = (i / 16) * FONT_HEIGHT;
        
        for (u32 j = 0; j < FONT_HEIGHT; j++) {
            for (u32 k = 0; k < FONT_WIDTH; k++) {
                if (GLYPHS[i][j][k]) {
                    u32 px = cpx + k;
                    u32 py = cpy + j;
                    u32 mrk = (py * (FONT_WIDTH * 16)) + px;
                    bitmap[mrk] = 255;
                }
            }
        }
    }

    glGenTextures(1, &renderer->text.texture.id);

    glBindTexture(GL_TEXTURE_2D, renderer->text.texture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (FONT_WIDTH * 16), (FONT_HEIGHT * 8), 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

    glGenVertexArrays(1, &renderer->text.vao);
    glGenBuffers(1, &renderer->text.vbo);

    glBindVertexArray(renderer->text.vao);

    glBindBuffer(GL_ARRAY_BUFFER, renderer->text.vbo);
    glBufferData(GL_ARRAY_BUFFER, 256 * 6 * 4 * sizeof(f32), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(v4), (void*) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(v4), (void*) (sizeof(v2)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void _renderer_postprocess_init(renderer_t *renderer) {
    for (u32 i = 0; i < 2; i++) {
        glGenFramebuffers(1, &renderer->postprocessing.fbos[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->postprocessing.fbos[i]);

        glGenTextures(1, &renderer->postprocessing.textures[i].id);
        glBindTexture(GL_TEXTURE_2D, renderer->postprocessing.textures[i].id);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); // TODO append that data to texture_t;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->postprocessing.textures[i].id, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { // swap to ASSERT?
            printf("FRAMEBUFFER_INIT_ERROR\n");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderer_init(renderer_t *renderer, shader_t *shaders) {
    renderer->shader = &shaders[0]; // sprite
    renderer->text.shader = &shaders[1]; // text
    renderer->postprocessing.shaders[0] = &shaders[2]; // crt
    // renderer->postprocessing.shaders[1] = &shaders[3]; // glitch

    v4 vertices[] = {
        vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 1.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), 
        vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 0.0f, 1.0f, 0.0f)
    };
    
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);

    glBindVertexArray(renderer->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(v4), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(v4), (void*) (2 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // TEXT
    _renderer_text_init(renderer);

    // POST-PROCESSING
    _renderer_postprocess_init(renderer);

    // PROJECTION
    m4 projection = mat4_ortho(0.0f, (f32) WINDOW_WIDTH, (f32) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    shader_use(renderer->shader);
    shader_set_mat4(renderer->shader, "u_Projection", projection);

    shader_use(renderer->text.shader);
    shader_set_mat4(renderer->text.shader, "u_Projection", projection);

}

void renderer_frame_command_push(renderer_t *renderer, command_t command) {
    if (renderer->frame.counter < RENDERER_COMMAND_ARRAY_SIZE) {
        renderer->frame.commands[renderer->frame.counter++] = command;
    }
}

void renderer_frame_clear(renderer_t *renderer) { // i have to find out, if it could work that way
    renderer->frame.counter = 0;
}

void _renderer_sprite_draw(renderer_t *renderer, texture_t *texture, v4 uv, v4 trans, f32 rot, v3 color, f32 dissolve, u8 flip) {
    m4 model = mat4(1.0f);
    model = mat4_trans(model, vec3(trans.x, trans.y, 0.0f));
    model = mat4_trans(model, vec3(trans.z * 0.5f, trans.w * 0.5f, 0.0f)); // what does it do? cant remember
    model = mat4_trans(model, vec3(trans.z * (-0.5f), trans.w * (-0.5f), 0.0f)); // same here?
    model = mat4_scale(model, vec3(trans.z, trans.w, 1.0f));
    // rotation??

    shader_use(renderer->shader); // here?

    shader_set_mat4(renderer->shader, "u_Model", model);

    shader_set_vec2(renderer->shader, "u_Scale", vec2(uv.x, uv.y));
    shader_set_vec2(renderer->shader, "u_Offset", vec2(uv.z, uv.w));

    shader_set_int(renderer->shader, "u_Flip", flip);

    shader_set_vec3(renderer->shader, "u_Color", color);

    shader_set_float(renderer->shader, "u_Dissolve", dissolve);
    shader_set_float(renderer->shader, "u_GlitchTime", renderer->time);
    shader_set_float(renderer->shader, "u_GlitchIntensity", 0.1f);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(texture);

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void _renderer_text_draw(renderer_t *renderer, char *content, f32 x, f32 y, f32 scale, v3 color) {
// #ifdef _WIN32
    v4 vertices[RENDERER_TEXT_LENGTH * 6];
// #else
//     v4 vertices[strlen(content) * 6];
// #endif
    u32 c = 0, len = strlen(content);

    f32 cx = x;
    for (u32 i = 0; i < len; i++) {
        f32 col = (f32) (content[i] % 16);
        f32 row = (f32) (content[i] / 16);

        f32 umin = col / 16.0f;
        f32 vmin = row / 8.0f;
        f32 umax = (col + 1.0f) / 16.0f;
        f32 vmax = (row + 1.0f) / 8.0f;

        f32 sx = cx;
        f32 sy = y;
        f32 w = FONT_WIDTH * scale;
        f32 h = FONT_HEIGHT * scale;

        vertices[c++] = vec4(sx, sy + h, umin, vmin);
        vertices[c++] = vec4(sx, sy, umin, vmax);
        vertices[c++] = vec4(sx + w, sy, umax, vmax);

        vertices[c++] = vec4(sx, sy + h, umin, vmin);
        vertices[c++] = vec4(sx + w, sy, umax, vmax);
        vertices[c++] = vec4(sx + w, sy + h, umax, vmin);

        cx += w;
    }

    glBindVertexArray(renderer->text.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->text.vbo);

    shader_use(renderer->text.shader);
    shader_set_int(renderer->text.shader, "u_Texture", 0);
    shader_set_vec3(renderer->text.shader, "u_Color", color);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(&renderer->text.texture);

    glBufferSubData(GL_ARRAY_BUFFER, 0, c * sizeof(v4), vertices); // here?

    glDrawArrays(GL_TRIANGLES, 0, len * 6);
    glBindVertexArray(0);
}

void _renderer_postprocess_draw(renderer_t *renderer) {
    u32 mrk = 0;
    for (u32 i = 0; i < 2; i++) { // loop postprocess shaders
        if (i == 1) glBindFramebuffer(GL_FRAMEBUFFER, 0); // last loop
        else glBindFramebuffer(GL_FRAMEBUFFER, renderer->postprocessing.fbos[mrk ? 0 : 1]);

        glClear(GL_COLOR_BUFFER_BIT);

        shader_use(renderer->postprocessing.shaders[i]);

        // here set uniforms for specific shader
        if (i == 0) { // crt
            // shader_use(renderer->postprocessing.shaders[i]);
            shader_set_int(renderer->postprocessing.shaders[i], "u_Texture", 0);
            shader_set_uint(renderer->postprocessing.shaders[i], "u_Lines", WINDOW_HEIGHT);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Bleed", 0.0016f);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Vignette", 0.4f);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Grain", 0.16f);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Time", (f32) renderer->time);
        } else if (i == 1) { // glitch'
            // shader_use(renderer->postprocessing.shaders[i]);
            // shader_set_float(renderer->shader, "u_Time", 1.0f);
            // shader_set_float(renderer->shader, "u_Intensity", 0.1f);
        }

        glActiveTexture(GL_TEXTURE0);
        texture_bind(&renderer->postprocessing.textures[mrk ? 1 : 0]);

        glBindVertexArray(renderer->vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        mrk = mrk ? 0 : 1;
    }
}

void renderer_draw(renderer_t *renderer) {
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->postprocessing.fbos[0]);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (u32 i = 0; i < renderer->frame.counter; i++) {
        if (renderer->frame.commands[i].type == COMMAND_TYPE_SPRITE) {
            _renderer_sprite_draw(
                renderer,
                renderer->frame.commands[i].data.sprite.texture,
                vec4(renderer->frame.commands[i].data.sprite.uv.scale.x, renderer->frame.commands[i].data.sprite.uv.scale.y, renderer->frame.commands[i].data.sprite.uv.offset.x, renderer->frame.commands[i].data.sprite.uv.offset.y),
                vec4(renderer->frame.commands[i].pos.x, renderer->frame.commands[i].pos.y, renderer->frame.commands[i].size.x, renderer->frame.commands[i].size.y),
                renderer->frame.commands[i].rot,
                renderer->frame.commands[i].color,
                renderer->frame.commands[i].dissolve,
                renderer->frame.commands[i].flip
            );
        } else {
            _renderer_text_draw(
                renderer,
                renderer->frame.commands[i].data.text.content,
                renderer->frame.commands[i].pos.x,
                renderer->frame.commands[i].pos.y,
                renderer->frame.commands[i].data.text.scale,
                renderer->frame.commands[i].color
            );
        }
    }

    _renderer_postprocess_draw(renderer);
}

void renderer_destroy(renderer_t *renderer) {
    glDeleteVertexArrays(1, &renderer->vao);
    glDeleteBuffers(1, &renderer->vbo);

    glDeleteVertexArrays(1, &renderer->text.vao);
    glDeleteBuffers(1, &renderer->text.vbo);
    texture_destroy(&renderer->text.texture);

    for (u32 i = 0; i < 2; i++) {
        glDeleteFramebuffers(1, &renderer->postprocessing.fbos[i]);
        texture_destroy(&renderer->postprocessing.textures[i]);
    }
}

// GAME

#define GAME_SIMULATION_FIXED_TIMESTEP (1.0f / 60.0f) // 60 fps
#define GAME_ANIMATION_FIXED_TIMESTEP (1.0f / 8.0f) // 8 fps

#define GAME_MEMORY_CAPACITY (2 * 1024 * 1024) // 2 MB
u8 GAME_MEMORY[GAME_MEMORY_CAPACITY];

#define GAME_RESOURCES_SHADER_ARRAY_SIZE 4
#define GAME_RESOURCES_TEXTURE_ARRAY_SIZE 32

#define GAME_LEVEL_GRAVITY -9600.0f

#define GAME_LEVEL_SPRITE_ARRAY_SIZE 4
#define GAME_LEVEL_TEXT_ARRAY_SIZE 2
#define GAME_LEVEL_BULLET_ARRAY_SIZE 8
#define GAME_LEVEL_BULLET_SPEED 8
#define GAME_LEVEL_PARTICLE_ARRAY_SIZE 128

#define GAME_ACTOR_SPRITE_ARRAY_SIZE 8
#define GAME_ACTOR_SPRITE_SCALE 2
#define GAME_ACTOR_SHOOT_COOLDOWN 30

#define GAME_ML_INPUTS 16
#define GAME_ML_OUTPUTS 9
#define GAME_ML_HIDDEN_NEURONS 128

typedef enum {
    GAME_STATE_LOAD = 0,
    GAME_STATE_PAUSE = 1,
    GAME_STATE_PLAY = 2,
    GAME_STATE_RESET = 3
} game_state_t;

typedef enum {
    GAME_CONTROLLER_MANUAL = 0,
    GAME_CONTROLLER_AUTO = 1
} game_controller_t;

typedef struct {
    v3 pos, tpos, hpos;
    f32 yaw, pitch;
    f32 mx, my;
    f32 speed, sens;
    u8 lock;
} game_camera_t;

typedef struct {
    v2 vel, force;
    f32 mass, grav;
    f32 fric, drag;
    f32 bounce;
} rigidbody_t;

#define GAME_COLL_NONE 0
#define GAME_COLL_TOP (1 << 0)
#define GAME_COLL_LEFT (1 << 1)
#define GAME_COLL_BOTTOM (1 << 2)
#define GAME_COLL_RIGHT (1 << 3)

typedef struct {
    v2 min, max;
    u8 mask;
} collider_t;

u8 game_collider_aabb_check(collider_t *a, collider_t *b) {
    return (b->min.x < a->max.x && b->max.x > a->min.x && b->min.y < a->max.y && b->max.y > a->min.y);
}

typedef struct {
    v2 pos;
    collider_t coll;
    sprite_t *sprite;
} rect_t;

void game_rect_init(rect_t *rect, sprite_t *sprite, v2 pos, v2 size) {
    rect->pos = pos;
    rect->coll = (collider_t) {.min = pos, .max = vec2_add(pos, size), .mask = GAME_COLL_NONE};
    rect->sprite = sprite;
}

void game_rect_trans(rect_t *rect, v2 pos, f32 rot, v2 scale) {}

typedef enum {
    ACTOR_ACTION_IDLE = 0,
    ACTOR_ACTION_JUMP = 1,
    ACTOR_ACTION_f64_JUMP = 2,
    ACTOR_ACTION_RUN = 3,
    ACTOR_ACTION_CROUCH = 4,
    ACTOR_ACTION_DASH = 5,
    ACTOR_ACTION_ATTACK = 6,
    ACTOR_ACTION_DEATH = 7
} actor_action_t;

typedef struct {
    v2 position, size;
    f32 rotation;
    v2 clip, offset;
} actor_state_t;

typedef enum {
    ACTOR_ANIMATION_STEP_1 = 1,
    ACTOR_ANIMATION_STEP_2 = 2,
    ACTOR_ANIMATION_STEP_3 = 3,
    ACTOR_ANIMATION_STEP_4 = 4,
    ACTOR_ANIMATION_STEP_6 = 6
} actor_animation_step_t;

typedef struct {
    actor_animation_step_t step;
    u8 tick, lock;
} actor_animation_t;

typedef enum {
    ACTOR_COOLDOWN_SHOOT = 0,
    ACTOR_COOLDOWN_f64_JUMP = 1,
    ACTOR_COOLDOWN_DASH = 2
} actor_cooldown_t;

typedef struct {
    v2 pos;
    rigidbody_t rigb;
    collider_t coll;

    sprite_t *sprites; // sprites as array? and action == index (with pos relative to actor's pose?), pos
    actor_action_t action;
    actor_animation_t animation; // mv to anim
    // actor_state_t cstate, pstate;

    ml_trajectory_t traject;

    u16 cooldowns[3];
    u16 stats[2]; // wohoooo

    // effects struct
    f32 effects[1];
    // effects

    u8 alive, grounded;
    u8 jumped, crouched, dashed;
    u8 flip;

    u16 kills; // change to uint16_t stats[2]? (don't forget to prevent overflow: KILLS < UINT16_MAX)
    u16 deaths;
} actor_t;

void game_actor_trans(actor_t *actor, v2 pos, f32 rot, v2 scale) {
    actor->pos = pos;
    actor->rigb.vel = vec2(0.0f, 0.0f); // temp?
    actor->coll.min = pos;
    actor->coll.max = vec2(pos.x + actor->sprites[actor->action].size.x, pos.y + actor->sprites[actor->action].size.y);
}

typedef struct {
    v2 pos;
    rigidbody_t rigb;
    collider_t coll;
    sprite_t sprite;
    actor_t *shooter;
    u8 used;
    u8 flip;
} bullet_t;

typedef struct {
    v2 pos, vel;
    v3 color;
    f32 start, end;
    u8 used;
} particle_t;

static struct {

    struct {
        GLFWwindow *window;
        i32 keys[512];
    } sys;

    struct {
        f64 lft, dt; // last frame time, delta time

        struct {
            f64 accum; // accum?
        } physics;

        struct {
            f64 accum;
        } animation;

        struct {
            f64 timer;
            u32 counter;
            u32 value;
        } framerate;

    } clock;

    mem_arena_t arena;

    struct {
        shader_t *shaders;
        texture_t *textures;
    } res;

    renderer_t renderer;

    struct {
        game_state_t state;
        game_controller_t controller;
        game_camera_t camera;

        struct {
            sprite_t *sprites;

            bullet_t *bullets;
            particle_t *particles;

            rect_t ground; // arr?
            
            u16 turn;
        } level;

        actor_t player;
        actor_t enemy;
    } game;

    struct {
        ml_network_t network;
        f32 *region; // make builft-in arena in libml
        u8 capped;
    } ml;

} context;

void game_level_save(void) {}

void game_level_load(void) {}

void game_level_reset(void) {
    context.game.state = GAME_STATE_RESET;

    context.game.player.action = ACTOR_ACTION_IDLE;
    context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
    context.game.player.animation.tick = 0;
    context.game.player.effects[0] = 0.0f;
    context.game.player.alive = 1;
    context.game.player.flip = 0;

    context.game.enemy.action = ACTOR_ACTION_IDLE;
    context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_4;
    context.game.enemy.animation.tick = 0;
    context.game.enemy.effects[0] = 0.0f;
    context.game.enemy.alive = 1;
    context.game.enemy.flip = 1;

    game_actor_trans(&context.game.player, vec2((50.0f + (ml_random() * 200.0f)), 50.0f), 0.0f, vec2(1.0f, 1.0f));
    game_actor_trans(&context.game.enemy, vec2(((f32) WINDOW_WIDTH - 50.0f - (context.game.enemy.sprites[context.game.enemy.action].size.x) - (ml_random() * 200.0f)), 50.0f), 0.0f, vec2(1.0f, 1.0f));

    for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) context.game.level.bullets[i].used = 0;
}

void game_particle_spawn(v2 pos, f32 direct, v3 color) {
    for (u32 i = 0; i < 32; i++) {
        for (u32 j = 0; j < GAME_LEVEL_PARTICLE_ARRAY_SIZE; j++) {
            if (context.game.level.particles[j].used) continue;

            const f32 drag = 0.98f;

            f32 angle = (ml_random() * 30.0f - 15.0f) * (PI / 180.0f);
            f32 speed = ml_random() * 256.0f;

            f32 vx = (cosf(angle) * speed * direct) * drag;
            f32 vy = (sinf(angle) * speed) * drag;

            context.game.level.particles[j] = (particle_t) {
                .pos = vec2(pos.x, pos.y + (ml_random() * 4.0f - 2.0f)),
                .vel = vec2(vx, vy),
                .color = color,
                .start = 0.0f,
                .end = (ml_random() * 0.3f),
                .used = 1
            };

            break;
        }
    }
}

void game_bullet_move(bullet_t *bullet) {
    v2 acc = vec2(
        bullet->rigb.force.x / bullet->rigb.mass,
        (bullet->rigb.force.y / bullet->rigb.mass) + (GAME_LEVEL_GRAVITY * bullet->rigb.grav)
    );

    bullet->rigb.vel.x = (bullet->rigb.vel.x + acc.x * GAME_SIMULATION_FIXED_TIMESTEP) * bullet->rigb.drag;
    bullet->rigb.vel.y = (bullet->rigb.vel.y + acc.y * GAME_SIMULATION_FIXED_TIMESTEP) * bullet->rigb.drag;

    bullet->rigb.force.x -= (bullet->flip ? 16.0f : -16.0f);
}

void game_bullet_colls_handle(bullet_t *bullet, collider_t **colls) { // it has to rewritten (ray betwen pos and prev pos)
    bullet->pos.x += bullet->rigb.vel.x * GAME_SIMULATION_FIXED_TIMESTEP;
    bullet->coll.min = bullet->pos;
    bullet->coll.max = vec2(bullet->pos.x + (GAME_ACTOR_SPRITE_SCALE * 8.0f), bullet->pos.y + (GAME_ACTOR_SPRITE_SCALE * 4.0f));
    bullet->coll.mask = GAME_COLL_NONE;

    for (u32 i = 0; i < 2; i++) {
        if (game_collider_aabb_check(&bullet->coll, colls[i])) {
            if (bullet->shooter->coll.min.x == colls[i]->min.x && bullet->shooter->coll.min.y == colls[i]->min.y
                && bullet->shooter->coll.max.x == colls[i]->max.x && bullet->shooter->coll.max.y == colls[i]->max.y) continue;
            bullet->used = 0;
        }
    }
}

void game_bullet_actor_colls_handle(bullet_t *bullet, actor_t **actors) {
    v2 origin = bullet->pos;
    bullet->pos.x += (bullet->rigb.vel.x * GAME_SIMULATION_FIXED_TIMESTEP);
    bullet->pos.y += (bullet->rigb.vel.y * GAME_SIMULATION_FIXED_TIMESTEP);

    bullet->coll.min = vec2(
        (origin.x < bullet->pos.x) ? origin.x : bullet->pos.x,
        (origin.y < bullet->pos.y) ? origin.y : bullet->pos.y
    );
    bullet->coll.max = vec2(
        ((origin.x > bullet->pos.x) ? origin.x : bullet->pos.x) + (8.0f * GAME_ACTOR_SPRITE_SCALE),
        ((origin.y > bullet->pos.y) ? origin.y : bullet->pos.y) + (4.0f * GAME_ACTOR_SPRITE_SCALE)
    );
    bullet->coll.mask = GAME_COLL_NONE;

    for (u32 i = 0; i < 2; i++) {
        if (game_collider_aabb_check(&bullet->coll, &actors[i]->coll)) {
            if (!actors[i]->alive || bullet->shooter == actors[i]) continue;

            actors[i]->coll.min = vec2(0.0f, 0.0f);
            actors[i]->coll.max = vec2(0.0f, 0.0f);
            actors[i]->alive = 0;

            actors[i]->action = ACTOR_ACTION_DEATH;
            actors[i]->animation.step = ACTOR_ANIMATION_STEP_6;
            actors[i]->animation.tick = 0;
            actors[i]->animation.lock = 1;

            if (bullet->shooter->traject.steps > 0) {
                bullet->shooter->traject.rewards[bullet->shooter->traject.steps - 1] += 100.0f;
            }
            bullet->used = 0;

            game_particle_spawn(
                vec2((bullet->rigb.vel.x > 0 ? (actors[i]->pos.x + 38.0f) : actors[i]->pos.x), bullet->pos.y),
                (bullet->rigb.vel.x > 0 ? 1.0f : -1.0f),
                ((actors[i] == &context.game.player) ? vec3(1.0f, 0.0f, 0.8f) : vec3(0.0f, 1.0f, 1.0f))
            );

            return;
        }
    }
}

void game_actor_move(actor_t *actor, f32 xacc, u8 jump, u8 crouch, u8 dash) {
    if (!actor->alive) return;

    if (actor->pos.x <= 0.0f) actor->pos.x = 0.0f;
    else if (actor->pos.x >= (WINDOW_WIDTH - actor->sprites[actor->action].size.x)) {
        actor->pos.x = (WINDOW_WIDTH - actor->sprites[actor->action].size.x);
    }

    if (dash && actor->dashed == 0 && actor->cooldowns[ACTOR_COOLDOWN_DASH] == 0) {
        actor->rigb.vel.x = actor->flip ? -1024.0f : 1024.0f;
        actor->rigb.vel.y = 0.0f;
        actor->cooldowns[ACTOR_COOLDOWN_DASH] = 60.0f;
        actor->dashed = 16; // temp
    }

    if (actor->dashed > 0)  {
        xacc = 0.0f; crouch = 0; jump = 0; actor->rigb.grav = 0.0f; actor->dashed--;
    } else {
        actor->rigb.grav = 1.0f;
    }

    if (crouch && actor->grounded) {
        xacc = 0.0f; actor->crouched = 1;
    } else {
        actor->crouched = 0;
    }

    actor->rigb.force.x += (xacc * 2048.0f);

    if (jump && actor->grounded) {
        actor->rigb.vel.y = 2048.0f;
        actor->cooldowns[ACTOR_COOLDOWN_f64_JUMP] = 3;
        actor->jumped = 1;
        actor->grounded = 0;
    } else if (jump && actor->jumped == 1 && actor->cooldowns[ACTOR_COOLDOWN_f64_JUMP] == 0) {
        actor->rigb.vel.y = 3072.0f;
        actor->jumped = 2;
    }

    v2 acc = vec2(
        actor->rigb.force.x / actor->rigb.mass, 
        (actor->rigb.force.y / actor->rigb.mass) + (GAME_LEVEL_GRAVITY * actor->rigb.grav)
    );

    actor->rigb.vel.x += (acc.x * GAME_SIMULATION_FIXED_TIMESTEP);
    actor->rigb.vel.y += (acc.y * GAME_SIMULATION_FIXED_TIMESTEP);

    if (actor->grounded) {
        actor->rigb.vel.x *= actor->rigb.fric;
    } else {
        actor->rigb.vel.x *= actor->rigb.drag;
        actor->rigb.vel.y *= actor->rigb.drag;
    }

    actor->rigb.force.x = 0.0f;
    actor->rigb.force.y = 0.0f;

    if (actor->dashed > 0) {
        if (actor->action != ACTOR_ACTION_DASH) {
            actor->action = ACTOR_ACTION_DASH;
            actor->animation.step = ACTOR_ANIMATION_STEP_6;
            actor->animation.tick = 0;
        }
    } else if (actor->crouched) {
        if (actor->action != ACTOR_ACTION_CROUCH) {
            actor->action = ACTOR_ACTION_CROUCH;
            actor->animation.step = ACTOR_ANIMATION_STEP_1;
            actor->animation.tick = 0;
        }
    } else if (!actor->grounded) {
        if (actor->jumped == 2) {
            if (actor->action != ACTOR_ACTION_f64_JUMP) {
                actor->action = ACTOR_ACTION_f64_JUMP;
                actor->animation.step = ACTOR_ANIMATION_STEP_4;
                actor->animation.tick = 0;
                actor->animation.lock = 1;
            }
        } else {
            if (actor->action != ACTOR_ACTION_IDLE) {
                actor->action = ACTOR_ACTION_IDLE;
                actor->animation.step = ACTOR_ANIMATION_STEP_4;
                actor->animation.tick = 0;
                actor->animation.lock = 0;
            }
        }
    } else if (xacc != 0.0f) {
        if (actor->action != ACTOR_ACTION_RUN) {
            actor->action = ACTOR_ACTION_RUN;
            actor->animation.step = ACTOR_ANIMATION_STEP_6;
            actor->animation.tick = 0;
        }
    } else {
        if (actor->action != ACTOR_ACTION_IDLE) {
            actor->action = ACTOR_ACTION_IDLE;
            actor->animation.step = ACTOR_ANIMATION_STEP_4;
            actor->animation.tick = 0;
        }
    }

}

void game_actor_shoot(actor_t *actor) {
    if (!actor->alive || actor->cooldowns[ACTOR_COOLDOWN_SHOOT] > 0 || actor->action == ACTOR_ACTION_f64_JUMP) return;

    v2 pos = vec2(
        actor->flip ? (actor->coll.min.x - (GAME_ACTOR_SPRITE_SCALE * 8.0f)) : actor->coll.max.x,
        actor->coll.max.y - (GAME_ACTOR_SPRITE_SCALE * (actor->crouched ? 4.0f: 16.0f))
    );

    bullet_t bullet = {
        .pos = pos,
        .rigb = (rigidbody_t) {.vel = vec2(0.0f, 0.0f), .force = vec2(actor->flip ? -128.0f : 128.0f, 0.0f), .mass = 0.01f, .grav = 0.0f, .fric = 1.0f, .drag = 0.99f, .bounce = 0.0f},
        .coll = (collider_t) {.min = pos, .max = vec2(pos.x + (GAME_ACTOR_SPRITE_SCALE * 8.0f), pos.y + (GAME_ACTOR_SPRITE_SCALE * 4.0f)), .mask = GAME_COLL_NONE},
        .sprite = context.game.level.sprites[1],
        .shooter = actor,
        .used = 1,
        .flip = actor->flip
    };

    for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) { // is uint32_t bad as a loop index?
        if (!context.game.level.bullets[i].used) {context.game.level.bullets[i] = bullet; break;}
    }

    actor->cooldowns[ACTOR_COOLDOWN_SHOOT] = GAME_ACTOR_SHOOT_COOLDOWN;
}

void game_actor_colls_handle(actor_t *actor, collider_t **colls) {
    if (!actor->alive) return;

    actor->pos.x += actor->rigb.vel.x * GAME_SIMULATION_FIXED_TIMESTEP;
    actor->coll.min = actor->pos;
    actor->coll.max = vec2(actor->pos.x + actor->sprites[actor->action].size.x, actor->crouched ? actor->pos.y + (actor->sprites[actor->action].size.y * 0.5f) : actor->pos.y + actor->sprites[actor->action].size.y);
    actor->coll.mask = GAME_COLL_NONE;
    actor->grounded = 0;

    // for (uint32_t i = 0; colls[i] != NULL; i++) // works well with: collider_t *pcolls[3] = {&a, &b, NULL};
    for (u32 i = 0; i < 2; i++) {
        if (game_collider_aabb_check(&actor->coll, colls[i])) {
            if (actor->rigb.vel.x > 0) {
                actor->pos.x = colls[i]->min.x - actor->sprites[actor->action].size.x;
                actor->coll.mask |= GAME_COLL_RIGHT;
            } else if (actor->rigb.vel.x < 0) {
                actor->pos.x = colls[i]->max.x;
                actor->coll.mask |= GAME_COLL_LEFT;
            }
            actor->rigb.vel.x = 0.0f;
        }
    }

    actor->pos.y += actor->rigb.vel.y * GAME_SIMULATION_FIXED_TIMESTEP;
    actor->coll.min = actor->pos;
    actor->coll.max = vec2(actor->pos.x + actor->sprites[actor->action].size.x, actor->crouched ? actor->pos.y + (actor->sprites[actor->action].size.y * 0.5f) : actor->pos.y + actor->sprites[actor->action].size.y);

    for (u32 i = 0; i < 2; i++) {
        if (game_collider_aabb_check(&actor->coll, colls[i])) {
            if (actor->rigb.vel.y > 0) {
                actor->pos.y = colls[i]->min.y - (actor->crouched ? (actor->sprites[actor->action].size.y / 2.0f) : actor->sprites[actor->action].size.y);
                actor->coll.mask |= GAME_COLL_TOP;
            } else if (actor->rigb.vel.y < 0) {
                actor->pos.y = colls[i]->max.y;
                actor->coll.mask |= GAME_COLL_BOTTOM;
                actor->jumped = 0;
                actor->grounded = 1;
            }
            actor->rigb.vel.y = 0.0f;
        }
    }
}

void game_res_init(void) {} // load shaders and textures

void game_ml_init(void) {
    f32 *memregs[5] = {
        mem_arena_alloc(&context.arena, GAME_ML_INPUTS * GAME_ML_HIDDEN_NEURONS * sizeof(f32)),
        mem_arena_alloc(&context.arena, GAME_ML_HIDDEN_NEURONS * sizeof(f32)),
        mem_arena_alloc(&context.arena, GAME_ML_OUTPUTS * GAME_ML_HIDDEN_NEURONS * sizeof(f32)),
        mem_arena_alloc(&context.arena, GAME_ML_OUTPUTS * sizeof(f32)),
        mem_arena_alloc(&context.arena, GAME_ML_HIDDEN_NEURONS * sizeof(f32))
    };

    ml_network_init(&context.ml.network, memregs, GAME_ML_HIDDEN_NEURONS, GAME_ML_INPUTS, GAME_ML_OUTPUTS);

    v4 size = vec4(
        GAME_ML_INPUTS * GAME_ML_HIDDEN_NEURONS,
        GAME_ML_HIDDEN_NEURONS,
        GAME_ML_OUTPUTS * GAME_ML_HIDDEN_NEURONS,
        GAME_ML_OUTPUTS
    );

    context.ml.region = mem_arena_alloc(&context.arena, ((size.x + size.y + size.z + size.w) + (GAME_ML_OUTPUTS * 2) + (GAME_ML_HIDDEN_NEURONS * 3) + (size.z * 2) + GAME_ML_INPUTS + (GAME_ML_INPUTS * GAME_ML_HIDDEN_NEURONS)) * sizeof(f32));
    context.ml.capped = 0;
}

void game_ml_step(actor_t *actor, actor_t *enemy, ml_trajectory_t *traject, i8 quadrant) {
    if (!actor->alive) return;

    f32 bdx = 0.0f, bdy = 0.0f;
    f32 mdist = 9999999.0f, threat = 0.0f;

    for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) { // bullet awareness
        if (context.game.level.bullets[i].used && context.game.level.bullets[i].shooter != actor) {
            
            f32 dx = context.game.level.bullets[i].pos.x - actor->pos.x;
            f32 dy = context.game.level.bullets[i].pos.y - actor->pos.y;
            f32 dist = (dx * dx) + (dy * dy);
            
            if (dist < mdist) {
                bdx = (dx * quadrant) / (f32) WINDOW_WIDTH;
                bdy = dy / (f32) WINDOW_HEIGHT;
                mdist = dist;

                if ((dx > 0 && context.game.level.bullets[i].rigb.vel.x < 0) || (dx < 0 && context.game.level.bullets[i].rigb.vel.x > 0)) threat = 1.0f; 
                else threat = 0.0f;
            }
        }
    }

    f32 inputs[GAME_ML_INPUTS] = {
        ((enemy->pos.x - actor->pos.x) * quadrant) / (f32) WINDOW_WIDTH,
        (enemy->pos.y - actor->pos.y) / (f32) WINDOW_HEIGHT,
        (enemy->rigb.vel.x * quadrant) / 1000.0f,
        enemy->rigb.vel.y / 1000.0f,
        (f32) enemy->grounded,
        actor->cooldowns[ACTOR_COOLDOWN_SHOOT] > 0 ? 1.0f : 0.0f,
        bdx,
        bdy,
        (actor->rigb.vel.x * quadrant) / 1000.0f,
        actor->rigb.vel.y / 1000.0f,
        (f32) actor->grounded,
        ((!actor->flip && enemy->pos.x > actor->pos.x) || (actor->flip && enemy->pos.x < actor->pos.x)),
        ((quadrant == 1.0f) ? actor->pos.x : ((f32) WINDOW_WIDTH - actor->pos.x)) / (f32) WINDOW_WIDTH,
        enemy->cooldowns[ACTOR_COOLDOWN_SHOOT] > 0 ? 1.0f : 0.0f,
        threat,
        actor->cooldowns[ACTOR_COOLDOWN_DASH] > 0 ? 1.0f : 0.0f
    };
    mat_t state = mat(inputs, 1, GAME_ML_INPUTS);

    f32 outputs[GAME_ML_OUTPUTS];
    mat_t prob = mat(outputs, 1, GAME_ML_OUTPUTS);

    ml_network_forward_move(&context.ml.network, &state, &prob);

    f32 xacc = 0.0f;
    u8 jump = 0, crouch = 0, shoot = 0, dash = 0;

    i32 actions[2] = {4, 8};

    f32 sample = ml_random(), accum = 0.0f;
    for (u32 i = 0; i < 6; i++) {
        accum += prob.data[i];
        if (sample <= accum) {
            actions[0] = i; break;
        }
    }

    sample = ml_random(), accum = 0.0f;
    for (u32 i = 6; i < 9; i++) {
        accum += prob.data[i];
        if (sample <= accum) {
            actions[1] = i; break;
        }
    }

    if (actions[0] == 1) xacc = 1.0f * quadrant;
    if (actions[0] == 2) xacc = -1.0f * quadrant;
    if (actions[0] == 3) jump = 1;
    if (actions[0] == 4) crouch = 1;
    if (actions[0] == 5) dash = 1;

    if (actions[1] == 6) actor->flip = (quadrant == 1.0f) ? 1 : 0;
    if (actions[1] == 7) actor->flip = (quadrant == 1.0f) ? 0 : 1;
    if (actions[1] == 8) {actor->flip = (quadrant == 1.0f) ? 0 : 1; shoot = 1;}

    game_actor_move(actor, xacc, jump, crouch, dash);

    if (quadrant == 1.0f && actor->pos.x > (WINDOW_WIDTH * 0.5f) - 60.0f) {
        actor->pos.x = (WINDOW_WIDTH * 0.5f) - 60.0f;
        actor->rigb.vel.x = 0;
    }
    if (quadrant == -1.0f && actor->pos.x < (WINDOW_WIDTH * 0.5f) + 60.0f) {
        actor->pos.x = (WINDOW_WIDTH * 0.5f) + 60.0f;
        actor->rigb.vel.x = 0;
    }

    collider_t *colls[] = {&context.game.level.ground.coll, &enemy->coll};
    game_actor_colls_handle(actor, colls);

    f32 reward = -0.02f;

    f32 enemydist = fabsf(enemy->pos.x - actor->pos.x);
    if (enemydist > 150.0f && enemydist < 450.0f) reward += 0.1f;
    else if (enemydist > 450.0f) reward -= 0.2f;
    else reward -= 0.1f;

    f32 middist = fabsf(actor->pos.x - (WINDOW_WIDTH * 0.5f));
    if (middist > 300.0f) reward -= 0.5f;

    u8 lowdist = (mdist < 15000.0f);
    if (lowdist && actions[0] == 3) reward += 0.2f;
    if (!lowdist && actions[0] == 3) reward -= 0.1f;
    if (!lowdist && actions[0] == 4) reward -= 0.1f;
    if (lowdist && actions[0] == 5) reward += 0.2f;
    if (!lowdist && actions[0] == 5) reward -= 0.1f;

    u8 facing = ((!actor->flip && enemy->pos.x > actor->pos.x) || (actor->flip && enemy->pos.x < actor->pos.x));
    if (!facing) reward -= 0.05f;

    if (shoot) {
        u8 valid = (actor->alive && actor->cooldowns[ACTOR_COOLDOWN_SHOOT] == 0);

        game_actor_shoot(actor);

        if (valid) {
            if (facing) {
                if (fabsf(enemy->pos.y - actor->pos.y) < 60.0f) reward += 0.3f;
                reward += 0.2f;
            } else {
                reward -= 1.0f;
            }
        } else {
            reward -= 0.01f;
        }
    }

    ml_trajectory_step_add(traject, &state, actions, reward);
}

void _game_window_icon_init(void) {
    const char *path = "res/arena.png";
    i32 width, height, channels;
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 0);
    if (!ASSERT(pixels != NULL)) {
        return;
    }

    GLFWimage images[1] = {(GLFWimage) {.width = width, .height = height, .pixels = pixels}};
    glfwSetWindowIcon(context.sys.window, 1, images);

    stbi_image_free(pixels);
}

void _game_keyboard_callback(GLFWwindow *window, i32 key, i32 scan, i32 action, i32 mode) {
    if (key > -1 && key < 512) {
        if (action == GLFW_PRESS) context.sys.keys[key] = 1;
        else if (action == GLFW_RELEASE) context.sys.keys[key] = 0;
    }
}

void game_keyboard_handle(void) { // it has to rewritten as well (try to handle keys only here, without logic) (callbacks? events?)
    if (context.game.state == GAME_STATE_LOAD) return;

    // KEY Q (quit)
    if (context.sys.keys[GLFW_KEY_LEFT_CONTROL] && context.sys.keys[GLFW_KEY_Q]) {
        glfwSetWindowShouldClose(context.sys.window, 1);
    }

    // KEY P (pause)
    if (context.sys.keys[GLFW_KEY_P]) {
        if (context.game.state != GAME_STATE_PAUSE) context.game.state = GAME_STATE_PAUSE;
    }

    // KEY R (resume)
    if (context.sys.keys[GLFW_KEY_R]) {
        if (context.game.state != GAME_STATE_PLAY) context.game.state = GAME_STATE_PLAY;
    }

    // KEY T (control)
    if (context.sys.keys[GLFW_KEY_T]) {
        if (context.game.controller == GAME_CONTROLLER_AUTO) context.game.controller = GAME_CONTROLLER_MANUAL;
        else context.game.controller = GAME_CONTROLLER_AUTO;
    }

    // KEY ESC (reset)
    if (context.sys.keys[GLFW_KEY_ESCAPE]) {
        if (context.game.state != GAME_STATE_RESET) game_level_reset();
    }
    if (!context.sys.keys[GLFW_KEY_ESCAPE]) { // quick workaround
        if (context.game.state == GAME_STATE_RESET) context.game.state = GAME_STATE_PLAY;
    }

    if (context.game.state != GAME_STATE_PLAY) return;
    if (context.game.controller != GAME_CONTROLLER_MANUAL) return;

    f32 xacc = 0.0f;
    u8 jump = 0, crouch = 0, dash = 0;

    // KEY W
    if (context.sys.keys[GLFW_KEY_W] == 1) {
        context.sys.keys[GLFW_KEY_W] = 2; jump = 1;
    }

    // KEY A
    if (context.sys.keys[GLFW_KEY_A]) {
        xacc -= 1.0f;
    }

    // KEY S
    if (context.sys.keys[GLFW_KEY_S]) {
        crouch = 1;
    }

    // KEY D
    if (context.sys.keys[GLFW_KEY_D]) {
        xacc += 1.0f;
    }

    // KEY LEFT
    if (context.sys.keys[GLFW_KEY_LEFT]) {
        context.game.player.flip = 1;
    }

    // KEY RIGHT
    if (context.sys.keys[GLFW_KEY_RIGHT]) {
        context.game.player.flip = 0;
    }

    // KEY SHIFT
    if (context.sys.keys[GLFW_KEY_LEFT_SHIFT] == 1) {
        context.sys.keys[GLFW_KEY_LEFT_SHIFT] = 2; dash = 1;
    }

    if (context.game.player.alive) {
        game_actor_move(&context.game.player, xacc, jump, crouch, dash);

        collider_t *colls[] = {&context.game.level.ground.coll, &context.game.enemy.coll};
        game_actor_colls_handle(&context.game.player, colls);

        // KEY SPACE
        if (context.sys.keys[GLFW_KEY_SPACE]) {
            game_actor_shoot(&context.game.player);
        }
    }
}

void game_init(void) {

    // GLFW
    if (!ASSERT(glfwInit())) {
        // do smth
    }
    // ASSERT(glfwInit(), "OPENGL_INIT_ERROR\n");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "arena");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "arena");
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "arena");

    context.sys.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
    if (!ASSERT(context.sys.window != NULL)) {
        // do smth
    }

    glfwMakeContextCurrent(context.sys.window);
    glfwSetKeyCallback(context.sys.window, _game_keyboard_callback);
    // glfwSetInputMode(context.sys.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);
    // glfwSwapInterval(0);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1; // what?
    i32 glewerr = glewInit();
    if (!ASSERT(glewerr == 0 || glewerr == 4)) {
        // do smth
    }
    // ASSERT(glewerr == 0 || glewerr == 4, "GLEW_INIT_ERROR\n"); // this needs to be rethinked
#endif

    // OPENGL
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    // glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // ICON
    _game_window_icon_init();

    // MEMORY
    mem_arena_init(&context.arena, &GAME_MEMORY, GAME_MEMORY_CAPACITY);

    // RESOURCES
    context.res.shaders = mem_arena_alloc(&context.arena, GAME_RESOURCES_SHADER_ARRAY_SIZE * sizeof(shader_t));
    if (!ASSERT(context.res.shaders != NULL)) {/*do something*/}

    if (shader_init(&context.res.shaders[0], (const char*[]) {"res/shader/sprite.vs", "res/shader/sprite.fs"}) != SHADER_STATUS_SUCCESS) {
        // do something as well
    }
    if (shader_init(&context.res.shaders[1], (const char*[]) {"res/shader/text.vs", "res/shader/text.fs"}) != SHADER_STATUS_SUCCESS) {
        // do something as well
    }
    if (shader_init(&context.res.shaders[2], (const char*[]) {"res/shader/crt.vs", "res/shader/crt.fs"}) != SHADER_STATUS_SUCCESS) {
        // do something as well
    }
    // if (shader_init(&context.res.shaders[3], (const char*[]) {"res/shader/glitch.vs", "res/shader/glitch.fs"}) != SHADER_STATUS_SUCCESS) {
    //     // do something as well
    // }

    // TODO read it auto, and search it by name (only in init)
    context.res.textures = mem_arena_alloc(&context.arena, GAME_RESOURCES_TEXTURE_ARRAY_SIZE * sizeof(texture_t));
    if (texture_init(&context.res.textures[0], "res/texture/mutiny.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[1], "res/texture/bullet.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[2], "res/texture/gun.png") != TEXTURE_STATUS_SUCCESS) {}

    if (texture_init(&context.res.textures[3], "res/texture/level/tile.png") != TEXTURE_STATUS_SUCCESS) {}

    if (texture_init(&context.res.textures[4], "res/texture/player/punk_idle-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[5], "res/texture/player/punk_jump.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[6], "res/texture/player/punk_double_jump-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[7], "res/texture/player/punk_run-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[8], "res/texture/player/punk_crouch-2.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[9], "res/texture/player/punk_dash-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[10], "res/texture/player/punk_attack.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[11], "res/texture/player/punk_death.png") != TEXTURE_STATUS_SUCCESS) {}

    if (texture_init(&context.res.textures[12], "res/texture/enemy/cyborg_idle-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[13], "res/texture/enemy/cyborg_jump.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[14], "res/texture/enemy/cyborg_double_jump-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[15], "res/texture/enemy/cyborg_run-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[16], "res/texture/enemy/cyborg_crouch-2.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[17], "res/texture/enemy/cyborg_dash-comp.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[18], "res/texture/enemy/cyborg_attack.png") != TEXTURE_STATUS_SUCCESS) {}
    if (texture_init(&context.res.textures[19], "res/texture/enemy/cyborg_death-comp.png") != TEXTURE_STATUS_SUCCESS) {}

    // RENDERER
    context.renderer.frame.commands = mem_arena_alloc(&context.arena, RENDERER_COMMAND_ARRAY_SIZE * sizeof(command_t));
    renderer_init(&context.renderer, context.res.shaders);

    // GAME
    context.game.state = GAME_STATE_LOAD;
    context.game.controller = GAME_CONTROLLER_AUTO;
    // context.game.controller = GAME_CONTROLLER_MANUAL;
    context.game.camera.pos = vec3(0.0f, 0.0f, 8.0f);
    context.game.camera.tpos = vec3(0.0f, 0.0f, -1.0f);
    context.game.camera.hpos = vec3(0.0f, 1.0f, 0.0f);
    context.game.camera.yaw = -90.0f;
    context.game.camera.pitch = 0.0f;
    context.game.camera.speed = 4.0f;
    context.game.camera.sens = 0.2f;
    context.game.level.sprites = mem_arena_alloc(&context.arena, GAME_LEVEL_SPRITE_ARRAY_SIZE * sizeof(sprite_t));
    context.game.level.bullets = mem_arena_alloc(&context.arena, GAME_LEVEL_BULLET_ARRAY_SIZE * sizeof(bullet_t));
    for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) context.game.level.bullets[i] = (bullet_t) {0}; // is there a cooler way to init this?
    context.game.level.particles = mem_arena_alloc(&context.arena, GAME_LEVEL_PARTICLE_ARRAY_SIZE * sizeof(particle_t));
    for (u32 i = 0; i < GAME_LEVEL_PARTICLE_ARRAY_SIZE; i++) context.game.level.particles[i] = (particle_t) {0};
    context.game.level.turn = 0;

    sprite_init(&context.game.level.sprites[0], &context.res.textures[1], vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 6.0f, GAME_ACTOR_SPRITE_SCALE * 6.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // bullet
    sprite_init(&context.game.level.sprites[1], &context.res.textures[2], vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 18.0f, GAME_ACTOR_SPRITE_SCALE * 8.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // gun
    sprite_init(&context.game.level.sprites[2], &context.res.textures[3], vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2((f32) WINDOW_WIDTH, 50.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // rect

    // level
    game_rect_init(&context.game.level.ground, &context.game.level.sprites[2], vec2(0.0f, 0.0f), vec2((f32) WINDOW_WIDTH, ((f32) WINDOW_HEIGHT / 12.0f)));

    // actor
    context.game.player.pos = vec2(100.0f, 50.0f);
    context.game.player.rigb = (rigidbody_t) {.vel = vec2(0.0f, 0.0f), .force = vec2(0.0f, 0.0f), .mass = 1.0f, .grav = 1.0f, .fric = 0.9f, .drag = 0.94f, .bounce = 0.0f};
    context.game.player.coll = (collider_t) {.min = vec2(100.0f, 50.0f), .max = vec2(100.0f + (GAME_ACTOR_SPRITE_SCALE * 17.0f), 50.0f + (GAME_ACTOR_SPRITE_SCALE * 34.0f)), .mask = GAME_COLL_NONE};
    context.game.player.sprites = mem_arena_alloc(&context.arena, GAME_ACTOR_SPRITE_ARRAY_SIZE * sizeof(sprite_t));
    context.game.player.action = ACTOR_ACTION_IDLE;
    context.game.player.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.game.player.effects[0] = 0.0f;
    context.game.player.alive = 1;
    context.game.player.crouched = 0;
    context.game.player.jumped = 0;
    context.game.player.grounded = 0;
    context.game.player.flip = 0;
    context.game.player.kills = 0;
    context.game.player.deaths = 0;

    // sprite_init(&sprite, "punk_run", ..);
    sprite_init(&context.game.player.sprites[0], &context.res.textures[4], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 19.0f, GAME_ACTOR_SPRITE_SCALE * 35.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // idle
    // sprite_init(&context.game.player.sprites[1], &context.res.textures[5], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // jump
    sprite_init(&context.game.player.sprites[2], &context.res.textures[6], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 31.0f, GAME_ACTOR_SPRITE_SCALE * 32.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // f64 jump
    sprite_init(&context.game.player.sprites[3], &context.res.textures[7],vec2(0.167f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 17.0f, GAME_ACTOR_SPRITE_SCALE * 33.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // run
    sprite_init(&context.game.player.sprites[4], &context.res.textures[8], vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 16.0f, GAME_ACTOR_SPRITE_SCALE * 27.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // crouch
    sprite_init(&context.game.player.sprites[5], &context.res.textures[9], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 30.0f, GAME_ACTOR_SPRITE_SCALE * 35.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // dash
    // sprite_init(&context.game.player.sprites[6], &context.res.textures[10], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // attack
    sprite_init(&context.game.player.sprites[7], &context.res.textures[11], vec2(0.167f, 1.0f), vec2(0.501f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 48.0f, GAME_ACTOR_SPRITE_SCALE * 48.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // death

    context.game.enemy.pos = vec2((f32) WINDOW_WIDTH - 100.0f - (GAME_ACTOR_SPRITE_SCALE * 16.0f), 50.0f);
    context.game.enemy.rigb = (rigidbody_t) {.vel = vec2(0.0f, 0.0f), .force = vec2(0.0f, 0.0f), .mass = 1.0f, .grav = 1.0f, .fric = 0.9f, .drag = 0.94f, .bounce = 0.0f};
    context.game.enemy.coll = (collider_t) {.min = vec2(((f32) WINDOW_WIDTH - 100.0f - (GAME_ACTOR_SPRITE_SCALE * 16.0f)), 50.0f), .max = vec2(((f32) WINDOW_WIDTH - 100.0f - (GAME_ACTOR_SPRITE_SCALE * 16.0f)) + (GAME_ACTOR_SPRITE_SCALE * 20.0f), 50.0f + (GAME_ACTOR_SPRITE_SCALE * 35.0f)), .mask = GAME_COLL_NONE};
    context.game.enemy.sprites = mem_arena_alloc(&context.arena, GAME_ACTOR_SPRITE_ARRAY_SIZE * sizeof(sprite_t));
    context.game.enemy.action = ACTOR_ACTION_IDLE;
    context.game.enemy.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.game.enemy.effects[0] = 0.0f;
    context.game.enemy.alive = 1;
    context.game.enemy.jumped = 0;
    context.game.enemy.crouched = 0;
    context.game.enemy.grounded = 0;
    context.game.enemy.flip = 1;
    context.game.enemy.kills = 0;
    context.game.enemy.deaths = 0;

    sprite_init(&context.game.enemy.sprites[0], &context.res.textures[12], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 20.0f, GAME_ACTOR_SPRITE_SCALE * 35.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // idle
    // sprite_init(&context.game.enemy.sprites[1], &context.res.textures[13], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // jump
    sprite_init(&context.game.enemy.sprites[2], &context.res.textures[14], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 31.0f, GAME_ACTOR_SPRITE_SCALE * 32.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // f64 jump
    sprite_init(&context.game.enemy.sprites[3], &context.res.textures[15],vec2(0.167f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 17.0f, GAME_ACTOR_SPRITE_SCALE * 32.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // run
    sprite_init(&context.game.enemy.sprites[4], &context.res.textures[16], vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 18.0f, GAME_ACTOR_SPRITE_SCALE * 27.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // crouch
    sprite_init(&context.game.enemy.sprites[5], &context.res.textures[17], vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 30.0f, GAME_ACTOR_SPRITE_SCALE * 35.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // dash
    // sprite_init(&context.game.enemy.sprites[6], &context.res.textures[18], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // attack
    sprite_init(&context.game.enemy.sprites[7], &context.res.textures[19], vec2(0.167f, 1.0f), vec2(0.501f, 0.0f), vec2(0.0f, 0.0f), vec2(GAME_ACTOR_SPRITE_SCALE * 22.0f, GAME_ACTOR_SPRITE_SCALE * 35.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f), 0, 0); // death

    // ML
    game_ml_init();

    ml_trajectory_init(&context.game.player.traject, mem_arena_alloc(&context.arena, ML_EPISODE_STEPS * GAME_ML_INPUTS * sizeof(f32)), GAME_ML_INPUTS);
    ml_trajectory_init(&context.game.enemy.traject, mem_arena_alloc(&context.arena, ML_EPISODE_STEPS * GAME_ML_INPUTS * sizeof(f32)), GAME_ML_INPUTS);

    // CLOCK
    context.clock.lft = glfwGetTime();

#ifdef DEBUG
    printf("GAME_MEMORY: Used %.2f MB / %.2f MB (%.2f%%)\n", ((f32) (context.arena.used) / (1024.0f * 1024.0f)), 
        ((f32) (context.arena.capacity) / (1024.0f * 1024.0f)), (f64) context.arena.used / (f64) context.arena.capacity * 100.0);
#endif

    // context.game.state = GAME_STATE_PLAY;
    // context.game.state = GAME_STATE_PAUSE;
}

void game_update(void) {
    while (!glfwWindowShouldClose(context.sys.window)) {

        // CLOCK
        const f64 time = glfwGetTime();
        context.clock.dt = time - context.clock.lft;
        context.clock.lft = time;

        // loader
        static f64 timer = 0.0;
        if (context.game.state == GAME_STATE_LOAD) {
            if ((timer += context.clock.dt) >= 1.0) context.game.state = GAME_STATE_PAUSE;
        }

        // framerate
        context.clock.framerate.timer += context.clock.dt;

        if (context.clock.framerate.timer >= 1.0f) {
            context.clock.framerate.value = (u32) context.clock.framerate.counter / context.clock.framerate.timer;
            context.clock.framerate.timer -= 1.0f;
            context.clock.framerate.counter = 0;
        }

        // physics
        if (context.clock.dt > 0.25f) {
            context.clock.dt = 0.25f;
        }

        context.clock.physics.accum += context.clock.dt;

        while (context.clock.physics.accum >= GAME_SIMULATION_FIXED_TIMESTEP) {
            
            // TODO save previous actors states

            // input
            game_keyboard_handle();

            if (context.game.state == GAME_STATE_PLAY) {

                // ml
                if (context.game.controller == GAME_CONTROLLER_AUTO) {
                    game_ml_step(&context.game.player, &context.game.enemy, &context.game.player.traject, 1.0f);
                }
                game_ml_step(&context.game.enemy, &context.game.player, &context.game.enemy.traject, -1.0f);

                // physics
                actor_t *actors[] = {&context.game.player, &context.game.enemy};
                for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) {
                    if (context.game.level.bullets[i].used) {
                        if (context.game.level.bullets[i].coll.max.x < 0 || context.game.level.bullets[i].pos.x > WINDOW_WIDTH) {
                            context.game.level.bullets[i].used = 0;
                        } else {
                            game_bullet_move(&context.game.level.bullets[i]);
                        }
                        game_bullet_actor_colls_handle(&context.game.level.bullets[i], actors); // merge into one func
                    }
                }

                for (u32 i = 0; i < GAME_LEVEL_PARTICLE_ARRAY_SIZE; i++) {
                    if (context.game.level.particles[i].used) {
                        context.game.level.particles[i].pos.x += (context.game.level.particles[i].vel.x * GAME_SIMULATION_FIXED_TIMESTEP);
                        context.game.level.particles[i].pos.y += (context.game.level.particles[i].vel.y * GAME_SIMULATION_FIXED_TIMESTEP);
                        context.game.level.particles[i].start += GAME_SIMULATION_FIXED_TIMESTEP;
                        if (context.game.level.particles[i].start >= context.game.level.particles[i].end) context.game.level.particles[i].used = 0;
                    }
                }

                if (!context.game.player.alive && context.game.player.effects[0] < 1.0f) {
                    context.game.player.effects[0] += ((f32) context.clock.dt * 2.0f);
                }
                if (!context.game.enemy.alive && context.game.enemy.effects[0] < 1.0f) {
                    context.game.enemy.effects[0] += ((f32) context.clock.dt * 2.0f);
                }

                // ml
                if (context.game.player.traject.steps >= 600 && (context.game.player.alive && context.game.enemy.alive)) {
                    context.game.player.alive = 0;
                    context.game.enemy.alive = 0;

                    context.game.player.action = ACTOR_ACTION_DEATH;
                    context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
                    context.game.player.animation.tick = 0;
                    context.game.player.animation.lock = 1;

                    context.game.enemy.action = ACTOR_ACTION_DEATH;
                    context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_6;
                    context.game.enemy.animation.tick = 0;
                    context.game.enemy.animation.lock = 1;

                    context.ml.capped = 1;
                }

                if ((!context.game.player.alive || !context.game.enemy.alive) && (!context.game.player.animation.lock && !context.game.enemy.animation.lock)) {

                    if (context.game.player.traject.steps > 0) {
                        f32 term = context.ml.capped ? -10.0f : (context.game.player.alive ? (context.game.enemy.alive ? 0.0f : 150.0f) : -50.0f);
                        context.game.player.traject.rewards[context.game.player.traject.steps - 1] += term;
                        ml_network_episode_train(&context.ml.network, context.ml.region, &context.game.player.traject);
                    }
                    if (context.game.enemy.traject.steps > 0) {
                        f32 term = context.ml.capped ? -10.0f : (context.game.enemy.alive ? (context.game.player.alive ? 0.0f : 150.0f) : -50.0f);
                        context.game.enemy.traject.rewards[context.game.enemy.traject.steps - 1] += term;
                        ml_network_episode_train(&context.ml.network, context.ml.region, &context.game.enemy.traject);
                    }

                    context.game.player.traject.steps = 0;
                    context.game.enemy.traject.steps = 0;

                    // level
                    context.game.level.turn++;
                    
                    if (!context.ml.capped) {
                        if (!context.game.player.alive) {context.game.player.deaths++; context.game.enemy.kills++;}
                        if (!context.game.enemy.alive) {context.game.enemy.deaths++; context.game.player.kills++;}
                    }
                    context.ml.capped = 0;

                    game_level_reset();

                }

                // cooldown
                if (context.game.player.cooldowns[ACTOR_COOLDOWN_SHOOT] > 0) context.game.player.cooldowns[ACTOR_COOLDOWN_SHOOT]--;
                if (context.game.enemy.cooldowns[ACTOR_COOLDOWN_SHOOT] > 0) context.game.enemy.cooldowns[ACTOR_COOLDOWN_SHOOT]--;
                if (context.game.player.cooldowns[ACTOR_COOLDOWN_f64_JUMP] > 0) context.game.player.cooldowns[ACTOR_COOLDOWN_f64_JUMP]--;
                if (context.game.enemy.cooldowns[ACTOR_COOLDOWN_f64_JUMP] > 0) context.game.enemy.cooldowns[ACTOR_COOLDOWN_f64_JUMP]--;
                if (context.game.player.cooldowns[ACTOR_COOLDOWN_DASH] > 0) context.game.player.cooldowns[ACTOR_COOLDOWN_DASH]--;
                if (context.game.enemy.cooldowns[ACTOR_COOLDOWN_DASH] > 0) context.game.enemy.cooldowns[ACTOR_COOLDOWN_DASH]--;

            }

            // animation
            context.clock.animation.accum += GAME_SIMULATION_FIXED_TIMESTEP;

            while (context.clock.animation.accum >= GAME_ANIMATION_FIXED_TIMESTEP) {
                context.clock.animation.accum -= GAME_ANIMATION_FIXED_TIMESTEP;

                // player
                if (context.game.player.action == ACTOR_ACTION_f64_JUMP || context.game.player.action == ACTOR_ACTION_DEATH) {
                    context.game.player.sprites[context.game.player.action].uv.offset.x = (context.game.player.sprites[context.game.player.action].uv.scale.x * context.game.player.animation.tick);
                    if (context.game.player.animation.tick < context.game.player.animation.step - 1) context.game.player.animation.tick++;
                    else context.game.player.animation.lock = 0;
                } else {
                    context.game.player.sprites[context.game.player.action].uv.offset.x = (context.game.player.sprites[context.game.player.action].uv.scale.x * context.game.player.animation.tick);
                    if (context.game.player.animation.tick < context.game.player.animation.step - 1) context.game.player.animation.tick++;
                    else if (!context.game.player.animation.lock) context.game.player.animation.tick = 0;
                }

                // enemy
                if (context.game.enemy.action == ACTOR_ACTION_f64_JUMP || context.game.enemy.action == ACTOR_ACTION_DEATH) {
                    context.game.enemy.sprites[context.game.enemy.action].uv.offset.x = (context.game.enemy.sprites[context.game.enemy.action].uv.scale.x * context.game.enemy.animation.tick);
                    if (context.game.enemy.animation.tick < context.game.enemy.animation.step - 1) context.game.enemy.animation.tick++;
                    else context.game.enemy.animation.lock = 0;
                } else {
                    context.game.enemy.sprites[context.game.enemy.action].uv.offset.x = (context.game.enemy.sprites[context.game.enemy.action].uv.scale.x * context.game.enemy.animation.tick);
                    if (context.game.enemy.animation.tick < context.game.enemy.animation.step - 1) context.game.enemy.animation.tick++;
                    else if (!context.game.enemy.animation.lock) context.game.enemy.animation.tick = 0;
                }

            }

            // physics
            context.clock.physics.accum -= GAME_SIMULATION_FIXED_TIMESTEP;
        }

        // f64 alpha = context.clock.accum / GAME_SIMULATION_FIXED_TIMESTEP;

        // RENDERER
        static u32 fastforward = 0;
        fastforward++;

        if (fastforward % 10 == 0) {
            context.clock.framerate.counter++;
            context.renderer.time = time;

            if (context.game.state == GAME_STATE_LOAD) {
                
                renderer_frame_command_push(&context.renderer, (command_t) {
                    .type = COMMAND_TYPE_SPRITE,
                    .data.sprite = {
                        .texture = &context.res.textures[0],
                        .uv = {.scale = vec2(1.0f, 1.0f), .offset = vec2(0.0f, 0.0f)}
                    },
                    .pos = vec2(0.0f, 0.0f),
                    .size = vec2(800.0f, 600.0f),
                    .rot = 0.0f,
                    .color = vec3(1.0f, 1.0f, 1.0f),
                    .dissolve = 0.0f,
                    .zorder = 0,
                    .flip = 0
                });

                renderer_draw(&context.renderer);
                renderer_frame_clear(&context.renderer);

                // OPENGL
                glfwSwapBuffers(context.sys.window);

                continue;

            }

            // level
            renderer_frame_command_push(&context.renderer, (command_t) {
                .type = COMMAND_TYPE_SPRITE,
                .data.sprite = {
                    .texture = context.game.level.sprites[2].texture,
                    .uv = {.scale = context.game.level.sprites[2].uv.scale, .offset = context.game.level.sprites[2].uv.offset}
                },
                .pos = context.game.level.sprites[2].pos,
                .size = context.game.level.sprites[2].size,
                .rot = context.game.level.sprites[2].rot,
                .color = context.game.level.sprites[2].color,
                .dissolve = 0.0f,
                .zorder = 2,
                .flip = 0
            });

            // bullet
            for (u32 i = 0; i < GAME_LEVEL_BULLET_ARRAY_SIZE; i++) {
                if (context.game.level.bullets[i].used) {
                    renderer_frame_command_push(&context.renderer, (command_t) {
                        .type = COMMAND_TYPE_SPRITE,
                        .data.sprite = {
                            .texture = context.game.level.sprites[0].texture,
                            .uv = {.scale = context.game.level.sprites[0].uv.scale, .offset = context.game.level.sprites[0].uv.offset}
                        },
                        .pos = context.game.level.bullets[i].pos,
                        .size = context.game.level.sprites[0].size,
                        .rot = context.game.level.sprites[0].rot,
                        .color = context.game.level.sprites[0].color,
                        .dissolve = 0.0f,
                        .zorder = 3,
                        .flip = context.game.level.bullets[i].shooter->flip
                    });
                }
            }

            // particle
            for (u32 i = 0; i < GAME_LEVEL_PARTICLE_ARRAY_SIZE; i++) {
                if (context.game.level.particles[i].used) {
                    renderer_frame_command_push(&context.renderer, (command_t) {
                        .type = COMMAND_TYPE_SPRITE,
                        .data.sprite = {
                            .texture = context.game.level.sprites[0].texture,
                            .uv = {.scale = vec2(1.0f, 1.0f), .offset = vec2(0.0f, 0.0f)}
                        },
                        .pos = context.game.level.particles[i].pos,
                        .size = vec2(4.0f, 4.0f),
                        .rot = 0.0f,
                        .color = context.game.level.particles[i].color,
                        .zorder = 3,
                        .flip = 0,
                        .dissolve = 0.0f
                    });
                }
            }

            // player
            if (context.game.player.alive || context.game.player.animation.lock) {

                // TEMP
                u8 action = (context.game.player.action == ACTOR_ACTION_CROUCH) ? 4 : (context.game.player.action == ACTOR_ACTION_RUN) ? 3 : (context.game.player.action == ACTOR_ACTION_f64_JUMP) ? 2 : (context.game.player.action == ACTOR_ACTION_JUMP) ? 1 : (context.game.player.action == ACTOR_ACTION_DASH) ? 5 : (context.game.player.action == ACTOR_ACTION_DEATH) ? 7 : 0;
                // TEMP

                renderer_frame_command_push(&context.renderer, (command_t) {
                    .type = COMMAND_TYPE_SPRITE,
                    .data.sprite = {
                        .texture = context.game.player.sprites[action].texture,
                        .uv = {.scale = context.game.player.sprites[action].uv.scale, .offset = context.game.player.sprites[action].uv.offset}
                    },
                    .pos = vec2_add(context.game.player.pos, context.game.player.sprites[action].pos),
                    .size = context.game.player.sprites[action].size,
                    .rot = context.game.player.sprites[action].rot,
                    .color = context.game.player.sprites[action].color,
                    .dissolve = context.game.player.alive ? 0.0f : context.game.player.effects[0],
                    .zorder = 2,
                    .flip = context.game.player.flip
                });

                if (context.game.player.action != ACTOR_ACTION_f64_JUMP && context.game.player.action != ACTOR_ACTION_DEATH) {
                    renderer_frame_command_push(&context.renderer, (command_t) {
                        .type = COMMAND_TYPE_SPRITE,
                        .data.sprite = {
                            .texture = context.game.level.sprites[1].texture, 
                            .uv = {.scale = context.game.level.sprites[1].uv.scale, .offset = context.game.level.sprites[1].uv.offset}
                        },
                        .pos = vec2(context.game.player.flip ? (context.game.player.coll.min.x - (GAME_ACTOR_SPRITE_SCALE * 18.0f)) : context.game.player.coll.max.x, context.game.player.coll.max.y - (GAME_ACTOR_SPRITE_SCALE * (context.game.player.crouched ? 4.0f: 16.0f))),
                        .size = context.game.level.sprites[1].size,
                        .rot = context.game.level.sprites[1].rot,
                        .color = context.game.level.sprites[1].color,
                        .dissolve = 0.0f,
                        .zorder = 2,
                        .flip = context.game.player.flip
                    });
                }
            }

            // enemy
            if (context.game.enemy.alive || context.game.enemy.animation.lock) {

                // TEMP
                u8 action = (context.game.enemy.action == ACTOR_ACTION_CROUCH) ? 4 : (context.game.enemy.action == ACTOR_ACTION_RUN) ? 3 : (context.game.enemy.action == ACTOR_ACTION_f64_JUMP) ? 2 : (context.game.enemy.action == ACTOR_ACTION_JUMP) ? 1 : (context.game.enemy.action == ACTOR_ACTION_DASH) ? 5 : (context.game.enemy.action == ACTOR_ACTION_DEATH) ? 7 : 0;
                // TEMP

                renderer_frame_command_push(&context.renderer, (command_t) {
                    .type = COMMAND_TYPE_SPRITE,
                    .data.sprite = {
                        .texture = context.game.enemy.sprites[action].texture,
                        .uv = {.scale = context.game.enemy.sprites[action].uv.scale, .offset = context.game.enemy.sprites[action].uv.offset}
                    },
                    .pos = context.game.enemy.pos,
                    .size = context.game.enemy.sprites[action].size,
                    .rot = context.game.enemy.sprites[action].rot,
                    .color = context.game.enemy.sprites[action].color,
                    .dissolve = context.game.enemy.alive ? 0.0f : context.game.enemy.effects[0],
                    .zorder = 2,
                    .flip = context.game.enemy.flip
                });

                if (context.game.enemy.action != ACTOR_ACTION_f64_JUMP && context.game.enemy.action != ACTOR_ACTION_DEATH) {
                    renderer_frame_command_push(&context.renderer, (command_t) {
                        .type = COMMAND_TYPE_SPRITE,
                        .data.sprite = {
                            .texture = context.game.level.sprites[1].texture,
                            .uv = {.scale = context.game.level.sprites[1].uv.scale, .offset = context.game.level.sprites[1].uv.offset}
                        },
                        .pos = vec2(context.game.enemy.flip ? (context.game.enemy.coll.min.x - (GAME_ACTOR_SPRITE_SCALE * 18.0f)) : context.game.enemy.coll.max.x, context.game.enemy.coll.max.y - (GAME_ACTOR_SPRITE_SCALE * (context.game.enemy.crouched ? 4.0f: 16.0f))),
                        .size = context.game.level.sprites[1].size,
                        .rot = context.game.level.sprites[1].rot,
                        .color = context.game.level.sprites[1].color,
                        .dissolve = 0.0f,
                        .zorder = 2,
                        .flip = context.game.enemy.flip
                    });
                }
            }

            // TEXT
            command_t command = {0};
            command.type = COMMAND_TYPE_TEXT;
            command.data.text.scale = 2.0f;
            command.color = vec3(1.0f, 1.0f, 1.0f);
            command.zorder = 4;
            command.flip = 0;

            command.pos = vec2(16.0f, (f32) (WINDOW_HEIGHT - 16.0f));
            sprintf(command.data.text.content, "GAME TURN %u", context.game.level.turn);
            renderer_frame_command_push(&context.renderer, command);

            command.pos = vec2(16.0f, (f32) (WINDOW_HEIGHT - 32.0f));
            sprintf(command.data.text.content, "PLAYER    %u/%u [K/D]", context.game.player.kills, context.game.player.deaths);
            renderer_frame_command_push(&context.renderer, command);

            command.pos = vec2(16.0f, (f32) (WINDOW_HEIGHT - 48.0f));
            sprintf(command.data.text.content, "ENEMY     %u/%u [K/D]", context.game.enemy.kills, context.game.enemy.deaths);
            renderer_frame_command_push(&context.renderer, command);

            u16 offset = 0;
            if (context.clock.framerate.value >= 10000) offset = (9.0f * FONT_WIDTH * 2.0f) + 16.0f;
            else if (context.clock.framerate.value >= 1000 && context.clock.framerate.value < 10000) offset = (8.0f * FONT_WIDTH * 2.0f) + 16.0f;
            else if (context.clock.framerate.value >= 100 && context.clock.framerate.value < 1000) offset = (7.0f * FONT_WIDTH * 2.0f) + 16.0f;
            else if (context.clock.framerate.value < 100) offset = (6.0f * FONT_WIDTH * 2.0f) + 16.0f;

            command.pos = vec2((f32) WINDOW_WIDTH - offset, (f32) (WINDOW_HEIGHT - 16.0f));
            sprintf(command.data.text.content, "FPS %u", context.clock.framerate.value);
            renderer_frame_command_push(&context.renderer, command);

            if (context.game.state == GAME_STATE_PAUSE) {
                command.pos = vec2(((f32) (WINDOW_WIDTH * 0.5f) - (9.0f * FONT_WIDTH * 2.0f)), (f32) WINDOW_HEIGHT * 0.5f);
                sprintf(command.data.text.content, "PRESS 'R' TO START"); // or resume
                renderer_frame_command_push(&context.renderer, command);
            }

            renderer_draw(&context.renderer);
            renderer_frame_clear(&context.renderer);

            // OPENGL
            glfwSwapBuffers(context.sys.window);
        }

        // OPENGL
        glfwPollEvents();

    }
}

void game_stop(void) { // first part is probably redundant
    renderer_destroy(&context.renderer);

    for (u32 i = 0; i < GAME_RESOURCES_TEXTURE_ARRAY_SIZE; i++) {
        if (context.res.textures[i].id) texture_destroy(&context.res.textures[i]);
    }

    for (u32 i = 0; i < GAME_RESOURCES_SHADER_ARRAY_SIZE; i++) {
        if (context.res.shaders[i].program) shader_destroy(&context.res.shaders[i]);
    }

    glfwDestroyWindow(context.sys.window);
    glfwTerminate();
}

// MAIN

int main(void) {
    game_init();
    game_update();
    game_stop();

    return 0;
}