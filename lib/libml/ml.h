#ifndef __LIBML__
#define __LIBML__

#if defined(__cplusplus)
    #include <cstdint>
#else
    #include <stdint.h>
#endif
#include <stdlib.h>

#include <assert.h>
#include <math.h>
#include <time.h>

#define ML_EPISODE_STEPS 10000

static inline float ml_random(void) {
    static uint8_t init = 0;
    if (!init) {srand((unsigned) time(NULL)); init = 1;}
    return (float) rand() / ((float) RAND_MAX + 1.0f);
}

static inline float ml_brandom(const float bound) {
    static uint8_t init = 0;
    if (!init) {srand((unsigned) time(NULL)); init = 1;}
    return ((float) rand() / (float) RAND_MAX) * 2.0f * bound - bound;
}

// MAT

#define mat(d, r, c) (ml_mat(d, r, c))

typedef struct {
    float *data;
    uint32_t rows, cols;
} mat_t;

static inline mat_t ml_mat(float *data, uint32_t rows, uint32_t cols) {
    return (mat_t) {.data = data, .rows = rows, .cols = cols};
}

static inline void ml_mat_rand(mat_t *mat, float min, float max) {
    for (uint32_t i = 0; i < (mat->rows * mat->cols); i++) {
        mat->data[i] = min + ((float) ml_random() * (max - min)); 
    }
}

static inline void ml_mat_dot(mat_t *mat, mat_t *a, mat_t *b) {
    assert(mat->rows == a->rows && mat->cols == b->cols);
    assert(a->cols == b->rows);

    for (uint32_t i = 0; i < a->rows; i++) {
        for (uint32_t j = 0; j < b->cols; j++) {
            float temp = 0.0f;
            for (uint32_t k = 0; k < a->cols; k++) {
                temp += (a->data[i * a->cols + k] * b->data[k * b->cols + j]);
            }
            mat->data[i * mat->cols + j] = temp;
        }
    }
}

static inline void ml_mat_add(mat_t *mat, mat_t *a, mat_t *b) {
    assert(mat->rows == a->rows && mat->cols == a->cols);
    assert(a->rows == b->rows && a->cols == b->cols);

    for (uint32_t i = 0; i < (a->rows * a->cols); i++) {
        mat->data[i] = a->data[i] + b->data[i];
    }
}

static inline void ml_mat_zero(mat_t *mat) {
    for (uint32_t i = 0; i < (mat->rows * mat->cols); i++) {
        mat->data[i] = 0.0f;
    }
}

static inline void ml_mat_transp(mat_t *mat, mat_t *a) {
    assert(mat->rows == a->cols && mat->cols == a->rows);
    for (uint32_t i = 0; i < a->rows; i++) {
        for (uint32_t j = 0; j < a->cols; j++) {
            mat->data[j * mat->cols + i] = a->data[i * a->cols + j];
        }
    }
}

// NEURAL NET

typedef struct {
    mat_t states[ML_EPISODE_STEPS];
    int32_t actions[ML_EPISODE_STEPS];
    float rewards[ML_EPISODE_STEPS];
    uint32_t steps;
} ml_trajectory_t;

static inline void ml_trajectory_init(ml_trajectory_t *traj, float *data, uint32_t size) {
    for (uint32_t i = 0; i < ML_EPISODE_STEPS; i++) {
        float *datapart = &data[i * size];
        traj->states[i] = mat(datapart, 1, size);
    }
    traj->steps = 0;
}

static inline void ml_trajectory_step_add(ml_trajectory_t *traj, mat_t *state, int32_t action, float reward) {
    if (traj->steps >= ML_EPISODE_STEPS) return;

    for (uint32_t i = 0; i < (state->rows * state->cols); i++) {
        traj->states[traj->steps].data[i] = state->data[i];
    }

    traj->actions[traj->steps] = action;
    traj->rewards[traj->steps] = reward;
    traj->steps++;
}

typedef struct {
    mat_t weights, biases;
    mat_t ins, outs; // memory (prev ins and outs) (backpropagation)
} ml_layer_t;

typedef struct {
    ml_layer_t hidd, out;
    float rate; // learning rate
} ml_network_t;

