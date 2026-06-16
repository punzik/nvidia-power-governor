#ifndef REGULATE_H
#define REGULATE_H

#include "config.h"
#include "gpu.h"

/* Compute new power limit based on average temperature.
 *
 * If avg_temp >= max_temp: decrease power by power_step (not below min_power).
 * If avg_temp <= max_temp - hysteresis: increase power by power_step (not above max_power).
 * Otherwise: return current_power unchanged.
 *
 * Returns the new power in watts.
 */
/* Compute average temperature from a ring buffer of samples.
 * Returns the average in °C, or 0 if no samples have been collected. */
int compute_avg_temp(struct gpu_state *state);

/* Compute new power limit based on average temperature.
 *
 * If avg_temp >= max_temp: decrease power by power_step (not below min_power).
 * If avg_temp <= max_temp - hysteresis: increase power by power_step (not above max_power).
 * Otherwise: return current_power unchanged.
 *
 * Returns the new power in watts. */
int regulate_compute(int avg_temp,
                     const struct gpu_config *gpu_cfg,
                     int hysteresis,
                     int current_power);

#endif
