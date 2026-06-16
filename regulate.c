#include "regulate.h"

int regulate_compute(int avg_temp,
                     const struct gpu_config *gpu_cfg,
                     const struct global_config *global_cfg,
                     int current_power)
{
    if (avg_temp >= gpu_cfg->max_temp) {
        /* Overheating: decrease power */
        int new_power = current_power - global_cfg->power_step;
        if (new_power < gpu_cfg->min_power)
            new_power = gpu_cfg->min_power;
        return new_power;
    }

    if (avg_temp <= gpu_cfg->max_temp - global_cfg->hysteresis) {
        /* Cool enough: increase power */
        int new_power = current_power + global_cfg->power_step;
        if (new_power > gpu_cfg->max_power)
            new_power = gpu_cfg->max_power;
        return new_power;
    }

    /* Within hysteresis band: no change */
    return current_power;
}