static inline void ml_network_init(ml_network_t *network, float **data, uint32_t hiddsize, uint32_t insize, uint32_t outsize) {
    network->hidd.weights = mat(data[0], insize, hiddsize);
    network->hidd.biases = mat(data[1], 1, hiddsize);
    network->out.weights = mat(data[2], hiddsize, outsize);
    network->out.biases = mat(data[3], 1, outsize);
    network->hidd.outs = mat(data[4], 1, hiddsize);

    for (uint32_t i = 0; i < (hiddsize * insize); i++) {
        network->hidd.weights.data[i] = ml_brandom(1.0f / sqrtf((float) insize));
    }

    for (uint32_t i = 0; i < hiddsize; i++) {
        network->hidd.biases.data[i] = 0.0f;
    }

    for (uint32_t i = 0; i < (hiddsize * outsize); i++) {
        network->out.weights.data[i] = ml_brandom(1.0f / sqrtf((float) hiddsize));
    }

    for (uint32_t i = 0; i < outsize; i++) {
        network->out.biases.data[i] = 0.0f;
    }

    network->rate = 0.001f;
}

static inline void ml_network_forward_move(ml_network_t *network, const mat_t *state, mat_t *probs) {

    ml_mat_dot(&network->hidd.outs, (mat_t *) state, &network->hidd.weights);
    ml_mat_add(&network->hidd.outs, &network->hidd.outs, &network->hidd.biases);

    for (uint32_t i = 0; i < (network->hidd.outs.rows * network->hidd.outs.cols); i++) {
        if (network->hidd.outs.data[i] < 0.0f) network->hidd.outs.data[i] = 0.0f;
    }

    ml_mat_dot(probs, &network->hidd.outs, &network->out.weights);
    ml_mat_add(probs, probs, &network->out.biases);

    float maxv = probs->data[0];
    for (uint32_t i = 1; i < probs->cols; i++) {
        if (probs->data[i] > maxv) maxv = probs->data[i];
    }

    float expv = 0.0f;
    for (uint32_t i = 0; i < probs->cols; i++) {
        probs->data[i] = expf(probs->data[i] - maxv);
        expv += probs->data[i];
    }

    for (uint32_t i = 0; i < probs->cols; i++) {
        probs->data[i] /= expv;
    }

}

