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
#define WINDOW_NAME "BattleArena 2D (Build v0.0.13)"

#define ASSERT(_e, ...) if (!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1);}

// SHADER

typedef struct {
    uint32_t ids[2];
    uint32_t program;
} shader_t;

void _shader_read(char **code, char *path) {
    FILE *file = fopen(path, "rb");
    ASSERT(file != NULL, "FILE_READ_ERROR: %s", path);

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
    ASSERT(pixels, "TEXTURE_READ_ERROR: %s", path);

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

void renderer_draw(renderer_t *renderer, texture_t *texture, vec2_t position, vec2_t size, vec2_t scale, vec2_t offset, uint8_t mirror, float time) {
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
    shader_set_float(renderer->shader, "u_GlitchTime", time);
    shader_set_float(renderer->shader, "u_GlitchIntensity", 0.1f);

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
    uint8_t lock, impact;
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

typedef struct {
    vec2_t position;
    collision_box_t box;
    sprite_t sprite;
    // current animation
    uint8_t source, mirror;
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

#define GAME_SIMULATION_FIXED_TIMESTEP 1.0f / 60.0f // 60 fps
#define GAME_ANIMATION_FIXED_TIMESTEP 1.0f / 8.0f // 8 fps

#define GAME_MEMORY_CAPACITY (64 * 1024 * 1024) // 64 MB
uint8_t GAME_MEMORY[GAME_MEMORY_CAPACITY];

typedef enum {
    GAME_STATE_LOAD = 0,
    GAME_STATE_PAUSE = 1,
    GAME_STATE_PLAY = 2
} game_state_t;

static struct {

    struct {
        GLFWwindow *window;
        uint32_t width, height;
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
            double timer;
            uint32_t counter;
        } framerate;

    } ticker;

    mem_arena_t arena;

    struct {
        shader_t *shaders;
        texture_t *textures;
    } resources;

    renderer_t renderer;

    struct {
        game_state_t state;

        struct { // TODO add z-index to sprite_t
            sprite_t background;
            
            // temp
            sprite_t floor;
            // end of temp
            
            sprite_t *objects;

            // temp
            collision_box_t box;
            bullet_t bullets[16];
            uint32_t counter;
            // end of temp

        } level;

        actor_t player;
        actor_t enemy;
    } game;

} context;

void _game_win32_icon_init(void) {    
    char *filepath = "assets/icon.png";
    int32_t width, height, channels;
    unsigned char *pixels = stbi_load(filepath, &width, &height, &channels, 0);
    ASSERT(pixels, "ICON_READ_ERROR: %s", filepath);

    GLFWimage images[1] = {(GLFWimage) {.width = width, .height = height, .pixels = pixels}};
    glfwSetWindowIcon(context.platform.window, 1, images);

    stbi_image_free(pixels);
}

void _game_keyboard_callback(GLFWwindow *window, int32_t key, int32_t scan, int32_t action, int32_t mode) {
    if (key > -1 && key < 512) {
        if (action == GLFW_PRESS) {  
            context.platform.keys[key] = 1;
        } else if (action == GLFW_RELEASE) {
            context.platform.keys[key] = 0;
        }
    }
}

void _game_keyboard_handle_f(void) {
    if (context.game.state != GAME_STATE_PLAY) return;
    
    if (context.platform.keys[GLFW_KEY_ESCAPE]) {glfwSetWindowShouldClose(context.platform.window, 1);}

    if (!context.platform.keys[GLFW_KEY_W] && !context.platform.keys[GLFW_KEY_S] && !context.platform.keys[GLFW_KEY_A] && !context.platform.keys[GLFW_KEY_D]) {
        if (context.game.player.action != ACTOR_ACTION_IDLE) {
            context.game.player.action = ACTOR_ACTION_IDLE;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.game.player.animation.tick = 0;
        }
    }

    if (context.platform.keys[GLFW_KEY_W]) {
        if (context.game.player.box.lock == 0) {
            context.game.player.box.velocity.y = 12.0f;
            context.game.player.box.lock = 1;
            context.game.player.box.impact = 0;
        } else if (context.game.player.box.lock == 2 && !context.game.player.box.impact) {
            context.game.player.box.velocity.y = 16.0f;
            context.game.player.box.lock = 3;
        }
    }
    if (!context.platform.keys[GLFW_KEY_W]) {
        if (context.game.player.box.lock == 1) context.game.player.box.lock = 2;
    }

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

    // preposition test
    vec2_t preposition = vec2_add(context.game.player.position, context.game.player.box.velocity);
    vec2_t premin = vec2(preposition.x, preposition.y);
    vec2_t premax = vec2(preposition.x + 192, preposition.y + 192);

    // TODO i need to find a way to stop velocity before or right on collision box border

    // end of preposition test

    context.game.player.position = vec2_add(context.game.player.position, context.game.player.box.velocity);
    context.game.player.box.min = vec2(context.game.player.position.x, context.game.player.position.y);
    context.game.player.box.max = vec2(context.game.player.position.x + 144, context.game.player.position.y + 144);

    if (context.platform.keys[GLFW_KEY_S]) {
        context.game.player.box.max.y = context.game.player.position.y + 96;
        // ANIMATION
        context.game.player.action = ACTOR_ACTION_CROUCH;
        context.game.player.animation.step = ACTOR_ANIMATION_STEP_1;
        context.game.player.animation.tick = 2;
    }

    if (collision_box_intersection(&context.game.player.box, &context.game.level.box)) {
        context.game.player.box.velocity = vec2(0.0f, 0.0f);
        if (context.game.player.box.lock) context.game.player.box.lock = 0; 
        context.game.player.box.impact = 1;
        // printf("PLAYER_COLLISION\n");
    } else {
        context.game.player.box.velocity.y += -1;
        printf("PLAYER_VELOCITY={x=%f, y=%f}\n", context.game.player.box.velocity.x, context.game.player.box.velocity.y);
    }

    if (context.platform.keys[GLFW_KEY_F]) {
        if (context.game.level.counter < 4 && context.game.player.cooldown == 0) {
            bullet_t bullet = {
                .position = vec2(context.game.player.box.max.x + 0.0f, context.game.player.box.max.y - 48.0f), // get position of the gun
                // pos and box has to be affected by mirror
                .box = (collision_box_t) {.min = vec2(0.0f, 0.0f), .max = vec2(16.0f, 8.0f), .velocity = vec2(4.0f, -0.05f), .lock = 0, .impact = 0},
                .sprite = context.game.player.sprites[4],
                .source = 0,
                .mirror = context.game.player.mirror
            };
            context.game.level.bullets[context.game.level.counter++] = bullet;
            printf("BULLET_POSITION={x=%f, y=%f}\n", bullet.position.x, bullet.position.y);
            context.game.player.cooldown = 60;
        }
    }

    if (context.game.player.cooldown > 0) {
         context.game.player.cooldown--;
    }

    if (context.game.level.counter > 0) {
        for (uint32_t i = 0; i < context.game.level.counter; i++) {
            if (context.game.level.bullets[i].position.x < 0 || context.game.level.bullets[i].position.x > WINDOW_WIDTH) continue;
            context.game.level.bullets[i].position = vec2_add(context.game.level.bullets[i].position, context.game.level.bullets[i].box.velocity);
        }
    }
   
}

/*void _game_keyboard_handle(void) {
    // if (context.state = PAUSE) {// check for resume key; return;}
    if (context.keys[GLFW_KEY_ESCAPE]) {glfwSetWindowShouldClose(context.window, 1);}
    if (!context.keys[GLFW_KEY_W] && !context.keys[GLFW_KEY_S] && !context.keys[GLFW_KEY_A] && !context.keys[GLFW_KEY_D]) {
        if (context.game.player.action == ACTOR_ACTION_IDLE || context.game.player.animation.lock == 1) return;
        context.game.player.action = ACTOR_ACTION_IDLE;
        context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
        context.game.player.animation.tick = 0;
        context.game.player.animation.lock = 0;
    }
    if (context.keys[GLFW_KEY_W]) {
        // physics
        if (context.keys[GLFW_KEY_A] && !context.keys[GLFW_KEY_D]) {} // NE velocity
        if (context.keys[GLFW_KEY_D] && !context.keys[GLFW_KEY_A]) {} // NW velocity
        
        // collision

        // animation
        if (context.game.player.animation.lock != 1) {
            context.game.player.box.velocity.y += 32;
            context.game.player.box.impact = 0;
        }
        if (context.game.player.action != ACTOR_ACTION_JUMP) {
            context.game.player.action = ACTOR_ACTION_JUMP;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.game.player.animation.tick = 0;
            context.game.player.animation.lock = 3;
        }
    }
    if (context.keys[GLFW_KEY_S]) {
        // physics
        //..

        // printf("PRESS_KEY_S\n");

        // animation
        if (context.game.player.animation.lock == 1) {
            context.game.player.animation.tick = context.game.player.mirror ? 1 : 2;
            context.game.player.animation.lock = 2;
            // printf("UPDATE_LOCK_2\n");
        }
        if (context.game.player.action != ACTOR_ACTION_CROUCH) {
            context.game.player.action = ACTOR_ACTION_CROUCH;
            context.game.player.animation.step = ACTOR_ANIMATION_STEP_4;
            context.game.player.animation.tick = 0;
            context.game.player.animation.lock = 1;
            // printf("UPDATE_LOCK_1\n");
        }
    }
    if (context.keys[GLFW_KEY_A]) {
        if (context.game.player.animation.lock) return;
        if (context.game.player.position.x > 0) {
            context.game.player.position.x -= 4.0f;
            context.game.player.mirror = 1;
            // animation
            if (context.game.player.action != ACTOR_ACTION_RUN) {
                context.game.player.action = ACTOR_ACTION_RUN;
                context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
                context.game.player.animation.tick = 0;
            }
        }
    }
    if (context.keys[GLFW_KEY_D]) {
        if (context.game.player.animation.lock) return;
        if (context.game.player.position.x < (WINDOW_WIDTH - (context.game.player.sprites[context.game.player.action].size.x / 2))) {
            context.game.player.position.x += 4.0f;
            context.game.player.mirror = 0;
            // animation
            if (context.game.player.action != ACTOR_ACTION_RUN) {
                context.game.player.action = ACTOR_ACTION_RUN;
                context.game.player.animation.step = ACTOR_ANIMATION_STEP_6;
                context.game.player.animation.tick = 0;
            }
        }
    }
}*/

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

    context.platform.window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
    ASSERT(context.platform.window, "GLFW_WINDOW_CREATE_ERROR");

    glfwMakeContextCurrent(context.platform.window);
    glfwSetKeyCallback(context.platform.window, _game_keyboard_callback);
    // glfwSetInputMode(context.platform.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    // GLEW
#ifndef __APPLE__
    glewExperimental = 1;
    int32_t glew_error = glewInit();
    ASSERT(glew_error == 0 || glew_error == 4, "GLEW_INIT_ERROR"); // this needs to be rethinked
#endif

    // OPENGL
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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
    context.resources.shaders = mem_arena_alloc(&context.arena, 4 * sizeof(shader_t));
    shader_init(&context.resources.shaders[0], "shader/sprite.vs", "shader/sprite.fs");
    // crt shader?

    context.resources.textures = mem_arena_alloc(&context.arena, 8 * sizeof(texture_t));
    texture_init(&context.resources.textures[0], "assets/texture/level/floor.jpg");
    texture_init(&context.resources.textures[1], "assets/texture/player/body/idle.png");
    texture_init(&context.resources.textures[2], "assets/texture/player/body/jump.png");
    texture_init(&context.resources.textures[3], "assets/texture/player/body/run.png");
    texture_init(&context.resources.textures[4], "assets/texture/player/body/crouch.png");
    texture_init(&context.resources.textures[5], "assets/texture/projectile.png");
    //..

    // RENDERER
    renderer_init(&context.renderer, &context.resources.shaders[0]);

    mat4_t projection = mat4_ortho(0.0f, (float) WINDOW_WIDTH, (float) WINDOW_HEIGHT, 0.0f, -1.0f, 1.0f);

    shader_use(context.renderer.shader);
    shader_set_mat4(context.renderer.shader, "u_Projection", projection);
    shader_set_int(context.renderer.shader, "u_Texture", 0);

    // GAME
    context.game.state = GAME_STATE_PLAY;

    // level
    context.game.level.box = (collision_box_t) {.min = vec2(0.0f, 0.0f), .max = vec2(800.0f, 50.0f), .velocity = vec2(0.0f, 0.0f), .lock = 0, .impact = 1};

    sprite_init(&context.game.level.floor, &context.resources.textures[0], vec2(0.0f, 0.0f), vec2(800.0f, 50.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));

    // actor
    context.game.player.position = vec2(150.0f, 100.0f);
    context.game.player.box = (collision_box_t) {.min = vec2(150.0f, 100.0f), .max = vec2(342.0f, 292.0f), .velocity = vec2(0.0f, 0.0f), .lock = 0, .impact = 0};
    context.game.player.sprites = mem_arena_alloc(&context.arena, 8 * sizeof(sprite_t));
    context.game.player.action = ACTOR_ACTION_IDLE;
    context.game.player.animation = (actor_animation_t) {.step = ACTOR_ANIMATION_STEP_4, .tick = 0, .lock = 0};
    context.game.player.mirror = 0;
    context.game.player.fire = 0;

    sprite_init(&context.game.player.sprites[0], &context.resources.textures[1], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // idle
    sprite_init(&context.game.player.sprites[1], &context.resources.textures[2], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // jump
    sprite_init(&context.game.player.sprites[2], &context.resources.textures[3], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.167f, 1.0f), vec2(0.167f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // run
    sprite_init(&context.game.player.sprites[3], &context.resources.textures[4], vec2(0.0f, 0.0f), vec2(192.0f, 192.0f), 0.0f, vec2(0.25f, 1.0f), vec2(0.25f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // crouch

    // bullet
    sprite_init(&context.game.player.sprites[4], &context.resources.textures[5], vec2(0.0f, 0.0f), vec2(24.0f, 16.0f), 0.0f, vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f)); // projectile

    // TICKER
    context.ticker.time_of_last_frame = glfwGetTime();
}

void game_update(void) {
    while (!glfwWindowShouldClose(context.platform.window)) {

        // OPENGL 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TICKER
        double current_time = glfwGetTime();
        context.ticker.time_between_frames = current_time - context.ticker.time_of_last_frame;
        context.ticker.time_of_last_frame = current_time;

        // framerate
        context.ticker.framerate.timer += context.ticker.time_between_frames;
        context.ticker.framerate.counter++;

        if (context.ticker.framerate.timer >= 1.0f) { // TODO draw it in UI
            char title[64];
            sprintf(title, "%s [%.0f FPS]", WINDOW_NAME, (double) context.ticker.framerate.counter / context.ticker.framerate.timer);
            glfwSetWindowTitle(context.platform.window, title);
            context.ticker.framerate.timer -= 1.0;
            context.ticker.framerate.counter = 0;
        }

        // physics
        if (context.ticker.time_between_frames > 0.25) {
            context.ticker.time_between_frames = 0.25;
        }

        context.ticker.physics.accumulator += context.ticker.time_between_frames;

        while (context.ticker.physics.accumulator >= GAME_SIMULATION_FIXED_TIMESTEP) {
            
            // TODO save previous actors states

            // input
            _game_keyboard_handle_f();

            // animation
            context.ticker.animation.accumulator += GAME_SIMULATION_FIXED_TIMESTEP;

            while (context.ticker.animation.accumulator >= GAME_ANIMATION_FIXED_TIMESTEP) {
                context.ticker.animation.accumulator -= GAME_ANIMATION_FIXED_TIMESTEP;

                context.game.player.sprites[context.game.player.action].offset.x = (context.game.player.sprites[context.game.player.action].scale.x * context.game.player.animation.tick);
                if (context.game.player.animation.lock != 2 && context.game.player.animation.tick < context.game.player.animation.step) context.game.player.animation.tick++;
                else if (context.game.player.animation.lock != 2) context.game.player.animation.tick = 0;
            }

            // physics
            context.ticker.physics.accumulator -= GAME_SIMULATION_FIXED_TIMESTEP;
        }

        // double alpha = context.clock.accumulator / GAME_SIMULATION_FIXED_TIMESTEP;

        // RENDERER
        renderer_draw(&context.renderer, context.game.level.floor.texture, context.game.level.floor.position, context.game.level.floor.size, context.game.level.floor.scale, context.game.level.floor.offset, 0, current_time);
        // printf("pos={x=%f, y=%f}\n", context.game.level.floor.position.x, context.game.level.floor.position.y);

        renderer_draw(&context.renderer, context.game.player.sprites[context.game.player.action].texture, vec2_add(context.game.player.position, context.game.player.sprites[context.game.player.action].position), context.game.player.sprites[context.game.player.action].size, context.game.player.sprites[context.game.player.action].scale, context.game.player.sprites[context.game.player.action].offset, context.game.player.mirror, current_time);

        for (uint32_t i = 0; i < context.game.level.counter; i++) {
            renderer_draw(&context.renderer, context.game.level.bullets[i].sprite.texture, vec2_add(context.game.level.bullets[i].position, context.game.level.bullets[i].sprite.position), context.game.level.bullets[i].sprite.size, context.game.level.bullets[i].sprite.scale, context.game.level.bullets[i].sprite.offset, context.game.level.bullets[i].mirror, current_time);
        }

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