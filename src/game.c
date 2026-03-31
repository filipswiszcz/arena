#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
#define WINDOW_NAME "BattleArena 2D (Build v0.0.3)"

#define ASSERT(_e, ...) if (!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1);}

// SHADER

typedef struct {
    uint32_t ids[2];
    uint32_t program;
} shader_t;

void _shader_read(char **content, char *filepath) {
    FILE *file = fopen(filepath, "rb");
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

    glShaderSource(*id, 1, (const char**) &content, NULL);
    glCompileShader(*id);

    free(content);
 
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

void shader_set_vec2(shader_t *shader, char *name, vec2_t vec) {
    glUniform2f(glGetUniformLocation(shader -> program, name), vec.x, vec.y);
}

void shader_set_vec3(shader_t *shader, char *name, vec3_t vec) {
    glUniform3f(glGetUniformLocation(shader -> program, name), vec.x, vec.y, vec.z);
}

void shader_set_mat4(shader_t *shader, char *name, mat4_t mat) {
    glUniformMatrix4fv(glGetUniformLocation(shader -> program, name), 1, GL_FALSE, &mat.m[0][0]);
}

// TEXTURE

typedef struct {
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

    stbi_set_flip_vertically_on_load(1);
    
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

// SPRITE

typedef struct {
    texture_t *texture;
    vec2_t position, size;
    float rotation;
    vec2_t scale, offset; // it has to be rethinked
    vec3_t color;
} sprite_t;

void sprite_init(sprite_t *sprite, texture_t *texture, vec2_t position, vec2_t size, float rotation, vec2_t scale, vec2_t offset, vec3_t color) {
    sprite->texture = texture;
    sprite->position = position;
    sprite->size = size;
    sprite->rotation = rotation;
    sprite->scale = scale;
    sprite->offset = offset;
    sprite->color = color;
}

// RENDERER

typedef struct renderer {
    // a queue of things to render?
    shader_t *shader;
    uint32_t vao, vbo;
} renderer_t;

void renderer_init(renderer_t *renderer, shader_t *shader) {
    renderer->shader = shader;

    vec4_t vertices[] = {
        vec4(0.0f, 1.0f, 0.0f, 1.0f),
        vec4(1.0f, 0.0f, 1.0f, 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 0.0f), 
        vec4(0.0f, 1.0f, 0.0f, 1.0f),
        vec4(1.0f, 1.0f, 1.0f, 1.0f),
        vec4(1.0f, 0.0f, 1.0f, 0.0f)
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
}

void renderer_draw(renderer_t *renderer, sprite_t *sprite) {
    shader_use(renderer->shader);

    mat4_t model = mat4(1.0f);
    model = mat4_trans(model, vec3(sprite->position.x, sprite->position.y, 0.0f));
    model = mat4_trans(model, vec3(sprite->size.x * 0.5f, sprite->size.y * 0.5f, 0.0f));
    // model = mat4_rot(model, float_rad(sprite -> rotation), vec3(0.0f, 0.0f, 1.0f));
    model = mat4_rot(model, sprite->rotation, vec3(0.0f, 0.0f, 1.0f));
    model = mat4_trans(model, vec3(sprite->size.x * (-0.5f), sprite->size.y * (-0.5f), 0.0f));
    model = mat4_scale(model, vec3(sprite->size.x, sprite->size.y, 1.0f));

    shader_set_mat4(renderer->shader, "u_Model", model);
    shader_set_vec2(renderer->shader, "u_Scale", sprite->scale);
    shader_set_vec2(renderer->shader, "u_Offset", sprite->offset);
    shader_set_vec3(renderer->shader, "u_Color", sprite->color);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(sprite->texture);

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// GAME

typedef enum {
    CHARACTER_ACTION_IDLE = 0,
    CHARACTER_ACTION_JUMP = 1,
    CHARACTER_ACTION_RUN = 2,
    CHARACTER_ACTION_CROUCH = 3,
    CHARACTER_ACTION_WALK  = 4
} character_action_t;

typedef struct {
    vec2_t position, size;
    float rotation;
    vec2_t clip, offset;
} character_state_t;

typedef struct {
    // sprites as array? and action == index (with pos relative to character's pose?), pos
    sprite_t *sprites;
    // enum (or uint8_t) for current action (idle, jump, run, crouch, walk)
    character_action_t action;
    character_state_t current_state;
    vec2_t position;
    // when 1, then hand is going vert and gun also
    uint8_t fire; // it manages hand and gun 
    uint8_t mirror;
} character_t;

typedef struct {
    // owner?
    // sprite
    // current animation
    uint8_t mirror;
} bullet_t;

typedef struct {
    // sprites
    // array/queue with bullets (bullet updates pos until hits smth or flies outside of the arena)
    uint16_t turn;
} level_t;

// what i need:
// - a list with all textures (mem arena)
//      - do i run through assets/ and load them all?
//      - how do i mark them? with a name? eg "punk_weapon_rest", "punk_weapon_point"
//      - path "assets/texture/character/punk_idle.png"
// - a level struct
//      - animated background
//      - floor (rigid body) + void
//      - animated finish texts (like in mortal combat)
//      - list of all projectiles>
//          - level checks if character is hit by any bullet from enemy character?
// - a character struct
//      - animated body (with hands and weapon)
//      - collision and movement physics?
//      - all ML data

// renderer:
// - an array with static sprites, that are drawn each update
// - a queue with dynamic sprites, that are drawn and removed

static struct {
    GLFWwindow *window;

    int32_t keys[512];

    // struct {
    //     double time_of_last_frame;
    //     double time_between_frames;
    //     double time_accumulated;
    //     uint32_t count;
    // } fps;

    struct {
        double time_of_last_frame;
        double time_between_frames;
        double accumulator;
    } clock;

    struct {
        double time;
    } animation;

    mem_arena_t arena;

    // assets
    shader_t *shaders;
    texture_t *textures;

    renderer_t renderer;

    struct {
        sprite_t floor;
    } level;

    sprite_t temp;
    uint8_t ttemp;

    character_t player;
    character_t enemy;

} context; // rename to game

#define GAME_SIMULATION_FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_ANIMATION_FIXED_TIMESTEP 1.0f / 6.0f

#define GAME_MEMORY_CAPACITY (64 * 1024 * 1024) // 64 MB
uint8_t GAME_MEMORY[GAME_MEMORY_CAPACITY];

void _game_keyboard_callback(GLFWwindow *window, int32_t key, int32_t scan, int32_t action, int32_t mode) {
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    //     glfwSetWindowShouldClose(window, 1);
    // }
    if (key > -1 && key < 512) {
        if (action == GLFW_PRESS) {
            context.keys[key] = 1;
        } else if (action == GLFW_RELEASE) {
            context.keys[key] = 0;
        }
    }
}

void _game_keyboard_handle(void) {
    // check game state
    if (context.keys[GLFW_KEY_W]) {}
    if (context.keys[GLFW_KEY_S]) {}
    if (context.keys[GLFW_KEY_A]) {
        if (context.temp.position.x > 0) context.temp.position.x -= 4.0f;
    }
    if (context.keys[GLFW_KEY_D]) {
        if (context.temp.position.x < 300) context.temp.position.x += 4.0f;
    }
}

// void Game::ProcessInput(float dt)
// {
//     if (this->State == GAME_ACTIVE)
//     {
//         float velocity = PLAYER_VELOCITY * dt;
//         // move playerboard
//         if (this->Keys[GLFW_KEY_A])
//         {
//             if (Player->Position.x >= 0.0f)
//                 Player->Position.x -= velocity;
//         }
//         if (this->Keys[GLFW_KEY_D])
//         {
//             if (Player->Position.x <= this->Width - Player->Size.x)
//                 Player->Position.x += velocity;
//         }
//     }
// }

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
    glfwSetKeyCallback(context.window, _game_keyboard_callback);
    // glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1;
    int32_t glew_err = glewInit();
    ASSERT(glew_err == 0 || glew_err == 4, "GLEW_INIT_ERROR");
#endif

    // OPENGL
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // ICON
    //..

    // FRAMES
    context.clock.accumulator = 0.0;
    context.animation.time = 0.0;

    // MEMORY
    mem_arena_init(&context.arena, &GAME_MEMORY, GAME_MEMORY_CAPACITY);

    // SHADER
    context.shaders = mem_arena_alloc(&context.arena, 2 * sizeof(shader_t));
    shader_init(&context.shaders[0], "shader/sprite.vs", "shader/sprite.fs");
    // crt shader?

    // TEXTURE
    context.textures = mem_arena_alloc(&context.arena, 10 * sizeof(texture_t));
    texture_init(&context.textures[0], "assets/texture/level/floor.jpg");
    texture_init(&context.textures[1], "assets/texture/player/body/idle.png");
    texture_init(&context.textures[2], "assets/texture/player/body/jump.png");
    texture_init(&context.textures[3], "assets/texture/player/body/run.png");
    texture_init(&context.textures[4], "assets/texture/player/body/crouch.png");
    texture_init(&context.textures[5], "assets/texture/player/body/walk.png");
    texture_init(&context.textures[6], "assets/texture/player/body/hand/hand_idle.png");
    texture_init(&context.textures[7], "assets/texture/player/body/hand/hand_fire.png");
    texture_init(&context.textures[8], "assets/texture/player/weapon/gun_idle.png");
    texture_init(&context.textures[9], "assets/texture/player/weapon/gun_fire.png");
    //..

    // RENDERER
    renderer_init(&context.renderer, &context.shaders[0]);

    // LEVEL
    // sprite_init(&context.player.floor, &context.textures[0], vec2(0.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));
    // sprite_init(&context.enemy.floor, &context.textures[0], vec2(780.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));
    sprite_init(&context.level.floor, &context.textures[0], vec2(0.0f, 0.0f), vec2(300.0f, 50.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
    sprite_init(&context.temp, &context.textures[1], vec2(150.0f, 50.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
    context.ttemp = 0;

    // VIEW
    mat4_t projection = mat4_ortho(0.0f, (float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    shader_use(context.renderer.shader);
    shader_set_mat4(context.renderer.shader, "u_Projection", projection);
    shader_set_int(context.renderer.shader, "u_Texture", 0);

}

// void _game_fps_record(void) {
//     double curr_time = glfwGetTime();
//     context.fps.time_between_frames = (float) (curr_time - context.fps.time_of_last_frame);
//     context.fps.time_of_last_frame = curr_time;
//     context.fps.time_accumulated += context.fps.time_between_frames;
//     context.fps.count++;
//     if (context.fps.time_accumulated >= 1.0f) {
//         char title[64];
//         sprintf(title, "%s [%d FPS]", WINDOW_NAME, context.fps.count);
//         glfwSetWindowTitle(context.window, title);
//         context.fps.time_accumulated = 0.0f;
//         context.fps.count = 0;
//     }
// }

void game_update(void) {
    context.clock.time_of_last_frame = glfwGetTime();
    while (!glfwWindowShouldClose(context.window)) {

        // OPENGL 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // FPS
        // _game_fps_record();

        // SIMULATION
        double current_time = glfwGetTime();
        context.clock.time_between_frames = current_time - context.clock.time_of_last_frame;
        context.clock.time_of_last_frame = current_time;

        if (context.clock.time_between_frames > 0.25) {
            context.clock.time_between_frames = 0.25;
        }

        context.clock.accumulator += context.clock.time_between_frames;

        while (context.clock.accumulator >= GAME_SIMULATION_FIXED_TIMESTEP) {
            // save prev character states

            // update game logic
            _game_keyboard_handle();

            // ANIMATION
            context.animation.time += GAME_SIMULATION_FIXED_TIMESTEP;

            if (context.animation.time >= GAME_ANIMATION_FIXED_TIMESTEP) {
                context.animation.time -= GAME_ANIMATION_FIXED_TIMESTEP;

                context.temp.offset.x = (context.temp.scale.x * context.ttemp);
                if (context.ttemp < 3) context.ttemp++;
                else context.ttemp = 0;
            }

            // context.clock.time_simulation += GAME_FIXED_TIMESTEP;
            context.clock.accumulator -= GAME_SIMULATION_FIXED_TIMESTEP;
        }

        // double alpha = context.clock.accumulator / GAME_SIMULATION_FIXED_TIMESTEP;

        // RENDERER
        context.level.floor.position = vec2_add(context.level.floor.position, vec2(500.0f, 0.0f));
        renderer_draw(&context.renderer, &context.level.floor);
        context.level.floor.position = vec2_sub(context.level.floor.position, vec2(500.0f, 0.0f));
        renderer_draw(&context.renderer, &context.level.floor);
        // printf("pos={x=%f, y=%f}\n", context.level.floor.position.x, context.level.floor.position.y);

        renderer_draw(&context.renderer, &context.temp);

        // OPENGL
        glfwSwapBuffers(context.window);
        glfwPollEvents();

    }
}

void game_stop(void) {

    // clear vaos and vbos

    mem_arena_free(&context.arena); // why do i even add it?

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