static inline void ml_network_episode_train(ml_network_t *network, float *data, ml_trajectory_t *traj) {
    if (traj->steps == 0) return;

    float rewards[ML_EPISODE_STEPS];
    float gamma = 0.99f, runner = 0.0f;

    for (int32_t i = traj->steps - 1; i >= 0; i--) {
        runner = runner * gamma + traj->rewards[i]; 
        rewards[i] = runner;
    }

    float mean = 0.0f, stddev = 0.0f;
    for (uint32_t i = 0; i < traj->steps; i++) mean += rewards[i];
    mean /= (float) traj->steps;
    for (uint32_t i = 0; i < traj->steps; i++) stddev += powf(rewards[i] - mean, 2.0f);
    stddev = sqrtf(stddev / (float) traj->steps) + 1e-8f;
    for (uint32_t i = 0; i < traj->steps; i++) rewards[i] = (rewards[i] - mean) / stddev;

    float *head = data;

    float *ghwdata = head; head += (network->hidd.weights.rows * network->hidd.weights.cols);
    float *ghbdata = head; head += (network->hidd.biases.rows * network->hidd.biases.cols);
    float *gowdata = head; head += (network->out.weights.rows * network->out.weights.cols);
    float *gobdata = head; head += (network->out.biases.rows * network->out.biases.cols);

    mat_t gradhw = mat(ghwdata, network->hidd.weights.rows, network->hidd.weights.cols);
    mat_t gradhb = mat(ghbdata, network->hidd.biases.rows, network->hidd.biases.cols);
    mat_t gradow = mat(gowdata, network->out.weights.rows, network->out.weights.cols);
    mat_t gradob = mat(gobdata, network->out.biases.rows, network->out.biases.cols);

    ml_mat_zero(&gradhw);
    ml_mat_zero(&gradhb);
    ml_mat_zero(&gradow);
    ml_mat_zero(&gradob);

    float *prodata = head; head += network->out.weights.cols;
    mat_t probs = mat(prodata, 1, network->out.weights.cols);
    float *socdata = head; head += probs.cols;
    mat_t socd = mat(socdata, 1, probs.cols);
    float *hidtdata = head; head += network->hidd.outs.cols;
    mat_t hidt = mat(hidtdata, network->hidd.outs.cols, 1);
    float *stgdata = head; head += (gradow.rows * gradow.cols);
    mat_t stg = mat(stgdata, gradow.rows, gradow.cols);
    float *outtdata = head; head += (network->out.weights.rows * network->out.weights.cols);
    mat_t outt = mat(outtdata, network->out.weights.cols, network->out.weights.rows);
    float *dtgdata = head; head += network->hidd.outs.cols;
    mat_t dtg = mat(dtgdata, 1, network->hidd.outs.cols);
    float *reldata = head; head += network->hidd.outs.cols;
    mat_t reld = mat(reldata, 1, network->hidd.outs.cols);
    float *sttdata = head; head += traj->states[0].cols;
    mat_t stt = mat(sttdata, traj->states[0].cols, 1);
    float *sggdata = head; head += (gradhw.rows * gradhw.cols);
    mat_t sgg = mat(sggdata, gradhw.rows, gradhw.cols);

    for (uint32_t i = 0; i < traj->steps; i++) {
        ml_network_forward_move(network, &traj->states[i], &probs);

        for (uint32_t j = 0; j < probs.cols; j++) {
            socd.data[j] = (probs.data[j] - (j == (uint32_t) traj->actions[i] ? 1.0f : 0.0f)) * rewards[i];
        }

        ml_mat_transp(&hidt, &network->hidd.outs);
        ml_mat_dot(&stg, &hidt, &socd);

        ml_mat_add(&gradow, &gradow, &stg);
        ml_mat_add(&gradob, &gradob, &socd);

        ml_mat_transp(&outt, &network->out.weights);
        ml_mat_dot(&dtg, &socd, &outt);

        for (uint32_t j = 0; j < network->hidd.outs.cols; j++) {
            reld.data[j] = (network->hidd.outs.data[j] > 0.0f) ? dtg.data[j] : 0.0f;
        }

        ml_mat_transp(&stt, &traj->states[i]);
        ml_mat_dot(&sgg, &stt, &reld);

        ml_mat_add(&gradhw, &gradhw, &sgg);
        ml_mat_add(&gradhb, &gradhb, &reld);
    }

    for (uint32_t j = 0; j < (network->hidd.weights.rows * network->hidd.weights.cols); j++) {
        network->hidd.weights.data[j] -= network->rate * gradhw.data[j];
    }
    for (uint32_t j = 0; j < (network->hidd.biases.rows * network->hidd.biases.cols); j++) {
        network->hidd.biases.data[j] -= network->rate * gradhb.data[j];
    }
    for (uint32_t j = 0; j < (network->out.weights.rows * network->out.weights.cols); j++) {
        network->out.weights.data[j] -= network->rate * gradow.data[j];
    }
    for (uint32_t j = 0; j < (network->out.biases.rows * network->out.biases.cols); j++) {
        network->out.biases.data[j] -= network->rate * gradob.data[j];
    }
}

// VLAs problem in win32 (msvc compiler)
// static inline void ml_network_episode_train(ml_network_t *network, ml_trajectory_t *traj) {
//     if (traj->steps == 0) return;

//     float rewards[ML_EPISODE_STEPS];
//     float gamma = 0.99f, runner = 0.0f;

//     for (int32_t i = traj->steps - 1; i >= 0; i--) {
//         runner = runner * gamma + traj->rewards[i]; 
//         rewards[i] = runner;
//     }

//     float mean = 0.0f, stddev = 0.0f;
//     for (uint32_t i = 0; i < traj->steps; i++) mean += rewards[i];
//     mean /= (float) traj->steps;
//     for (uint32_t i = 0; i < traj->steps; i++) stddev += powf(rewards[i] - mean, 2.0f);
//     stddev = sqrtf(stddev / (float) traj->steps) + 1e-8f;
//     for (uint32_t i = 0; i < traj->steps; i++) rewards[i] = (rewards[i] - mean) / stddev;

//     float ghwdata[network->hidd.weights.rows * network->hidd.weights.cols];
//     float ghbdata[network->hidd.biases.rows * network->hidd.biases.cols];
//     float gowdata[network->out.weights.rows * network->out.weights.cols];
//     float gobdata[network->out.biases.rows * network->out.biases.cols];

