#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

struct gpu_config {
    int id;
    int max_temp;   /* °C */
    int max_power;  /* W */
    int min_power;  /* W */
    int power_step_up;   /* W — increase step */
    int power_step_down; /* W — decrease step */
};

struct global_config {
    int poll_interval;  /* ms */
    int avg_samples;
    int hysteresis;     /* °C */
};

#define MAX_GPUS 8

struct config {
    struct global_config global;
    struct gpu_config gpus[MAX_GPUS];
    int gpu_count;
};

/* Parse config file. Returns NULL on error. */
struct config *config_load(const char *path);

/* Free config memory. */
void config_free(struct config *cfg);

#endif
