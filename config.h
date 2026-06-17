#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

struct gpu_config {
    int id;
    int temp_threshold_high;    /* °C — upper temperature threshold */
    int temp_threshold_low;     /* °C — lower temperature threshold */
    int max_power;              /* W */
    int min_power;              /* W */
    int power_step_down_temp;   /* W — decrease step when overheating */
    int power_step_down_draw;   /* W — decrease step when draw is low */
    int power_step_up_draw;     /* W — increase step when draw is high */
    int power_draw_offset_down; /* W — offset below limit for decrease threshold */
    int power_draw_offset_up;   /* W — offset below limit for increase threshold */
};

struct global_config {
    int poll_interval;  /* ms */
    int avg_samples;
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
