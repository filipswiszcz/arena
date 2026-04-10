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
#define WINDOW_NAME "BattleArena 2D (Build v0.0.8)"

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
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(1);
    
    int32_t channels;
    unsigned char *pixels = stbi_load(filepath, &texture->width, &texture->height, &channels, 0);
    ASSERT(pixels, "TEXTURE_READ_ERROR: %s", filepath);

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
    glBindTexture(GL_TEXTURE_2D, texture -> id);
}

// SPRITE

typedef struct {
    texture_t *texture;
    vec2_t position, size;
    float rotation;
    vec2_t scale, offset;
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

void renderer_draw(renderer_t *renderer, texture_t *texture, vec2_t position, vec2_t size, vec2_t scale, vec2_t offset, uint8_t mirror) {
    shader_use(renderer->shader);

    mat4_t model = mat4(1.0f);
    model = mat4_trans(model, vec3(position.x, position.y, 0.0f));
    model = mat4_trans(model, vec3(size.x * 0.5f, size.y * 0.5f, 0.0f));
    // model = mat4_rot(model, float_rad(sprite -> rotation), vec3(0.0f, 0.0f, 1.0f));
    // model = mat4_rot(model, rotation, vec3(0.0f, 0.0f, 1.0f));
    model = mat4_trans(model, vec3(size.x * (-0.5f), size.y * (-0.5f), 0.0f));
    model = mat4_scale(model, vec3(size.x, size.y, 1.0f));

    shader_set_mat4(renderer->shader, "u_Model", model);
    shader_set_vec2(renderer->shader, "u_Scale", scale);
    shader_set_vec2(renderer->shader, "u_Offset", offset);
    shader_set_int(renderer->shader, "u_Mirror", mirror);
    shader_set_vec3(renderer->shader, "u_Color", vec3(1.0f, 1.0f, 1.0f));
    // shader_set_vec3(renderer->shader, "u_Color", sprite->color);

    glActiveTexture(GL_TEXTURE0);
    texture_bind(texture);

    glBindVertexArray(renderer->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// GAME

typedef struct {
    vec2_t min, max; // offsets to global pos
    vec2_t velocity;
    uint8_t ground;
} collision_box_t;

// until there is a collision, velocity should be constantly (-ffs, 0?);
    // ffs = free fall speed, 0? = it should always strive to reach a verticall fall

uint8_t collision_box_intersection(collision_box_t *a, collision_box_t *b) {
    return (((b->min.x <= a->max.x) && (b->max.x >= a->min.x)) && ((b->min.y <= a->max.y) && (b->max.y >= a->min.y)));
}

typedef enum {
    ACTOR_ACTION_IDLE = 0,
    ACTOR_ACTION_JUMP = 1,
    ACTOR_ACTION_RUN = 2,
    ACTOR_ACTION_CROUCH = 3
} actor_action_t; // change to bit flags

typedef struct {
    vec2_t position, size;
    float rotation;
    vec2_t clip, offset;
} actor_state_t;

typedef enum {
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
    vec2_t position;
    collision_box_t box;
    // sprites as array? and action == index (with pos relative to actor's pose?), pos
    sprite_t *sprites;
    // enum (or uint8_t) for current action (idle, jump, run, crouch)
    actor_action_t action;
    actor_animation_t animation;
    // uint8_t tick;
    // actor_state_t cstate, pstate;
    uint8_t mirror;
    uint8_t fire; // it manages hand and gun | when 1, then hand and gun are going vert (animation)
} actor_t;

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

    shader_t *shaders;
    texture_t *textures;

    renderer_t renderer;

    struct {
        sprite_t floor;
        collision_box_t box;
    } level;

    actor_t player;
    actor_t enemy;

} context; // rename to game

#define GAME_SIMULATION_FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_ANIMATION_FIXED_TIMESTEP 1.0f / 6.0f

#define GAME_MEMORY_CAPACITY (64 * 1024 * 1024) // 64 MB
uint8_t GAME_MEMORY[GAME_MEMORY_CAPACITY];

void _game_icon_init(void) {    
    char *filepath = "assets/icon.png";
    int32_t width, height, channels;
    unsigned char *pixels = stbi_load(filepath, &width, &height, &channels, 0);
    ASSERT(pixels, "ICON_READ_ERROR: %s", filepath);

    GLFWimage images[1] = {(GLFWimage) {.width = width, .height = height, .pixels = pixels}};
    glfwSetWindowIcon(context.window, 1, images);

    stbi_image_free(pixels);
}

void _game_keyboard_callback(GLFWwindow *window, int32_t key, int32_t scan, int32_t action, int32_t mode) {
    if (key > -1 && key < 512) {
        if (action == GLFW_PRESS) {  
            context.keys[key] = 1;
        } else if (action == GLFW_RELEASE) {
            context.keys[key] = 0;
        }
    }
}

void _game_keyboard_handle_f(void) {
    // if (context.state == GAME_PAUSE) return;
    if (context.keys[GLFW_KEY_ESCAPE]) {glfwSetWindowShouldClose(context.window, 1);}

    // TODO
    // i have to detect key as one-press
        // one action for one press
        // press once, lock, perform jump
        // add doublejump

    if (context.keys[GLFW_KEY_W]) {
        context.player.box.velocity.y = 4.0f;
        context.player.box.ground = 0;
    }

    if (context.keys[GLFW_KEY_S]) {}

    if (context.keys[GLFW_KEY_A]) {
        if ((context.player.position.x - 4.0f) >= 0) context.player.box.velocity.x = -4.0f;
    }

    if (context.keys[GLFW_KEY_D]) {
        if ((context.player.position.x + 4.0f) <= (WINDOW_WIDTH - (context.player.sprites[context.player.action].size.x / 2))) context.player.box.velocity.x = 4.0f;
    }

    // if (!context.player.box.ground) context.player.box.velocity.y += -1;
    context.player.position = vec2_add(context.player.position, context.player.box.velocity);
    context.player.box.min = vec2(context.player.position.x, context.player.position.y);
    context.player.box.max = vec2(context.player.position.x + 192, context.player.position.y + 192);

    // if (!context.player.box.ground) {
    if (collision_box_intersection(&context.player.box, &context.level.box)) {
        context.player.box.velocity = vec2(0.0f, 0.0f);
        context.player.box.ground = 1;
        // printf("PLAYER_COLLISION\n");
    } else {
        context.player.box.velocity.y += -1;
        printf("PLAYER_VELOCITY={x=%f, y=%f}\n", context.player.box.velocity.x, context.player.box.velocity.y);
        // context.player.box.velocity = vec2(0.0f, -1.0f);
    }
    // }

    // context.player.position = vec2_add(context.player.position, context.player.box.velocity);
    // context.player.box.min = vec2(context.player.position.x, context.player.position.y);
    // context.player.box.max = vec2(context.player.position.x + 192, context.player.position.y + 192);

}

void _game_keyboard_handle(void) {
    // if (context.state = PAUSE) {// check for resume key; return;}
    if (context.keys[GLFW_KEY_ESCAPE]) {glfwSetWindowShouldClose(context.window, 1);}
    if (!context.keys[GLFW_KEY_W] && !context.keys[GLFW_KEY_S] && !context.keys[GLFW_KEY_A] && !context.keys[GLFW_KEY_D]) {
        if (context.player.action == ACTOR_ACTION_IDLE || context.player.animation.lock == 1) return;
        context.player.action = ACTOR_ACTION_IDLE;
        context.player.animation.step = ACTOR_ANIMATION_STEP_4;
        context.player.animation.tick = 0;
        context.player.animation.lock = 0;
    }
    if (context.keys[GLFW_KEY_W]) {
        // physics
        if (context.keys[GLFW_KEY_A] && !context.keys[GLFW_KEY_D]) {} // NE velocity
        if (context.keys[GLFW_KEY_D] && !context.keys[GLFW_KEY_A]) {} // NW velocity
        
        // collision

        // animation
        if (context.player.animation.lock != 1) {
            context.player.box.velocity.y += 32;
            context.player.box.ground = 0;
        }
        if (context.player.action != ACTOR_ACTION_JUMP) {
            context.player.action = ACTOR_ACTION_JUMP;
            context.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.player.animation.tick = 0;
            context.player.animation.lock = 3;
        }
    }
    if (context.keys[GLFW_KEY_S]) {
        // physics
        //..

        // printf("PRESS_KEY_S\n");

        // animation
        if (context.player.animation.lock == 1) {
            context.player.animation.tick = context.player.mirror ? 1 : 2;
            context.player.animation.lock = 2;
            // printf("UPDATE_LOCK_2\n");
        }
        if (context.player.action != ACTOR_ACTION_CROUCH) {
            context.player.action = ACTOR_ACTION_CROUCH;
            context.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.player.animation.tick = 0;
            context.player.animation.lock = 1;
            // printf("UPDATE_LOCK_1\n");
        }
    }
    if (context.keys[GLFW_KEY_A]) { 
        if (context.player.animation.lock) return;
        if (context.player.position.x > 0) {
            context.player.position.x -= 4.0f;
            context.player.mirror = 1;
            // animation
            if (context.player.action != ACTOR_ACTION_RUN) {
                context.player.action = ACTOR_ACTION_RUN;
                context.player.animation.step = ACTOR_ANIMATION_STEP_6;
                context.player.animation.tick = 0;
            }
        }
    }
    if (context.keys[GLFW_KEY_D]) {
        if (context.player.animation.lock) return;
        if (context.player.position.x < (WINDOW_WIDTH - (context.player.sprites[context.player.action].size.x / 2))) {
            context.player.position.x += 4.0f;
            context.player.mirror = 0;
            // animation
            if (context.player.action != ACTOR_ACTION_RUN) {
                context.player.action = ACTOR_ACTION_RUN;
                context.player.animation.step = ACTOR_ANIMATION_STEP_6;
                context.player.animation.tick = 0;
            }
        }
    }
}

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
#ifdef _WIN32
    _game_icon_init();
#else
    //..
#endif

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
    context.textures = mem_arena_alloc(&context.arena, 9 * sizeof(texture_t));
    texture_init(&context.textures[0], "assets/texture/level/floor.jpg");
    texture_init(&context.textures[1], "assets/texture/player/body/idle.png");
    texture_init(&context.textures[2], "assets/texture/player/body/jump.png");
    texture_init(&context.textures[3], "assets/texture/player/body/run.png");
    texture_init(&context.textures[4], "assets/texture/player/body/crouch.png");
    texture_init(&context.textures[5], "assets/texture/player/body/hand/hand_idle.png");
    texture_init(&context.textures[6], "assets/texture/player/body/hand/hand_fire.png");
    texture_init(&context.textures[7], "assets/texture/player/weapon/gun_idle.png");
    texture_init(&context.textures[8], "assets/texture/player/weapon/gun_fire.png");
    //..

    // RENDERER
    renderer_init(&context.renderer, &context.shaders[0]);

    // actor
    context.player.position = vec2(150.0f, 100.0f);
    context.player.box = (collision_box_t) {.min = vec2(150.0f, 100.0f), .max = vec2(342.0f, 292.0f), .velocity = vec2(0.0f, 0.0f), .ground = 0};
    context.player.sprites = mem_arena_alloc(&context.arena, 8 * sizeof(sprite_t));
    context.player.action = ACTOR_ACTION_IDLE;
    context.player.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.player.mirror = 0;
    context.player.fire = 0;

    sprite_init(&context.player.sprites[0], &context.textures[1], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // idle
    sprite_init(&context.player.sprites[1], &context.textures[2], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // jump
    sprite_init(&context.player.sprites[2], &context.textures[3], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // run
    sprite_init(&context.player.sprites[3], &context.textures[4], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // crouch
    sprite_init(&context.player.sprites[4], &context.textures[5], vec2(10.0f, 18.0f), vec2(128.0f, 128.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // hand idle
    sprite_init(&context.player.sprites[5], &context.textures[6], vec2(10.0f, 18.0f), vec2(128.0f, 128.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // hand fire
    sprite_init(&context.player.sprites[6], &context.textures[7], vec2(0.0f, 0.0f), vec2(72.0f, 32.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // weapon idle
    sprite_init(&context.player.sprites[7], &context.textures[8], vec2(128.0f, 128.0f), vec2(72.0f, 32.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // weapon fire

    // LEVEL
    // sprite_init(&context.player.floor, &context.textures[0], vec2(0.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));
    // sprite_init(&context.enemy.floor, &context.textures[0], vec2(780.0f, 0.0f), vec2(500.0f, 100.0f), 0.0f, vec3(1.0f, 1.0f, 1.0f));
    context.level.box = (collision_box_t) {.min = vec2(0.0f, 0.0f), .max = vec2(800.0f, 50.0f), .velocity = vec2(0.0f, 0.0f), .ground = 1};
    sprite_init(&context.level.floor, &context.textures[0], vec2(0.0f, 0.0f), vec2(800.0f, 50.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
    // sprite_init(&context.temp, &context.textures[1], vec2(150.0f, 50.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f));

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
            // save prev actor states

            // LOGIC
            _game_keyboard_handle_f();

            // if (!context.player.box.ground) {
            //     // printf("PLAYER_NOT_GROUND\n");
            //     if (collision_box_intersection(&context.player.box, &context.level.box)) {
            //         context.player.box.velocity = vec2(0.0f, 0.0f);
            //         context.player.box.ground = 1;
            //         // printf("PLAYER_COLLISION\n");
            //     } else {
            //         context.player.box.velocity = vec2(0.0f, -1.0f);
            //     }
            //     // context.player.position = vec2_add(context.player.position, context.player.box.velocity);
            // }

            // context.player.position = vec2_add(context.player.position, context.player.box.velocity);
            // context.player.box.min = vec2(context.player.position.x, context.player.position.y);
            // context.player.box.max = vec2(context.player.position.x + 192, context.player.position.y + 192);

            // ANIMATION
            context.animation.time += GAME_SIMULATION_FIXED_TIMESTEP;

            if (context.animation.time >= GAME_ANIMATION_FIXED_TIMESTEP) {
                context.animation.time -= GAME_ANIMATION_FIXED_TIMESTEP;

                context.player.sprites[context.player.action].offset.x = (context.player.sprites[context.player.action].scale.x * context.player.animation.tick);
                if (context.player.animation.lock != 2 && context.player.animation.tick < context.player.animation.step) context.player.animation.tick++;
                else if (context.player.animation.lock != 2) context.player.animation.tick = 0;
                // else context.player.animation.tick = 0;
                // else if (context.player.animation.tick == context.player.animation.step && context.player.animation.lock == 2) context.player.animation.lock = 0;
                // if (context.player.animation.lock) context.player.animation.tick = 0;
                // else if ()
                // else context.player.animation.tick = 0;

                 // it can only work properly if the whole animation is played out
                // if (context.player.animation.tick == 2) context.player.sprites[5].position.x -= 2;
                // else if (context.player.animation.tick == 4) context.player.sprites[5].position.x += 2;
            }

            context.clock.accumulator -= GAME_SIMULATION_FIXED_TIMESTEP;
        }

        // double alpha = context.clock.accumulator / GAME_SIMULATION_FIXED_TIMESTEP;

        // RENDERER
        // context.level.floor.position = vec2_add(context.level.floor.position, vec2(0.0f, 0.0f));
        renderer_draw(&context.renderer, context.level.floor.texture, context.level.floor.position, context.level.floor.size, context.level.floor.scale, context.level.floor.offset, 0);
        // context.level.floor.position = vec2_sub(context.level.floor.position, vec2(500.0f, 0.0f));
        // renderer_draw(&context.renderer, context.level.floor.texture, context.level.floor.position, context.level.floor.size, context.level.floor.scale, context.level.floor.offset, 0);
        // printf("pos={x=%f, y=%f}\n", context.level.floor.position.x, context.level.floor.position.y);

        // renderer_draw(&context.renderer, context.player.sprites[4].texture, vec2_add(context.player.position, context.player.sprites[4].position), context.player.sprites[4].size, context.player.sprites[4].scale, context.player.sprites[4].offset);
        renderer_draw(&context.renderer, context.player.sprites[context.player.action].texture, vec2_add(context.player.position, context.player.sprites[context.player.action].position), context.player.sprites[context.player.action].size, context.player.sprites[context.player.action].scale, context.player.sprites[context.player.action].offset, context.player.mirror);
        // renderer_draw(&context.renderer, context.player.sprites[5].texture, vec2_add(context.player.position, context.player.sprites[5].position), context.player.sprites[5].size, context.player.sprites[5].scale, context.player.sprites[5].offset);

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

int32_t main(void) {
    game_init();
    game_update();
    game_stop();
    return 0;
}