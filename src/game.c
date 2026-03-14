#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libmath/math.h>

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
#define WINDOW_NAME "BattleArena 2D (Build v0.0.2)"

#define ASSERT(_e, ...) if (!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1);}

// SHADER

typedef struct shader {
    uint32_t ids[2];
    uint32_t program;
} shader_t;

void _shader_read(char **content, char *filepath) {
    FILE *file = fopen(filepath, "r");
    ASSERT(file != NULL, "FILE_READ_ERROR: %s", filepath);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    *content = (char*) malloc(size + 1);
    fread(*content, 1, size, file);
    (*content)[size] = '\0';
    
    fclose(file);
}

void _shader_compile(uint32_t *id, uint32_t type, char *content) {
    *id = glCreateShader(type);

    glShaderSource(*id, 1, &content, NULL);
    glCompileShader(*id);

    free(content);

#ifdef DEBUG
    int32_t params;
    glGetShaderiv(*id, GL_COMPILE_STATUS, &params);
    if (params == 0) {
        char log[512];
        glGetShaderInfoLog(*id, 512, NULL, log);
        printf("SHADER_COMPILE_ERROR: %s\n", log);
        return;
    }
#endif
}

void shader_create(shader_t *shader, char *vertpath, char *fragpath) {
    char *vertcont, *fragcont;
    _shader_read(&vertcont, vertpath);
    _shader_read(&fragcont, fragpath);

    _shader_compile(&shader -> ids[0], GL_VERTEX_SHADER, vertcont);
    _shader_compile(&shader -> ids[1], GL_FRAGMENT_SHADER, fragcont);

    shader -> program = glCreateProgram();

    glAttachShader(shader -> program, shader -> ids[0]);
    glAttachShader(shader -> program, shader -> ids[1]);

    glLinkProgram(shader -> program);
}

void shader_use(shader_t *shader) {
    glUseProgram(shader -> program);
}

void shader_set_int(shader_t *shader, char *name, int val) {
    glUniform1i(glGetUniformLocation(shader -> program, name), val);
}

void shader_set_float(shader_t *shader, char *name, float val) {
    glUniform1f(glGetUniformLocation(shader -> program, name), val);
}

void shader_set_vec3(shader_t *shader, char *name, vec3_t vec) {
    glUniform3f(glGetUniformLocation(shader -> program, name), vec.x, vec.y, vec.z);
}

void shader_set_mat4(shader_t *shader, char *name, mat4_t mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader -> program, name), 1, GL_FALSE, &mat.m[0][0]);
}

// RENDERER

typedef struct texture {
    uint32_t id;
    uint32_t width, height;
    uint32_t format;
    uint32_t wraps, wrapt;
    uint32_t fmin, fmax;
} texture_t;

void texture_init(texture_t *texture, uint32_t width, uint32_t height, unsigned char *data) {
    glGenTextures(1, &texture -> id);

    texture -> width = width;
    texture -> height = height;

    glBindTexture(GL_TEXTURE_2D, texture -> id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void texture_bind(texture_t *texture) {
    glBindTexture(GL_TEXTURE_2D, 0);
}

typedef struct sprite {
    uint32_t texture;
    vec2_t position, size;
    float rotation;
    vec3_t color;
} sprite_t;

typedef struct renderer {
    shader_t shader;
    uint32_t vao, vbo;
} renderer_t;

void renderer_init(renderer_t *renderer) {
    vec4_t vertices[] = {
        vec4(0.0f, 1.0f, 0.0f, 1.0f),
        vec4(1.0f, 0.0f, 1.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 0.0f), 
        vec4(0.0f, 1.0f, 0.0f, 1.0f),
        vec4(1.0f, 1.0f, 1.0f, 1.0f),
        vec4(1.0f, 0.0f, 1.0f, 0.0f)
    };
    
    glGenVertexArrays(1, &renderer -> vao);
    glGenBuffers(1, &renderer -> vbo);

    glBindVertexArray(renderer -> vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer -> vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);  
    glBindVertexArray(0);
}

void renderer_draw(renderer_t *renderer, sprite_t *sprite) {
    shader_use(&renderer -> shader);

    mat4_t model = mat4(1.0f);
    model = mat4_trans(model, vec3(sprite -> position.x, sprite -> position.y, 0.0f));
    model = mat4_trans(model, vec3(sprite -> size.x * 0.5f, sprite -> size.y * 0.5f, 0.0f));
    model = mat4_rot(model, float_rad(sprite -> rotation), vec3(0.0f, 0.0f, 1.0f));
    model = mat4_trans(model, vec3(sprite -> size.x * (-0.5f), sprite -> size.y * (-0.5f), 0.0f));
    model = mat4_scale(model, vec3(sprite -> size.x, sprite -> size.y, 1.0f));

    shader_set_mat4(&renderer -> shader, "u_Model", model);
    shader_set_vec3(&renderer -> shader, "u_Color", sprite -> color);

    glActiveTexture(GL_TEXTURE0);
    // bind texture from sprit

    glBindVertexArray(renderer -> vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// GAME

static struct {
    GLFWwindow *window;

    renderer_t renderer;

    struct {
        sprite_t ground;
    } world;

    sprite_t player;
    sprite_t enemy;

} context;

// TODO
// game icon
// memory arena
// assets
// ..

void game_init(void) {
    // GLFW
    ASSERT(glfwInit(), "OPENGL_INIT_ERROR");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    context.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
    ASSERT(context.window, "GLFW_WINDOW_CREATE_ERROR");

    glfwMakeContextCurrent(context.window);
    // glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1;
    int32_t glew_err = glewInit();
    ASSERT(glew_err == 0 || glew_err == 4, "GLEW_INIT_ERROR");
#endif

    // OPENGL
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_PROGRAM_POINT_SIZE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // SHADER
    shader_create(&context.renderer.shader, "shader/sprite.vs", "shader/sprite.fs");

    // TEXTURE

    // RENDERER
    // renderer_init(&context.renderer);

    // SPRITE
    // context.player.texture = 0;
    // context.player.position = vec2(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    // context.player.size = vec2(128.0f, 256.0f);
    // context.player.rotation = 0.0f;
    // context.player.color = vec3(1.0f, 1.0f, 1.0f);

    // VIEW
    // mat4_t projection = mat4_ortho(0.0f, (float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    // shader_set_mat4(&context.renderer.shader, "u_View", projection);
    // shader_set_int(&context.renderer.shader, "u_Image", 0);

}

void game_update(void) {
    while (!glfwWindowShouldClose(context.window)) {

        // OPENGL
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // renderer_draw(&context.renderer, &context.player);
        //..

        // OPENGL
        glfwSwapBuffers(context.window);
        glfwPollEvents();

    }
}

void game_stop(void) {
    glfwDestroyWindow(context.window);
    glfwTerminate();
}

// MAIN

int main(void) {
    game_init();
    game_update();
    game_stop();
    return 0;
}        