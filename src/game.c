#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// #include <libmath/math.h>

#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #define GLFW_INCLUDE_GLCOREARB
#else
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_NAME "BattleArena 2D (Build v0.0.1)"

#define ASSERT(_e, ...) if (!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1);}

// SHADER

typedef struct shader {
    uint32_t ids[2];
    uint32_t program;
} shader_t;

void _shader_read(char *content, char *filepath) {
    FILE *file = fopen(filepath, "r");
    ASSERT(file != NULL, "Failed to read shader file: %s", filepath);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    content = (char*) malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    
    fclose(file);
}

void _shader_compile(uint32_t *id, uint32_t type, char *content) {
    *id = glCreateShader(type);
    
    const char *copy = content;
    glShaderSource((uint32_t) &id, 1, &copy, NULL);
    glCompileShader((uint32_t) &id);

    free(content);
}

void shader_create(shader_t *shader, char *vertpath, char *fragpath) {
    char *vertcont, *fragcont;
    _shader_read(vertcont, vertpath);
    _shader_read(fragcont, fragpath);

    _shader_compile(&shader -> ids[0], GL_VERTEX_SHADER, vertcont);
    _shader_compile(&shader -> ids[1], GL_FRAGMENT_SHADER, fragcont);

    shader -> program = glCreateProgram();

    glAttachShader(shader -> program, shader -> ids[0]);
    glAttachShader(shader -> program, shader -> ids[1]);

    glLinkProgram(shader -> program);
}

// GAME

static struct {
    GLFWwindow *window;
} context;

// TODO
// game icon
// memory arena
// assets
// ..

void game_init(void) {
    // GLFW
    ASSERT(glfwInit(), "Failed to initialize OpenGL!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    context.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
    ASSERT(context.window, "Failed to create GLFW window!");

    glfwMakeContextCurrent(context.window);
    // glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1;
    int32_t glew_err = glewInit();
    ASSERT(glew_err == 0 || glew_err == 4, "Failed to initialize GLEW!");
#endif

    // OPENGL
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_PROGRAM_POINT_SIZE);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void game_update(void) {
    while (!glfwWindowShouldClose(context.window)) {

        // OPENGL
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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