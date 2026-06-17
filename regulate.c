#include "regulate.h"

int compute_avg_temp(struct gpu_state *state)
{
    int count = state->temp_count;
    if (count == 0)
        return 0;

    long sum = 0;
    for (int i = 0; i < count; i++)
        sum += state->temp_buffer[i];

    return (int)(sum / count);
}

int compute_avg_draw(struct gpu_state *state)
{
    int count = state->draw_count;
    if (count == 0)
        return 0;

    long sum = 0;
    for (int i = 0; i < count; i++)
        sum += state->draw_buffer[i];

    return (int)(sum / count);
}

int regulate_compute(int avg_temp,
                     int power_draw_w,
                     const struct gpu_config *gpu_cfg,
                     int power_limit)
{
    /* Zone 1: overheat — temperature priority, always decrease */
    if (avg_temp >= gpu_cfg->temp_threshold_high) {
        int new_power = power_limit - gpu_cfg->power_step_down_temp;
        if (new_power < gpu_cfg->min_power)
            new_power = gpu_cfg->min_power;
        return new_power;
    }

    /* Zone 3: middle band — no change */
    if (avg_temp > gpu_cfg->temp_threshold_low)
        return power_limit;

    /* Zone 2: cool enough — draw-based regulation */
    int threshold_down = power_limit - gpu_cfg->power_draw_offset_down;
    int threshold_up   = power_limit - gpu_cfg->power_draw_offset_up;

    if (power_draw_w <= threshold_down) {
        /* Low draw — tighten limit */
        int new_power = power_limit - gpu_cfg->power_step_down_draw;
        if (new_power < gpu_cfg->min_power)
            new_power = gpu_cfg->min_power;
        return new_power;
    }

    if (power_draw_w >= threshold_up) {
        /* High draw — allow more power */
        int new_power = power_limit + gpu_cfg->power_step_up_draw;
        if (new_power > gpu_cfg->max_power)
            new_power = gpu_cfg->max_power;
        return new_power;
    }

    /* Draw hysteresis band — no change */
    return power_limit;
}
