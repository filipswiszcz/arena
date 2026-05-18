#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <libmath/math.h>
#include <libmem/mem.h>

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
#define WINDOW_NAME "BattleArena 2D (Build v0.0.15)"

#define ASSERT(_e, ...) if (!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1);}

// UTIL

float random(void) {
    static uint8_t init = 0;
    if (!init) {srand((unsigned) time(NULL)); init = 1;}
    return (float) rand() / ((float) RAND_MAX + 1.0f);
}

// SHADER

typedef struct {
    uint32_t ids[2];
    uint32_t program;
} shader_t;

void _shader_read(char **code, char *path) {
    FILE *file = fopen(path, "rb");
    ASSERT(file != NULL, "FILE_READ_ERROR: %s\n", path);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    *code = (char*) malloc(size + 1);
    fread(*code, 1, size, file);
    (*code)[size] = '\0';
    
    fclose(file);
}

void _shader_compile(uint32_t *id, uint32_t type, char *code) {
    *id = glCreateShader(type);

    glShaderSource(*id, 1, (const char**) &code, NULL);
    glCompileShader(*id);

    free(code);
 
    int32_t params;
    glGetShaderiv(*id, GL_COMPILE_STATUS, &params);
    if (params == 0) {
        char log[512];
        glGetShaderInfoLog(*id, 512, NULL, log);
        printf("SHADER_COMPILE_ERROR: %s\n", log);
        return;
    }
}

void shader_init(shader_t *shader, char *vertpath, char *fragpath) {
    char *vertcode, *fragcode;
    _shader_read(&vertcode, vertpath);
    _shader_read(&fragcode, fragpath);

    _shader_compile(&shader->ids[0], GL_VERTEX_SHADER, vertcode);
    _shader_compile(&shader->ids[1], GL_FRAGMENT_SHADER, fragcode);

    shader->program = glCreateProgram();

    glAttachShader(shader->program, shader->ids[0]);
    glAttachShader(shader->program, shader->ids[1]);

    glLinkProgram(shader->program);
}

void shader_use(shader_t *shader) {
    glUseProgram(shader->program);
}

void shader_set_int(shader_t *shader, char *name, int val) {
    glUniform1i(glGetUniformLocation(shader->program, name), val);
}

void shader_set_uint(shader_t *shader, char *name, unsigned int val) {
    glUniform1ui(glGetUniformLocation(shader->program, name), val);
}

void shader_set_float(shader_t *shader, char *name, float val) {
    glUniform1f(glGetUniformLocation(shader->program, name), val);
}

void shader_set_vec2(shader_t *shader, char *name, vec2_t vec) {
    glUniform2f(glGetUniformLocation(shader->program, name), vec.x, vec.y);
}

void shader_set_vec3(shader_t *shader, char *name, vec3_t vec) {
    glUniform3f(glGetUniformLocation(shader->program, name), vec.x, vec.y, vec.z);
}

void shader_set_mat4(shader_t *shader, char *name, mat4_t mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader->program, name), 1, GL_FALSE, &mat.m[0][0]);
}

// TEXTURE

typedef struct {
    uint32_t id;
    int32_t width, height;
    int32_t format;
} texture_t;

void texture_init(texture_t *texture, char *path) {
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(1);
    
    int32_t channels;
    unsigned char *pixels = stbi_load(path, &texture->width, &texture->height, &channels, 0);
    ASSERT(pixels, "TEXTURE_READ_ERROR: %s\n", path);

    switch (channels) {
        case 1: {texture->format = GL_RED; break;}
        case 3: {texture->format = GL_RGB; break;}
        case 4: {texture->format = GL_RGBA; break;}
    }

    glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->width, texture->height, 0, texture->format, GL_UNSIGNED_BYTE, pixels);
    // ?

    stbi_image_free(pixels);
}

void texture_bind(texture_t *texture) {
    glBindTexture(GL_TEXTURE_2D, texture->id);
}

// TEXT

#define FONT_WIDTH 6
#define FONT_HEIGHT 6