//     mat_t gradhw = mat(ghwdata, network->hidd.weights.rows, network->hidd.weights.cols);
//     mat_t gradhb = mat(ghbdata, network->hidd.biases.rows, network->hidd.biases.cols);
//     mat_t gradow = mat(gowdata, network->out.weights.rows, network->out.weights.cols);
//     mat_t gradob = mat(gobdata, network->out.biases.rows, network->out.biases.cols);

//     ml_mat_zero(&gradhw);
//     ml_mat_zero(&gradhb);
//     ml_mat_zero(&gradow);
//     ml_mat_zero(&gradob);

//     float prodata[network->out.weights.cols];
//     mat_t probs = mat(prodata, 1, network->out.weights.cols);

//     for (uint32_t i = 0; i < traj->steps; i++) {
//         ml_network_forward_move(network, &traj->states[i], &probs);

//         float socdata[probs.cols];
//         mat_t socd = mat(socdata, 1, probs.cols);
//         for (uint32_t j = 0; j < probs.cols; j++) {
//             socd.data[j] = (probs.data[j] - (j == (uint32_t) traj->actions[i] ? 1.0f : 0.0f)) * rewards[i];
//         }

//         float hidtdata[network->hidd.outs.cols];
//         mat_t hidt = mat(hidtdata, network->hidd.outs.cols, 1);
//         ml_mat_transp(&hidt, &network->hidd.outs);

//         float stgdata[gradow.rows * gradow.cols];
//         mat_t stg = mat(stgdata, gradow.rows, gradow.cols);
//         ml_mat_dot(&stg, &hidt, &socd);

//         ml_mat_add(&gradow, &gradow, &stg);
//         ml_mat_add(&gradob, &gradob, &socd);

//         float outtdata[network->out.weights.rows * network->out.weights.cols];
//         mat_t outt = mat(outtdata, network->out.weights.cols, network->out.weights.rows);
//         ml_mat_transp(&outt, &network->out.weights);

//         float dtgdata[network->hidd.outs.cols];
//         mat_t dtg = mat(dtgdata, 1, network->hidd.outs.cols);
//         ml_mat_dot(&dtg, &socd, &outt);

//         float reldata[network->hidd.outs.cols];
//         mat_t reld = mat(reldata, 1, network->hidd.outs.cols);
//         for (uint32_t j = 0; j < network->hidd.outs.cols; j++) {
//             reld.data[j] = (network->hidd.outs.data[j] > 0.0f) ? dtg.data[j] : 0.0f;
//         }

//         float sttdata[traj->states[i].cols];
//         mat_t stt = mat(sttdata, traj->states[i].cols, 1);
//         ml_mat_transp(&stt, &traj->states[i]);

//         float sggdata[gradhw.rows * gradhw.cols];
//         mat_t sgg = mat(sggdata, gradhw.rows, gradhw.cols);
//         ml_mat_dot(&sgg, &stt, &reld);

//         ml_mat_add(&gradhw, &gradhw, &sgg);
//         ml_mat_add(&gradhb, &gradhb, &reld);
//     }

//     for (uint32_t j = 0; j < (network->hidd.weights.rows * network->hidd.weights.cols); j++) {
//         network->hidd.weights.data[j] -= network->rate * gradhw.data[j];
//     }
//     for (uint32_t j = 0; j < (network->hidd.biases.rows * network->hidd.biases.cols); j++) {
//         network->hidd.biases.data[j] -= network->rate * gradhb.data[j];
//     }
//     for (uint32_t j = 0; j < (network->out.weights.rows * network->out.weights.cols); j++) {
//         network->out.weights.data[j] -= network->rate * gradow.data[j];
//     }
//     for (uint32_t j = 0; j < (network->out.biases.rows * network->out.biases.cols); j++) {
//         network->out.biases.data[j] -= network->rate * gradob.data[j];
//     }

// }

static inline int32_t ml_sample_action(mat_t *probs) {
    float r = ml_random(), accum = 0.0f;
    for (uint32_t i = 0; i < probs->cols; i++) {
        accum += probs->data[i];
        if (r <= accum) return i;
    }
    return probs->cols - 1;
}

#endif // !__LIBML__