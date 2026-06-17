#ifndef GPU_H
#define GPU_H

#include "config.h"

struct gpu_state {
    int power_limit;              /* W */
    int temp_index;               /* next write position in temp buffer */
    int temp_count;               /* number of valid temp samples */
    int temp_buffer[MAX_AVG_SAMPLES];
    int draw_index;               /* next write position in draw buffer */
    int draw_count;               /* number of valid draw samples */
    int draw_buffer[MAX_AVG_SAMPLES];
};

/* Count GPUs on the system via nvidia-smi. Returns count or -1 on error. */
int gpu_count(void);

/* Read current temperature of a GPU. Returns °C or -1 on error. */
int gpu_read_temp(int id);

/* Set power limit for a GPU. Returns 0 on success, -1 on error.
 * If out is not NULL, captures nvidia-smi output (up to out_size-1 bytes). */
int gpu_set_power(int id, int power_w, char *out, int out_size);

/* Read factory default power limit. Returns W or -1 on error. */
int gpu_default_power(int id);

/* Read current power draw of a GPU. Returns W (rounded) or -1 on error. */
int gpu_read_power_draw(int id);

/* Allocate and initialize gpu_state array. Returns pointer or NULL on error. */
struct gpu_state **gpu_state_init(const struct config *cfg);

/* Free gpu_state array. */
void gpu_state_free(struct gpu_state **states, int count);

#endif