static char GLYPHS[128][FONT_WIDTH][FONT_HEIGHT] = {
    ['F'] = {
        {1, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    ['P'] = {
        {1, 1, 1, 0, 0},
        {1, 0, 0, 1, 0},
        {1, 0, 0, 1, 0},
        {1, 1, 1, 0, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
    },
    ['S'] = {
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 0},
        {1, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 0, 1},
        {1, 1, 1, 1, 0},
    },
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
    [':'] = {
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0},
    }
};

typedef struct {
    shader_t *shader;
    texture_t texture;
    uint32_t vao, vbo;
} text_t;

void text_init(text_t *text) {
    uint8_t bitmap[(FONT_WIDTH * 16) * (FONT_HEIGHT * 8)];
    memset(bitmap, 0, sizeof(bitmap));
    
    for (uint32_t i = 0; i < 128; i++) {
        uint32_t cpx = (i % 16) * FONT_WIDTH;
        uint32_t cpy = (i / 16) * FONT_HEIGHT;
        
        for (uint32_t j = 0; j < FONT_HEIGHT; j++) {
            for (uint32_t k = 0; k < FONT_WIDTH; k++) {
                if (GLYPHS[i][j][k]) {
                    uint32_t px = cpx + k;
                    uint32_t py = cpy + j;
                    uint32_t mrk = (py * (FONT_WIDTH * 16)) + px;
                    bitmap[mrk] = 255;
                }
            }
        }
    }

    glGenTextures(1, &text->texture.id);

    glBindTexture(GL_TEXTURE_2D, text->texture.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (FONT_WIDTH * 16), (FONT_HEIGHT * 8), 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

    glGenVertexArrays(1, &text->vao);
    glGenBuffers(1, &text->vbo);

    glBindVertexArray(text->vao);

    glBindBuffer(GL_ARRAY_BUFFER, text->vbo);
    glBufferData(GL_ARRAY_BUFFER, 256 * 6 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) (sizeof(vec2_t)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void text_draw(text_t *text, char *content, float x, float y, float scale) {
#ifdef _WIN32
    vec4_t vertices[128 * 6];
#else
    vec4_t vertices[strlen(content) * 6];
#endif
    uint32_t c = 0;

    float cx = x;
    for (uint32_t i = 0; i < strlen(content); i++) {
        float col = (float) (content[i] % 16);
        float row = (float) (content[i] / 16);

        float umin = col / 16.0f;
        float vmin = row / 8.0f;
        float umax = (col + 1.0f) / 16.0f;
        float vmax = (row + 1.0f) / 8.0f;

        float sx = cx;
        float sy = y;
        float w = FONT_WIDTH * scale;
        float h = FONT_HEIGHT * scale;

        vertices[c++] = vec4(sx, sy + h, umin, vmin);
        vertices[c++] = vec4(sx, sy, umin, vmax);
        vertices[c++] = vec4(sx + w, sy, umax, vmax);

        vertices[c++] = vec4(sx, sy + h, umin, vmin);
        vertices[c++] = vec4(sx + w, sy, umax, vmax);
        vertices[c++] = vec4(sx + w, sy + h, umax, vmin);

        cx += w;
    }

    glBindVertexArray(text->vao);
    glBindBuffer(GL_ARRAY_BUFFER, text->vbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, text->texture.id);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_TRIANGLES, 0, strlen(content) * 6);
    glBindVertexArray(0);
}

// SPRITE

// typedef struct {
//      char **name;

//     texture_t *texture;

//     struct {
//         vec2_t scale, offset;
//     } uv;

//     vec2_t pos, size; // local pos
//     float rot;

//     vec3_t color;

//     uint8_t zorder;
//     uint8_t mirror;
// } sprite_t;

// void sprite_init(sprite_t *sprite, texture_t *texture, vec2_t scale, vec2_t offset, vec2_t pos, vec2_t size, float rot, vec3_t color, uint8_t zorder, uint8_t mirror) {
//     sprite->texture = texture;
//     sprite->uv.scale = scale;
//     sprite->uv.offset = offset;
//     sprite->pos = pos;
//     sprite->size = size;
//     sprite->rot = rot;
//     sprite->color = color;
//     sprite->zorder = zorder;
//     sprite->mirror = mirror;
// }

typedef struct {
    texture_t *texture;
    vec2_t pos, size;
    float rot;
    vec2_t scale, offset;
    vec3_t color;
    uint8_t zorder, mirror;
} sprite_t;

void sprite_init(sprite_t *sprite, texture_t *texture, vec2_t pos, vec2_t size, float rot, vec2_t scale, vec2_t offset, vec3_t color, uint32_t zorder) {
    sprite->texture = texture;
    sprite->pos = pos;
    sprite->size = size;
    sprite->rot = rot;
    sprite->scale = scale;
    sprite->offset = offset;
    sprite->color = color;
    sprite->zorder = zorder;
    sprite->mirror = 0;
}

// RENDERER

typedef struct command {
    texture_t *texture;

    struct {
        vec2_t scale, offset;
    } uv;

    vec2_t pos, size;
    float rot;

    vec3_t color;

    uint8_t zorder;
    uint8_t mirror;
} command_t;

typedef struct renderer {

    struct {
        command_t *commands;
        uint32_t counter;
    } frame;
    
    shader_t *shader;
    uint32_t vao, vbo;

    struct {
        shader_t *shaders[2]; // crt, glitch
        texture_t textures[2];
        uint32_t fbos[2];
    } postprocessing;

    text_t *texts;

} renderer_t;

void _renderer_postprocess_init(renderer_t *renderer) {
    for (uint32_t i = 0; i < 2; i++) {
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
    renderer->postprocessing.shaders[0] = &shaders[1]; // crt
    renderer->postprocessing.shaders[1] = &shaders[3]; // glitch
    // renderer->texts[0].shader = &shaders[2]; // text

    vec4_t vertices[] = {
        vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(1.0f, 0.0f, 1.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), 
        vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(1.0f, 0.0f, 1.0f, 0.0f)
    };
    
    glGenVertexArrays(1, &renderer->vao);
    glGenBuffers(1, &renderer->vbo);

    glBindVertexArray(renderer->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // POST-PROCESSING
    _renderer_postprocess_init(renderer);

    // TEXTS
    // alloc text array

    // PROJECTION
    mat4_t projection = mat4_ortho(0.0f, (float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    shader_use(renderer->shader); // sprite
    shader_set_mat4(renderer->shader, "u_Projection", projection);

    // shader_use(renderer->texts[0].shader); // text
    // shader_set_mat4(renderer->texts[0].shader, "u_Projection", projection);

    shader_use(&shaders[2]); // text
    shader_set_mat4(&shaders[2], "u_Projection", projection);

}

uint8_t renderer_frame_command_push(renderer_t *renderer, command_t command) {
    if (renderer->frame.counter >= 16) return 0;
    renderer->frame.commands[renderer->frame.counter++] = command;
    return 1;
}

void renderer_frame_clear(renderer_t *renderer) { // i have to find out, if it could work that way
    renderer->frame.counter = 0;
}

void _renderer_sprite_draw(renderer_t *renderer, texture_t *texture, vec4_t uv, vec4_t trans, float rot, vec3_t color, uint8_t mirror, float time) { // time is temp here (store it in renderer?)
    
    mat4_t model = mat4(1.0f);
    model = mat4_trans(model, vec3(trans.x, trans.y, 0.0f));
    model = mat4_trans(model, vec3(trans.z * 0.5f, trans.w * 0.5f, 0.0f)); // what does it do? cant remember
    model = mat4_trans(model, vec3(trans.z * (-0.5f), trans.w * (-0.5f), 0.0f)); // same here?
    model = mat4_scale(model, vec3(trans.z, trans.w, 1.0f));
    // rotation??

    shader_set_mat4(renderer->shader, "u_Model", model);

    shader_set_vec2(renderer->shader, "u_Scale", vec2(uv.x, uv.y));
    shader_set_vec2(renderer->shader, "u_Offset", vec2(uv.z, uv.w));

    shader_set_int(renderer->shader, "u_Mirror", mirror);

    shader_set_vec3(renderer->shader, "u_Color", color);

    shader_set_float(renderer->shader, "u_GlitchTime", time);
    shader_set_float(renderer->shader, "u_GlitchIntensity", 0.1f);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(texture);

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

}

void _renderer_postprocess_draw(renderer_t *renderer) {

    uint32_t mrk = 0;
    for (uint32_t i = 0; i < 2; i++) { // loop postprocess shaders
        if (i == 1) glBindFramebuffer(GL_FRAMEBUFFER, 0); // last loop
        else glBindFramebuffer(GL_FRAMEBUFFER, renderer->postprocessing.fbos[mrk ? 0 : 1]);

        glClear(GL_COLOR_BUFFER_BIT);

        // shader_use(renderer->postprocessing.shaders[i]);

        // here set uniforms for specific shader
        if (i == 0) { // crt
            shader_use(renderer->postprocessing.shaders[i]);
            shader_set_int(renderer->postprocessing.shaders[i], "u_Texture", 0);
            shader_set_uint(renderer->postprocessing.shaders[i], "u_Lines", WINDOW_HEIGHT);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Bleed", 0.002f);
            shader_set_float(renderer->postprocessing.shaders[i], "u_Vignette", 0.8f);
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

void renderer_draw(renderer_t *renderer, float time) { // store time in renderer?

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->postprocessing.fbos[0]);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader_use(renderer->shader);
    for (uint32_t i = 0; i < renderer->frame.counter; i++) {
        _renderer_sprite_draw(
            renderer, 
            renderer->frame.commands[i].texture, 
            vec4(renderer->frame.commands[i].uv.scale.x, renderer->frame.commands[i].uv.scale.y, renderer->frame.commands[i].uv.offset.x, renderer->frame.commands[i].uv.offset.y), 
            vec4(renderer->frame.commands[i].pos.x, renderer->frame.commands[i].pos.y, renderer->frame.commands[i].size.x, renderer->frame.commands[i].size.y), 
            renderer->frame.commands[i].rot, 
            renderer->frame.commands[i].color, 
            renderer->frame.commands[i].mirror, 
            time // temp
        );        
    }

    // render text
        // if i want to postprocess text as well

    _renderer_postprocess_draw(renderer);

        // if not, then here
    
}

// GAME

#define GAME_GRAV -980.0f

typedef struct {
    vec2_t vel, force;
    float mass, grav;
    float fric, drag;
    float bounce;
} rigidbody_t;

#define GAME_COLL_NONE 0
#define GAME_COLL_TOP (1 << 0)
#define GAME_COLL_LEFT (1 << 1)
#define GAME_COLL_BOTTOM (1 << 2)
#define GAME_COLL_RIGHT (1 << 3)

typedef struct {
    vec2_t min, max;
    uint8_t side;
} collider_t;

typedef struct {
    vec2_t min, max; // offsets to parent pos
    vec2_t velocity;
    uint8_t lock, impact;
    uint8_t owner;
} collision_box_t;

// TODO make collision detection better
    // until there is a south collision, velocity should be constantly (-ffs, 0?);
    // ffs = free fall speed, 0? = it should always strive to reach a verticall fall

uint8_t collision_box_intersection(collision_box_t *a, collision_box_t *b) {
    return (((b->min.x <= a->max.x) && (b->max.x >= a->min.x)) && ((b->min.y <= a->max.y) && (b->max.y >= a->min.y)));
}

typedef enum {
    ACTOR_ACTION_IDLE = 0,
    ACTOR_ACTION_JUMP = 1,
    ACTOR_ACTION_DOUBLE_JUMP = 2,
    ACTOR_ACTION_RUN = 3,
    ACTOR_ACTION_CROUCH = 4
} actor_action_t; // change to bit flags

typedef struct {
    vec2_t position, size;
    float rotation;
    vec2_t clip, offset;
} actor_state_t;

typedef enum {
    ACTOR_ANIMATION_STEP_1 = 1,
    ACTOR_ANIMATION_STEP_2 = 2,
    ACTOR_ANIMATION_STEP_3 = 3,
    ACTOR_ANIMATION_STEP_4 = 4,
    ACTOR_ANIMATION_STEP_6 = 6
} actor_animation_step_t; // change to bit flags

typedef struct {
    actor_animation_step_t step;
    uint8_t tick, lock;
} actor_animation_t;

typedef struct {

    // temp
    vec2_t pos;
    rigidbody_t rigb;
    collider_t coll;

    uint8_t grounded;
    // temp

    vec2_t position;
    collision_box_t box;
    // sprites as array? and action == index (with pos relative to actor's pose?), pos
    sprite_t *sprites;
    // enum (or uint8_t) for current action (idle, jump, run, crouch)
    actor_action_t action;
    actor_animation_t animation;
    // actor_state_t cstate, pstate;
    uint16_t cooldown;
    uint8_t mirror;
    uint8_t fire; // it manages hand and gun | when 1, then hand and gun are going vert (animation)
} actor_t;

void game_actor_move(actor_t *actor, float xacc, uint8_t jump) {
    actor->rigb.force.x += (xacc * 1500.0f);
    (jump && actor->grounded) ? (void) (actor->rigb.vel.y = 400.f, actor->grounded = 0) : (void) 0;

    vec2_t acc = vec2(actor->rigb.force.x * actor->rigb.mass, (actor->rigb.force.y * actor->rigb.mass) + (GAME_GRAV * actor->rigb.grav));

    actor->rigb.vel.x += acc.x;
    actor->rigb.vel.y += acc.y;

    if (actor->grounded) actor->rigb.vel.x *= actor->rigb.fric;
    else {
        actor->rigb.vel.x *= actor->rigb.drag;
        // actor->rigb.vel.y *= actor->rigb.drag;
    }

    actor->rigb.vel.x = 0.0f;
    actor->rigb.vel.y = 0.0f;
}

// void game_actor_fire(actor_t *actor) {}

typedef struct {
    vec2_t pos;
    collision_box_t box;
    sprite_t sprite;
    // current animation
    uint8_t owner, used;
    uint8_t mirror;
} bullet_t;

// what i need:
// - a list with all textures (mem arena)
//      - do i run through assets/ and load them all?
//      - how do i mark them? with a name? eg "punk_weapon_rest", "punk_weapon_point"
//      - path "assets/texture/actor/punk_idle.png"
// - a level struct
//      - animated background
//      - floor (rigid body) + void
//      - animated finish texts (like in mortal combat)
//      - list of all projectiles>
//          - level checks if actor is hit by any bullet from enemy actor?
// - an actor struct
//      - animated body (with hands and weapon)
//      - collision and movement physics?
//      - all ML data

#define GAME_SIMULATION_FIXED_TIMESTEP 1.0f / 60.0f // 60 fps
#define GAME_ANIMATION_FIXED_TIMESTEP 1.0f / 8.0f // 8 fps

#define GAME_MEMORY_CAPACITY (64 * 1024 * 1024) // 64 MB
uint8_t GAME_MEMORY[GAME_MEMORY_CAPACITY];

#define GAME_RESOURCES_SHADERS_SIZE 4
#define GAME_RESOURCES_TEXTURES_SIZE 16

#define GAME_RENDERER_COMMANDS_SIZE 16

#define GAME_ACTOR_SPRITES_SIZE 8
#define GAME_ACTOR_SPRITE_SCALE 2

#define GAME_LEVEL_SPRITES_SIZE 4
#define GAME_LEVEL_TEXTS_SIZE 2
#define GAME_LEVEL_BULLETS_MAX 8
#define GAME_LEVEL_BULLET_SPEED 8

typedef enum {
    GAME_STATE_LOAD = 0,
    GAME_STATE_PAUSE = 1,
    GAME_STATE_PLAY = 2
} game_state_t;

static struct {

    struct {
        GLFWwindow *window;
        int32_t keys[512];
    } platform;

    struct {
        double time_of_last_frame;
        double time_between_frames;

        struct {
            double accumulator;
        } physics;

        struct {
            double accumulator;
        } animation;

        struct {
            double value;
            double timer;
            uint32_t counter;
            text_t text;
        } framerate;

    } ticker;

    // struct {} metrics;

    mem_arena_t arena;

    struct {
        shader_t *shaders;
        texture_t *textures;
    } resources;

    renderer_t renderer;

    struct {
        game_state_t state;

        struct {

            sprite_t *sprites;
            // // temp
            collision_box_t box;
            // // end of temp

            text_t *texts;

            bullet_t *bullets;

        } level;

        actor_t player;
        actor_t enemy;
    } game;

} context;

void _game_win32_icon_init(void) {
    char *path = "assets/icon.png";
    int32_t width, height, channels;
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 0);
    ASSERT(pixels, "ICON_READ_ERROR: %s", path);

    GLFWimage images[1] = {(GLFWimage) {.width = width, .height = height, .pixels = pixels}};
    glfwSetWindowIcon(context.platform.window, 1, images);

    stbi_image_free(pixels);
}

void _game_macos_icon_init(void) {}

void _game_keyboard_callback(GLFWwindow *window, int32_t key, int32_t scan, int32_t action, int32_t mode) {
    if (key > -1 && key < 512) {
        if (action == GLFW_PRESS) context.platform.keys[key] = 1;
        else if (action == GLFW_RELEASE) context.platform.keys[key] = 0;
    }
}

void _game_keyboard_handle(void) { // velocity needs to look for collision as well (currently it just goes beyond it)
    if (context.game.state != GAME_STATE_PLAY) return;
    
    if (context.platform.keys[GLFW_KEY_ESCAPE]) {glfwSetWindowShouldClose(context.platform.window, 1);}

    // player
    if (!context.platform.keys[GLFW_KEY_W] && !context.platform.keys[GLFW_KEY_S] && !context.platform.keys[GLFW_KEY_A] && !context.platform.keys[GLFW_KEY_D]) {
        if (context.game.player.action != ACTOR_ACTION_IDLE) {
            context.game.player.action = ACTOR_ACTION_IDLE;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.game.player.animation.tick = 0;
        }
    }

    // enemy
    if (!context.platform.keys[GLFW_KEY_UP] && !context.platform.keys[GLFW_KEY_DOWN] && !context.platform.keys[GLFW_KEY_LEFT] && !context.platform.keys[GLFW_KEY_RIGHT]) {
        if (context.game.enemy.action != ACTOR_ACTION_IDLE) {
            context.game.enemy.action = ACTOR_ACTION_IDLE;
            context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_4;
            context.game.enemy.animation.tick = 0;
        }
    }
    
    // player
    if (context.platform.keys[GLFW_KEY_W]) {
        if (context.game.player.box.lock == 0) {
            context.game.player.box.velocity.y = 16.0f;
            context.game.player.box.lock = 1;
            context.game.player.box.impact = 0;
        } else if (context.game.player.box.lock == 2 && !context.game.player.box.impact) {
            context.game.player.box.velocity.y = 18.0f;
            context.game.player.box.lock = 3;
            if (context.game.player.action != ACTOR_ACTION_DOUBLE_JUMP) {
                context.game.player.action = ACTOR_ACTION_DOUBLE_JUMP;
                context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
                context.game.player.animation.tick = 0;
            }
        }
    }
    if (!context.platform.keys[GLFW_KEY_W]) {
        if (context.game.player.box.lock == 1) context.game.player.box.lock = 2;
    }

    // enemy
    if (context.platform.keys[GLFW_KEY_UP]) {
        if (context.game.enemy.box.lock == 0) {
            context.game.enemy.box.velocity.y = 12.0f;
            context.game.enemy.box.lock = 1;
            context.game.enemy.box.impact = 0;
        } else if (context.game.enemy.box.lock == 2 && !context.game.enemy.box.impact) {
            context.game.enemy.box.velocity.y = 16.0f;
            context.game.enemy.box.lock = 3;
        }
    }
    if (!context.platform.keys[GLFW_KEY_UP]) {
        if (context.game.enemy.box.lock == 1) context.game.enemy.box.lock = 2;
    }

    // player
    if (context.platform.keys[GLFW_KEY_A] && !context.platform.keys[GLFW_KEY_S]) {
        if ((context.game.player.position.x - 4.0f) >= 0) context.game.player.box.velocity.x = -4.0f;
        // ANIMATION
        context.game.player.mirror = 1;
        if (context.game.player.action != ACTOR_ACTION_RUN) {
            context.game.player.action = ACTOR_ACTION_RUN;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
            context.game.player.animation.tick = 0;
        }
    }

    if (context.platform.keys[GLFW_KEY_D] && !context.platform.keys[GLFW_KEY_S]) {
        if ((context.game.player.position.x + 4.0f) <= (WINDOW_WIDTH - (context.game.player.sprites[context.game.player.action].size.x / 2))) context.game.player.box.velocity.x = 4.0f;
        // ANIMATION
        context.game.player.mirror = 0;
        if (context.game.player.action != ACTOR_ACTION_RUN) {
            context.game.player.action = ACTOR_ACTION_RUN;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
            context.game.player.animation.tick = 0;
        }
    }

    if (!context.platform.keys[GLFW_KEY_A] && !context.platform.keys[GLFW_KEY_D]) {
        context.game.player.box.velocity.x = 0.0f;
    }

    // enemy
    if (context.platform.keys[GLFW_KEY_LEFT] && !context.platform.keys[GLFW_KEY_DOWN]) {
        if ((context.game.enemy.position.x - 4.0f) >= 0) context.game.enemy.box.velocity.x = -4.0f;
        // ANIMATION
        context.game.enemy.mirror = 1;
        if (context.game.enemy.action != ACTOR_ACTION_RUN) {
            context.game.enemy.action = ACTOR_ACTION_RUN;
            context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_6;
            context.game.enemy.animation.tick = 0;
        }
    }

    if (context.platform.keys[GLFW_KEY_RIGHT] && !context.platform.keys[GLFW_KEY_DOWN]) {
        if ((context.game.enemy.position.x + 4.0f) <= (WINDOW_WIDTH - (context.game.enemy.sprites[context.game.enemy.action].size.x / 2))) context.game.enemy.box.velocity.x = 4.0f;
        // ANIMATION
        context.game.enemy.mirror = 0;
        if (context.game.enemy.action != ACTOR_ACTION_RUN) {
            context.game.enemy.action = ACTOR_ACTION_RUN;
            context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_6;
            context.game.enemy.animation.tick = 0;
        }
    }

    // preposition test
    // vec2_t preposition = vec2_add(context.game.player.position, context.game.player.box.velocity);
    // vec2_t premin = vec2(preposition.x, preposition.y);
    // vec2_t premax = vec2(preposition.x + 192, preposition.y + 192);
    // end of preposition test

    // TODO i need to find a way to stop velocity before or right on collision box border

    // player
    // context.game.player.position = vec2_add(context.game.player.position, context.game.player.box.velocity);
    // context.game.player.box.min = vec2(context.game.player.position.x, context.game.player.position.y);
    // context.game.player.box.max = vec2(context.game.player.position.x + (GAME_ACTOR_SPRITE_SCALE * 48.0f), context.game.player.position.y + (GAME_ACTOR_SPRITE_SCALE * 48.0f));

    if (context.platform.keys[GLFW_KEY_S]) {
        context.game.player.box.max.y = context.game.player.position.y + (GAME_ACTOR_SPRITE_SCALE * 24.0f);
        // ANIMATION
        context.game.player.action = ACTOR_ACTION_CROUCH;
        context.game.player.animation.step = ACTOR_ANIMATION_STEP_1;
        context.game.player.animation.tick = 2;
    }

    // enemy
    context.game.enemy.position = vec2_add(context.game.enemy.position, context.game.enemy.box.velocity);
    context.game.enemy.box.min = vec2(context.game.enemy.position.x, context.game.enemy.position.y);
    context.game.enemy.box.max = vec2(context.game.enemy.position.x + (GAME_ACTOR_SPRITE_SCALE * 48.0f), context.game.enemy.position.y + (GAME_ACTOR_SPRITE_SCALE * 48.0f));

    if (context.platform.keys[GLFW_KEY_DOWN]) {
        context.game.enemy.box.max.y = context.game.enemy.position.y + (GAME_ACTOR_SPRITE_SCALE * 24.0f);
        // ANIMATION
        // context.game.enemy.action = ACTOR_ACTION_CROUCH;
        // context.game.enemy.animation.step = ACTOR_ANIMATION_STEP_1;
        // context.game.enemy.animation.tick = 2;
    }

    // player
    // if (collision_box_intersection(&context.game.player.box, &context.game.level.box)) {
    //     context.game.player.box.velocity = vec2(0.0f, 0.0f);
    //     if (context.game.player.box.lock) context.game.player.box.lock = 0; 
    //     context.game.player.box.impact = 1;
    //     // printf("PLAYER_COLLISION\n");
    // } else {
    //     context.game.player.box.velocity.y += -1;
    //     // printf("PLAYER_VELOCITY={x=%f, y=%f}\n", context.game.player.box.velocity.x, context.game.player.box.velocity.y);
    // }

    // enemy
    if (collision_box_intersection(&context.game.enemy.box, &context.game.level.box)) {
        context.game.enemy.box.velocity = vec2(0.0f, 0.0f);
        if (context.game.enemy.box.lock) context.game.enemy.box.lock = 0; 
        context.game.enemy.box.impact = 1;
        // printf("ENEMY_COLLISION\n");
    } else {
        context.game.enemy.box.velocity.y += -1;
        // printf("ENEMY_VELOCITY={x=%f, y=%f}\n", context.game.enemy.box.velocity.x, context.game.enemy.box.velocity.y);
    }

    // player
    if (context.platform.keys[GLFW_KEY_F]) {
        if (context.game.player.cooldown <= 0) {

            vec2_t pos = vec2(context.game.player.box.max.x + (0.0f * GAME_ACTOR_SPRITE_SCALE), context.game.player.box.max.y - (12.0f * GAME_ACTOR_SPRITE_SCALE)); // from pos of the gun

            bullet_t bullet = {
                .pos = pos, // pos and box has to be affected by mirror
                .box = (collision_box_t) {.min = pos, .max = vec2(pos.x + (8.0f * GAME_ACTOR_SPRITE_SCALE), pos.y + (4.0f * GAME_ACTOR_SPRITE_SCALE)), .velocity = vec2(context.game.player.mirror ? -8.0f : 8.0f, -0.05f), .lock = 0, .impact = 0},
                .sprite = context.game.level.sprites[1],
                .owner = 0,
                .used = 1,
                .mirror = context.game.player.mirror
            };

            for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) { // is uint32_t bad as a loop index?
                if (!context.game.level.bullets[i].used) {
                    context.game.level.bullets[i] = bullet;
                    break;
                }
            }

            context.game.player.cooldown = 64;
        }
    }

    // enemy
    if (context.platform.keys[GLFW_KEY_ENTER]) {
        if (context.game.enemy.cooldown <= 0) {

            vec2_t pos = vec2(context.game.enemy.box.max.x + (0.0f * GAME_ACTOR_SPRITE_SCALE), context.game.enemy.box.max.y - (12.0f * GAME_ACTOR_SPRITE_SCALE));

            bullet_t bullet = {
                .pos = pos,
                .box = (collision_box_t) {.min = pos, .max = vec2(pos.x + (8.0f * GAME_ACTOR_SPRITE_SCALE), pos.y + (4.0f * GAME_ACTOR_SPRITE_SCALE)), .velocity = vec2(context.game.enemy.mirror ? -8.0f : 8.0f, -0.05f), .lock = 0, .impact = 0},
                .sprite = context.game.level.sprites[1],
                .owner = 1,
                .used = 1,
                .mirror = context.game.enemy.mirror
            };

            for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) {
                if (!context.game.level.bullets[i].used) {
                    context.game.level.bullets[i] = bullet;
                    break;
                }
            }

            context.game.enemy.cooldown = 64;
        }
    }

    if (context.game.player.cooldown > 0) context.game.player.cooldown--;
    if (context.game.enemy.cooldown > 0) context.game.enemy.cooldown--;

    for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) {
        if (context.game.level.bullets[i].used) {
            if (context.game.level.bullets[i].pos.x < 0 || context.game.level.bullets[i].pos.x > WINDOW_WIDTH) {
                context.game.level.bullets[i].used = 0;
            } else {
                context.game.level.bullets[i].pos = vec2_add(context.game.level.bullets[i].pos, context.game.level.bullets[i].box.velocity);
                context.game.level.bullets[i].box.min = vec2_add(context.game.level.bullets[i].box.min, context.game.level.bullets[i].box.velocity);
                context.game.level.bullets[i].box.max = vec2_add(context.game.level.bullets[i].box.max, context.game.level.bullets[i].box.velocity);
            }
        }
    }
   
}

void _game_keyboard_qhandle(void) {

    float xacc = 0.0f;
    uint8_t jump = 0;

    // KEY W
    if (context.platform.keys[GLFW_KEY_W]) jump = 1;

    // KEY A
    if (context.platform.keys[GLFW_KEY_A]) xacc -= 1.0f;

    // KEY S
    if (context.platform.keys[GLFW_KEY_S]) {}

    // KEY D
    if (context.platform.keys[GLFW_KEY_D]) xacc += 1.0f;

    game_actor_move(&context.game.player, xacc, jump);
}

void _game_physics_handle(actor_t *actor, collision_box_t *box) {
    if (context.game.state != GAME_STATE_PLAY) return;

    // if (actor->box.impact && actor->box.velocity.x == 0 && actor->box.velocity.y == 0) return;

    vec2_t prepos = vec2_add(actor->position, actor->box.velocity);
    vec2_t premin = vec2(prepos.x, prepos.y);
    vec2_t premax = vec2(prepos.x + (48.0f * GAME_ACTOR_SPRITE_SCALE), prepos.y + (48.0f * GAME_ACTOR_SPRITE_SCALE));

    int8_t coll = (((premax.x >= box->min.x) && (premin.x <= box->max.x)) 
        && ((premax.y >= box->min.y) && (premin.y <= box->max.y)));

    if (coll) {

        vec2_t acpos = vec2(premin.x + ((premax.x - premin.x) / 2), premin.y + ((premax.y - premin.y) / 2));
        vec2_t bcpos = vec2(box->min.x + ((box->max.x - box->min.x) / 2), box->min.y + ((box->max.y - box->min.y) / 2));

        float opp = bcpos.y - acpos.y;
        float adj = bcpos.x - acpos.x;

        float alpha = 180.0f - fmodf((roundf(90.0f - float_deg(atan2f(opp, adj))) + 180.0f), 360.0f);
        // printf("alpha=%f\n", alpha);

        float beta = float_deg(atanf((bcpos.x - box->min.x) / (box->max.y - bcpos.y)));
        // printf("beta=%f\n", beta);

        if (alpha < ((180.0f - beta) * -1) || alpha > (180.0f - beta)) {
            if (box->owner) printf("event=COLL_TOP\n");

            actor->box.velocity.y = 0.0f;
            actor->box.lock = 0;
            actor->box.impact = 1;

            // float y = actor->position.y - box->max.y;
            // if (y > 0) actor->box.velocity.y = (y * -1.0f);
            // else {
            //     actor->box.velocity.y = 0.0f;
            //     actor->box.lock = 0;
            //     actor->box.impact = 1;
            // }

        } else if (alpha < (beta * -1) && alpha > ((180.0f - beta) * -1)) {
            if (box->owner) printf("event=COLL_LEFT\n");

            actor->box.velocity.x = 0.0f;
            actor->box.lock = 0;
            actor->box.impact = 1;

            // float x = box->min.x - actor->position.x;
            // if (x > 0) actor->box.velocity.x = (x * -1.0f);
            // else {
            //     actor->box.velocity.x = 0.0f;
            //     actor->box.lock = 0;
            //     actor->box.impact = 1;
            // }

        } else if (alpha < beta && alpha > (beta * -1)) {
            if (box->owner) printf("event=COLL_BOTTOM\n");

            // ..

        } else if (alpha < (180.0f - beta) && alpha > beta) {
            if (box->owner) printf("event=COLL_RIGHT\n");

            actor->box.velocity.x = 0.0f;
            actor->box.lock = 0;
            actor->box.impact = 1;

        }

    } else {
        if (actor->box.impact) return;
        actor->box.velocity.y += -1.0f;
        printf("event=NO_COLL\n");
    }

}

void game_init(void) {

    // GLFW
    ASSERT(glfwInit(), "OPENGL_INIT_ERROR\n");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    context.platform.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
    ASSERT(context.platform.window, "GLFW_WINDOW_CREATE_ERROR\n");

    glfwMakeContextCurrent(context.platform.window);
    glfwSetKeyCallback(context.platform.window, _game_keyboard_callback);
    // glfwSetInputMode(context.platform.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1;
    int32_t glew_error = glewInit();
    ASSERT(glew_error == 0 || glew_error == 4, "GLEW_INIT_ERROR\n"); // this needs to be rethinked
#endif

    // OPENGL
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // ICON
#ifdef _WIN32
    _game_win32_icon_init();
#else
    //..
#endif

    // MEMORY
    mem_arena_init(&context.arena, &GAME_MEMORY, GAME_MEMORY_CAPACITY);

    // RESOURCES
    context.resources.shaders = mem_arena_alloc(&context.arena, GAME_RESOURCES_SHADERS_SIZE * sizeof(shader_t));
    shader_init(&context.resources.shaders[0], "shader/sprite.vs", "shader/sprite.fs");
    shader_init(&context.resources.shaders[1], "shader/crt.vs", "shader/crt.fs");
    shader_init(&context.resources.shaders[2], "shader/text.vs", "shader/text.fs");
    shader_init(&context.resources.shaders[3], "shader/glitch.vs", "shader/glitch.fs");

    // TODO read it auto, and search it by name (only in init)
    context.resources.textures = mem_arena_alloc(&context.arena, GAME_RESOURCES_TEXTURES_SIZE * sizeof(texture_t));
    texture_init(&context.resources.textures[0], "assets/texture/level/tile.png");

    texture_init(&context.resources.textures[1], "assets/texture/player/body/punk_idle.png");
    texture_init(&context.resources.textures[2], "assets/texture/player/body/punk_jump.png");
    texture_init(&context.resources.textures[3], "assets/texture/player/body/punk_doublejump.png");
    texture_init(&context.resources.textures[4], "assets/texture/player/body/punk_run.png");
    texture_init(&context.resources.textures[5], "assets/texture/player/body/punk_crouch.png");
    texture_init(&context.resources.textures[6], "assets/texture/player/body/punk_attack.png");
    texture_init(&context.resources.textures[7], "assets/texture/player/body/punk_death.png");

    texture_init(&context.resources.textures[8], "assets/texture/enemy/body/cyborg_idle.png"); 
    texture_init(&context.resources.textures[9], "assets/texture/enemy/body/cyborg_jump.png");
    texture_init(&context.resources.textures[10], "assets/texture/enemy/body/cyborg_doublejump.png");
    texture_init(&context.resources.textures[11], "assets/texture/enemy/body/cyborg_run.png");
    // texture_init(&context.resources.textures[12], "assets/texture/enemy/body/cyborg_crouch.png");
    texture_init(&context.resources.textures[13], "assets/texture/enemy/body/cyborg_attack.png");
    texture_init(&context.resources.textures[14], "assets/texture/enemy/body/cyborg_death.png");

    texture_init(&context.resources.textures[15], "assets/texture/bullet.png");

    // RENDERER
    context.renderer.frame.commands = mem_arena_alloc(&context.arena, GAME_RENDERER_COMMANDS_SIZE * sizeof(command_t));
    renderer_init(&context.renderer, context.resources.shaders);

    // GAME
    context.game.state = GAME_STATE_PLAY;
    context.game.level.sprites = mem_arena_alloc(&context.arena, GAME_LEVEL_SPRITES_SIZE * sizeof(sprite_t));
    context.game.level.texts = mem_arena_alloc(&context.arena, GAME_LEVEL_TEXTS_SIZE * sizeof(text_t));
    context.game.level.bullets = mem_arena_alloc(&context.arena, GAME_LEVEL_BULLETS_MAX * sizeof(bullet_t));
    for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) context.game.level.bullets[i] = (bullet_t) {0}; // is there a cooler way to init this?

    // level
    context.game.level.box = (collision_box_t) {.min = vec2(0.0f, 0.0f), .max = vec2((float) WINDOW_WIDTH, (WINDOW_HEIGHT / 12.0f)), .velocity = vec2(0.0f, 0.0f), .lock = 0, .impact = 1, .owner = 0};

    sprite_init(&context.game.level.sprites[0], &context.resources.textures[0], vec2(0.0f, 0.0f), vec2((float) WINDOW_WIDTH, 50.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // tile
    sprite_init(&context.game.level.sprites[1], &context.resources.textures[15], vec2(0.0f, 0.0f), vec2(6.0f * GAME_ACTOR_SPRITE_SCALE, 6.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // bullet
    // sprite_init(&context.game.player.sprites[16], &context.resources.textures[5], vec2(0.0f, 0.0f), vec2(24.0f, 16.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // gun

    // actor
    context.game.player.position = vec2(150.0f, 100.0f);
    context.game.player.box = (collision_box_t) {.min = vec2(150.0f, 100.0f), .max = vec2(342.0f, 292.0f), .velocity = vec2(0.0f, 0.0f), .lock = 0, .impact = 0, .owner = 0};
    context.game.player.sprites = mem_arena_alloc(&context.arena, GAME_ACTOR_SPRITES_SIZE * sizeof(sprite_t));
    context.game.player.action = ACTOR_ACTION_IDLE;
    context.game.player.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.game.player.mirror = 0;
    context.game.player.fire = 0;

    // sprite_init(&sprite, "punk_run", ..);
    sprite_init(&context.game.player.sprites[0], &context.resources.textures[1], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // idle
    sprite_init(&context.game.player.sprites[1], &context.resources.textures[2], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // jump
    sprite_init(&context.game.player.sprites[2], &context.resources.textures[3], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // double jump
    sprite_init(&context.game.player.sprites[3], &context.resources.textures[4], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // run
    sprite_init(&context.game.player.sprites[4], &context.resources.textures[5], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // crouch
    sprite_init(&context.game.player.sprites[5], &context.resources.textures[6], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // attack
    sprite_init(&context.game.player.sprites[6], &context.resources.textures[7], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // death

    context.game.enemy.position = vec2(450.0f, 100.0f);
    context.game.enemy.box = (collision_box_t) {.min = vec2(450.0f, 100.0f), .max = vec2(642.0f, 292.0f), .velocity = vec2(0.0f, 0.0f), .lock = 0, .impact = 0, .owner = 1};
    context.game.enemy.sprites = mem_arena_alloc(&context.arena, GAME_ACTOR_SPRITES_SIZE * sizeof(sprite_t));
    context.game.enemy.action = ACTOR_ACTION_IDLE;
    context.game.enemy.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.game.enemy.mirror = 1;
    context.game.enemy.fire = 0;

    sprite_init(&context.game.enemy.sprites[0], &context.resources.textures[8], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // idle
    sprite_init(&context.game.enemy.sprites[1], &context.resources.textures[9], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // jump
    sprite_init(&context.game.enemy.sprites[2], &context.resources.textures[10], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // double jump
    sprite_init(&context.game.enemy.sprites[3], &context.resources.textures[11], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // run
    // sprite_init(&context.game.enemy.sprites[4], &context.resources.textures[12], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // crouch
    sprite_init(&context.game.enemy.sprites[5], &context.resources.textures[13], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // attack
    sprite_init(&context.game.enemy.sprites[6], &context.resources.textures[14], vec2(0.0f, 0.0f), vec2(48.0f * GAME_ACTOR_SPRITE_SCALE, 48.0f * GAME_ACTOR_SPRITE_SCALE), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f), 0); // death

    // TICKER
    text_init(&context.ticker.framerate.text);
    context.ticker.time_of_last_frame = glfwGetTime();
    context.ticker.framerate.text.shader = &context.resources.shaders[2]; // text

    // printf("GAME_MEMORY: Used %.4f MB / %.2f MB (%.1f%%)\n", ((float) (context.arena.used) / (1024.0f * 1024.0f)), 
    //     ((float) (context.arena.capacity) / (1024.0f * 1024.0f)), (double) context.arena.used / (double) context.arena.capacity * 100.0);
}

void game_update(void) {
    while (!glfwWindowShouldClose(context.platform.window)) {

        // TICKER
        double current_time = glfwGetTime();
        context.ticker.time_between_frames = current_time - context.ticker.time_of_last_frame;
        context.ticker.time_of_last_frame = current_time;

        // framerate
        context.ticker.framerate.timer += context.ticker.time_between_frames;
        context.ticker.framerate.counter++;

        if (context.ticker.framerate.timer >= 1.0f) {
            context.ticker.framerate.value = (double) context.ticker.framerate.counter / context.ticker.framerate.timer;
            context.ticker.framerate.timer -= 1.0f;
            context.ticker.framerate.counter = 0;
        }

        // physics
        if (context.ticker.time_between_frames > 0.25f) {
            context.ticker.time_between_frames = 0.25f;
        }

        context.ticker.physics.accumulator += context.ticker.time_between_frames;

        while (context.ticker.physics.accumulator >= GAME_SIMULATION_FIXED_TIMESTEP) {
            
            // TODO save previous actors states

            // input
            // _game_keyboard_handle();
            _game_keyboard_qhandle();

            // _game_physics_handle(&context.game.player, &context.game.level.box);
            // _game_physics_handle(&context.game.player, &context.game.enemy.box);

            context.game.player.position = vec2_add(context.game.player.position, context.game.player.box.velocity);
            context.game.player.box.min = vec2(context.game.player.position.x, context.game.player.position.y);
            context.game.player.box.max = vec2(context.game.player.position.x + (GAME_ACTOR_SPRITE_SCALE * 48.0f), context.game.player.position.y + (GAME_ACTOR_SPRITE_SCALE * 48.0f));
            
            // temp
            // for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) { // is uint32_t bad as a loop index?
            //     if (context.game.level.bullets[i].used) {
            //         printf("BULLET_POS={x=%f, y=%f}\n", context.game.level.bullets[i].pos.x, context.game.level.bullets[i].pos.y);
            //         printf("BULLET_MIN={x=%f, y=%f}\n", context.game.level.bullets[i].box.min.x, context.game.level.bullets[i].box.min.y);
            //         printf("BULLET_MAX={x=%f, y=%f}\n", context.game.level.bullets[i].box.max.x, context.game.level.bullets[i].box.max.y);
            //     }
            // }
            // end of temp

            // animation
            context.ticker.animation.accumulator += GAME_SIMULATION_FIXED_TIMESTEP;

            while (context.ticker.animation.accumulator >= GAME_ANIMATION_FIXED_TIMESTEP) {
                context.ticker.animation.accumulator -= GAME_ANIMATION_FIXED_TIMESTEP;

                // player
                context.game.player.sprites[context.game.player.action].offset.x = (context.game.player.sprites[context.game.player.action].scale.x * context.game.player.animation.tick);
                if (context.game.player.animation.lock != 2 && context.game.player.animation.tick < context.game.player.animation.step) context.game.player.animation.tick++;
                else if (context.game.player.animation.lock != 2) context.game.player.animation.tick = 0;

                // enemy
                context.game.enemy.sprites[context.game.enemy.action].offset.x = (context.game.enemy.sprites[context.game.enemy.action].scale.x * context.game.enemy.animation.tick);
                if (context.game.enemy.animation.lock != 2 && context.game.enemy.animation.tick < context.game.enemy.animation.step) context.game.enemy.animation.tick++;
                else if (context.game.enemy.animation.lock != 2) context.game.enemy.animation.tick = 0;
            }

            // physics
            context.ticker.physics.accumulator -= GAME_SIMULATION_FIXED_TIMESTEP;
        }

        // double alpha = context.clock.accumulator / GAME_SIMULATION_FIXED_TIMESTEP;

        // RENDERER

        renderer_frame_command_push(&context.renderer, (command_t) {
            .texture = context.game.level.sprites[0].texture,
            .uv = {.scale = context.game.level.sprites[0].scale, .offset = context.game.level.sprites[0].offset},
            .pos = context.game.level.sprites[0].pos,
            .size = context.game.level.sprites[0].size,
            .rot = context.game.level.sprites[0].rot,
            .color = context.game.level.sprites[0].color,
            .zorder = 2,
            .mirror = 0
        });

        for (uint32_t i = 0; i < GAME_LEVEL_BULLETS_MAX; i++) {
            if (context.game.level.bullets[i].used) {
                renderer_frame_command_push(&context.renderer, (command_t) {
                    .texture = context.game.level.sprites[1].texture,
                    .uv = {.scale = context.game.level.sprites[1].scale, .offset = context.game.level.sprites[1].offset},
                    .pos = context.game.level.bullets[i].pos,
                    .size = context.game.level.sprites[1].size,
                    .rot = context.game.level.sprites[1].rot,
                    .color = context.game.level.sprites[1].color,
                    .zorder = 3,
                    .mirror = (context.game.level.bullets[i].owner ? context.game.enemy.mirror : context.game.player.mirror)
                });
            }
        }

        renderer_frame_command_push(&context.renderer, (command_t) {
            .texture = context.game.player.sprites[context.game.player.action].texture, 
            .uv = {.scale = context.game.player.sprites[context.game.player.action].scale, .offset = context.game.player.sprites[context.game.player.action].offset}, 
            .pos = vec2_add(context.game.player.position, context.game.player.sprites[context.game.player.action].pos), 
            .size = context.game.player.sprites[context.game.player.action].size,
            .rot = context.game.player.sprites[context.game.player.action].rot,
            .color = context.game.player.sprites[context.game.player.action].color,
            .zorder = 2,
            .mirror = context.game.player.mirror
        });

        renderer_frame_command_push(&context.renderer, (command_t) {
            .texture = context.game.enemy.sprites[context.game.enemy.action].texture, 
            .uv = {.scale = context.game.enemy.sprites[context.game.enemy.action].scale, .offset = context.game.enemy.sprites[context.game.enemy.action].offset}, 
            .pos = vec2_add(context.game.enemy.position, context.game.enemy.sprites[context.game.enemy.action].pos), 
            .size = context.game.enemy.sprites[context.game.enemy.action].size,
            .rot = context.game.enemy.sprites[context.game.enemy.action].rot,
            .color = context.game.enemy.sprites[context.game.enemy.action].color,
            .zorder = 2,
            .mirror = context.game.enemy.mirror
        });

        renderer_draw(&context.renderer, current_time);

        renderer_frame_clear(&context.renderer);

        // TEXT
        shader_use(context.ticker.framerate.text.shader);
        shader_set_int(context.ticker.framerate.text.shader, "u_Texture", 0);
        shader_set_vec3(context.ticker.framerate.text.shader, "u_Color", vec3(1.0f, 1.0f, 1.0f));

        char content[64];
        sprintf(content, "FPS: %0.f", context.ticker.framerate.value);
        text_draw(&context.ticker.framerate.text, content, 16.0f, (float) (WINDOW_HEIGHT - 16.0f), 2.0f);

        // OPENGL
        glfwSwapBuffers(context.platform.window);
        glfwPollEvents();

    }
}

void game_stop(void) {

    // clear vaos and vbos

    mem_arena_free(&context.arena); // why do i even add it?

    glfwDestroyWindow(context.platform.window);
    glfwTerminate();

}

// MAIN

int32_t main(void) {

    game_init();
    game_update();
    game_stop();

    return 0;
}