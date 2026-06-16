#ifndef GPU_H
#define GPU_H

#include "config.h"

struct gpu_state {
    int id;
    int current_power;     /* W */
    int temp_index;        /* next write position in buffer */
    int temp_count;        /* number of valid samples (until buffer is full) */
    int temp_buffer[];     /* flexible array, size = avg_samples */
};

/* Count GPUs on the system via nvidia-smi. Returns count or -1 on error. */
int gpu_count(void);

/* Read current temperature of a GPU. Returns °C or -1 on error. */
int gpu_read_temp(int id);

/* Set power limit for a GPU. Returns 0 on success, -1 on error. */
int gpu_set_power(int id, int power_w);

/* Read factory default power limit. Returns W or -1 on error. */
int gpu_default_power(int id);

/* Allocate and initialize gpu_state array. Returns pointer or NULL on error. */
struct gpu_state **gpu_state_init(const struct config *cfg);

/* Free gpu_state array. */
void gpu_state_free(struct gpu_state **states, int count);

#endif
