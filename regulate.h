#ifndef REGULATE_H
#define REGULATE_H

#include "config.h"

/* Compute new power limit based on average temperature.
 *
 * If avg_temp >= max_temp: decrease power by power_step (not below min_power).
 * If avg_temp <= max_temp - hysteresis: increase power by power_step (not above max_power).
 * Otherwise: return current_power unchanged.
 *
 * Returns the new power in watts.
 */
int regulate_compute(int avg_temp,
                     const struct gpu_config *gpu_cfg,
                     const struct global_config *global_cfg,
                     int current_power);

#endif
