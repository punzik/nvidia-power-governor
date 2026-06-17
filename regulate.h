#ifndef REGULATE_H
#define REGULATE_H

#include "config.h"
#include "gpu.h"

/* Compute average temperature from a ring buffer of samples.
 * Returns the average in °C, or 0 if no samples have been collected. */
int compute_avg_temp(struct gpu_state *state);

/* Compute average power draw from a ring buffer of samples.
 * Returns the average in W, or 0 if no samples have been collected. */
int compute_avg_draw(struct gpu_state *state);

/* Compute new power limit based on average temperature and current power draw.
 *
 * Zone 1 (avg_temp >= temp_threshold_high): decrease by power_step_down_temp.
 * Zone 2 (avg_temp <= temp_threshold_low): draw-based regulation using offsets.
 * Zone 3 (between thresholds): no change.
 *
 * Temperature always has priority over draw-based regulation.
 * Returns the new power limit in watts. */
int regulate_compute(int avg_temp,
                     int power_draw_w,
                     const struct gpu_config *gpu_cfg,
                     int power_limit);

#endif
