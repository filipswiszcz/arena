#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libmath/math.h>
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

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 960
#define WINDOW_NAME "BattleArena 2D (Build v0.0.3)"

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

void shader_init(shader_t *shader, char *vertpath, char *fragpath) {
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
    int32_t width, height;
    int32_t format;
} texture_t;

void texture_init(texture_t *texture, char *filepath) {
    glGenTextures(1, &texture -> id);
    glBindTexture(GL_TEXTURE_2D, texture -> id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int32_t channels;
    unsigned char *pixels = stbi_load(filepath, &texture -> width, &texture -> height, &channels, 0);
    ASSERT(pixels, "TEXTURE_READ_ERROR: %s", filepath);

    switch (channels) {
        case 1: {texture -> format = GL_RED; break;}
        case 3: {texture -> format = GL_RGB; break;}
        case 4: {texture -> format = GL_RGBA; break;}
    }

    glTexImage2D(GL_TEXTURE_2D, 0, texture -> format, texture -> width, texture -> height, 0, texture -> format, GL_UNSIGNED_BYTE, pixels);
    // ?

    stbi_image_free(pixels);
}

void texture_bind(texture_t *texture) {
    glBindTexture(GL_TEXTURE_2D, texture -> id);
}

typedef struct sprite {
    texture_t *texture;
    vec2_t position, size;
    float rotation;
    vec3_t color;
} sprite_t;

void sprite_init(sprite_t *sprite, texture_t *texture, vec2_t position, vec2_t size, float rotation, vec3_t color) {
    sprite -> texture = texture;
    sprite -> position = position;
    sprite -> size = size;
    sprite -> rotation = rotation;
    sprite -> color = color;
}

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

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec4_t), (void*) (2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);  
    glBindVertexArray(0);
}

void renderer_draw(renderer_t *renderer, sprite_t *sprite) {
    shader_use(&renderer -> shader);

    mat4_t model = mat4(1.0f);
    model = mat4_trans(model, vec3(sprite -> position.x, sprite -> position.y, 0.0f));
    model = mat4_trans(model, vec3(sprite -> size.x * 0.5f, sprite -> size.y * 0.5f, 0.0f));
    // model = mat4_rot(model, float_rad(sprite -> rotation), vec3(0.0f, 0.0f, 1.0f));
    model = mat4_rot(model, sprite -> rotation, vec3(0.0f, 0.0f, 1.0f));
    model = mat4_trans(model, vec3(sprite -> size.x * (-0.5f), sprite -> size.y * (-0.5f), 0.0f));
    model = mat4_scale(model, vec3(sprite -> size.x, sprite -> size.y, 1.0f));

    shader_set_mat4(&renderer -> shader, "u_Model", model);
    shader_set_vec3(&renderer -> shader, "u_Color", sprite -> color);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(sprite -> texture);

    glBindVertexArray(renderer -> vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// GAME

static struct {
    GLFWwindow *window;

    // texture_t *textures;
    texture_t textures[3]; // temp

    renderer_t renderer;

    struct {
        sprite_t floor;
    } player;

    struct {
        sprite_t floor;
    } enemy;

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
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

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
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // SHADER
    shader_init(&context.renderer.shader, "shader/sprite.vs", "shader/sprite.fs");

    // TEXTURE
    // arena alloc
    texture_init(&context.textures[0], "assets/texture/floor.jpg");
    //..

    // RENDERER
    renderer_init(&context.renderer);

    // SPRITE
    sprite_init(&context.player.floor, &context.textures[0], vec2(0.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));
    sprite_init(&context.enemy.floor, &context.textures[0], vec2(780.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));

    // VIEW
    mat4_t projection = mat4_ortho(0.0f, (float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    shader_use(&context.renderer.shader);
    shader_set_mat4(&context.renderer.shader, "u_Projection", projection);
    shader_set_int(&context.renderer.shader, "u_Texture", 0);

}

void game_update(void) {
    while (!glfwWindowShouldClose(context.window)) {

        // OPENGL
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer_draw(&context.renderer, &context.player.floor);
        renderer_draw(&context.renderer, &context.enemy.floor);
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