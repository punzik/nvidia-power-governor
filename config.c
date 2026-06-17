#include "config.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_LINE_BUF 256

/* Trim leading and trailing whitespace in-place. Returns pointer to first non-space. */
static char *trim(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    return s;
}

/* Parse a single integer value. Returns 0 on success, -1 on error. */
static int parse_int(const char *str, int *out)
{
    char *end;
    if (*str == '\0')
        return -1; /* empty string */
    long val = strtol(str, &end, 10);
    if (end == str || *end != '\0' || val < INT_MIN || val > INT_MAX)
        return -1;
    *out = (int)val;
    return 0;
}

enum section {
    SECTION_NONE,
    SECTION_GLOBAL,
    SECTION_GPU,
};

struct parse_ctx {
    struct config *cfg;
    enum section section;
    int gpu_idx;
    /* bitmasks: which fields have been set */
    int global_flags;
    int gpu_flags[MAX_GPUS];
};

#define G_POLL_INTERVAL      (1 << 0)
#define G_AVG_SAMPLES        (1 << 1)
#define G_DRAW_AVG_SAMPLES   (1 << 2)
#define G_ALL                (G_POLL_INTERVAL | G_AVG_SAMPLES | G_DRAW_AVG_SAMPLES)

#define GPU_TEMP_THRESHOLD_HIGH    (1 << 0)
#define GPU_TEMP_THRESHOLD_LOW     (1 << 1)
#define GPU_MAX_POWER              (1 << 2)
#define GPU_MIN_POWER              (1 << 3)
#define GPU_POWER_STEP_DOWN_TEMP   (1 << 4)
#define GPU_POWER_STEP_DOWN_DRAW   (1 << 5)
#define GPU_POWER_STEP_UP_DRAW     (1 << 6)
#define GPU_POWER_DRAW_OFFSET_DOWN (1 << 7)
#define GPU_POWER_DRAW_OFFSET_UP   (1 << 8)
#define GPU_ALL  (GPU_TEMP_THRESHOLD_HIGH | GPU_TEMP_THRESHOLD_LOW | GPU_MAX_POWER | \
                  GPU_MIN_POWER | GPU_POWER_STEP_DOWN_TEMP | GPU_POWER_STEP_DOWN_DRAW | \
                  GPU_POWER_STEP_UP_DRAW | GPU_POWER_DRAW_OFFSET_DOWN | GPU_POWER_DRAW_OFFSET_UP)

static void ctx_init(struct parse_ctx *ctx, struct config *cfg)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->cfg = cfg;
}

static int parse_global(struct parse_ctx *ctx, const char *key, const char *val)
{
    int v;
    struct global_config *g = &ctx->cfg->global;

    if (strcmp(key, "poll_interval") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        g->poll_interval = v;
        ctx->global_flags |= G_POLL_INTERVAL;
    } else if (strcmp(key, "avg_samples") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        g->avg_samples = v;
        ctx->global_flags |= G_AVG_SAMPLES;
    } else if (strcmp(key, "draw_avg_samples") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        g->draw_avg_samples = v;
        ctx->global_flags |= G_DRAW_AVG_SAMPLES;
    } else {
        return -1; /* unknown key */
    }
    return 0;
}

static int parse_gpu(struct parse_ctx *ctx, const char *key, const char *val)
{
    int v;
    int idx = ctx->gpu_idx;
    struct gpu_config *gpu = &ctx->cfg->gpus[idx];

    if (strcmp(key, "temp_threshold_high") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->temp_threshold_high = v;
        ctx->gpu_flags[idx] |= GPU_TEMP_THRESHOLD_HIGH;
    } else if (strcmp(key, "temp_threshold_low") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->temp_threshold_low = v;
        ctx->gpu_flags[idx] |= GPU_TEMP_THRESHOLD_LOW;
    } else if (strcmp(key, "max_power") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->max_power = v;
        ctx->gpu_flags[idx] |= GPU_MAX_POWER;
    } else if (strcmp(key, "min_power") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->min_power = v;
        ctx->gpu_flags[idx] |= GPU_MIN_POWER;
    } else if (strcmp(key, "power_step_down_temp") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->power_step_down_temp = v;
        ctx->gpu_flags[idx] |= GPU_POWER_STEP_DOWN_TEMP;
    } else if (strcmp(key, "power_step_down_draw") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->power_step_down_draw = v;
        ctx->gpu_flags[idx] |= GPU_POWER_STEP_DOWN_DRAW;
    } else if (strcmp(key, "power_step_up_draw") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->power_step_up_draw = v;
        ctx->gpu_flags[idx] |= GPU_POWER_STEP_UP_DRAW;
    } else if (strcmp(key, "power_draw_offset_down") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->power_draw_offset_down = v;
        ctx->gpu_flags[idx] |= GPU_POWER_DRAW_OFFSET_DOWN;
    } else if (strcmp(key, "power_draw_offset_up") == 0) {
        if (parse_int(val, &v) != 0 || v <= 0)
            return -1;
        gpu->power_draw_offset_up = v;
        ctx->gpu_flags[idx] |= GPU_POWER_DRAW_OFFSET_UP;
    } else {
        return -1; /* unknown key */
    }
    return 0;
}

struct config *config_load(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "config: cannot open '%s'\n", path);
        return NULL;
    }

    struct config *cfg = calloc(1, sizeof(*cfg));
    if (!cfg) {
        fprintf(stderr, "config: out of memory\n");
        fclose(f);
        return NULL;
    }

    struct parse_ctx ctx;
    ctx_init(&ctx, cfg);

    char line[CONFIG_LINE_BUF];
    int line_num = 0;

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        char *s = trim(line);

        /* skip empty lines and comments */
        if (*s == '\0' || *s == '#' || *s == ';')
            continue;

        /* section header */
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (!end) {
                fprintf(stderr, "config: '%s': line %d: missing ']'\n", path, line_num);
                fclose(f);
                config_free(cfg);
                return NULL;
            }
            *end = '\0';
            char *name = trim(s + 1);

            if (strcmp(name, "global") == 0) {
                ctx.section = SECTION_GLOBAL;
            } else if (strncmp(name, "gpu.", 4) == 0) {
                char *num_start = name + 4;
                char *endp;
                if (*num_start == '\0') {
                    fprintf(stderr, "config: '%s': line %d: invalid section name '[%s]'\n", path, line_num, s + 1);
                    fclose(f);
                    config_free(cfg);
                    return NULL;
                }
                long idx = strtol(num_start, &endp, 10);
                if (endp == num_start || *endp != '\0') {
                    fprintf(stderr, "config: '%s': line %d: invalid section name '[%s]'\n", path, line_num, s + 1);
                    fclose(f);
                    config_free(cfg);
                    return NULL;
                }
                if (idx < 0) {
                    fprintf(stderr, "config: '%s': line %d: invalid section name '[%s]'\n", path, line_num, s + 1);
                    fclose(f);
                    config_free(cfg);
                    return NULL;
                }
                if (idx >= MAX_GPUS) {
                    fprintf(stderr, "config: '%s': line %d: GPU index %ld exceeds MAX_GPUS (%d)\n",
                            path, line_num, idx, MAX_GPUS);
                    fclose(f);
                    config_free(cfg);
                    return NULL;
                }
                ctx.section = SECTION_GPU;
                ctx.gpu_idx = (int)idx;
                if ((int)idx >= cfg->gpu_count)
                    cfg->gpu_count = (int)idx + 1;
            } else {
                fprintf(stderr, "config: '%s': line %d: unknown section '[%s]'\n", path, line_num, name);
                fclose(f);
                config_free(cfg);
                return NULL;
            }
            continue;
        }

        /* key=value */
        char *eq = strchr(s, '=');
        if (!eq) {
            fprintf(stderr, "config: '%s': line %d: expected 'key=value', got '%s'\n", path, line_num, s);
            fclose(f);
            config_free(cfg);
            return NULL;
        }
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);

        if (ctx.section == SECTION_NONE) {
            fprintf(stderr, "config: '%s': line %d: key '%s' outside of any section\n", path, line_num, key);
            fclose(f);
            config_free(cfg);
            return NULL;
        }

        int ret;
        if (ctx.section == SECTION_GLOBAL) {
            ret = parse_global(&ctx, key, val);
        } else {
            ret = parse_gpu(&ctx, key, val);
        }
        if (ret != 0) {
            fprintf(stderr, "config: '%s': line %d: invalid value for '%s'\n", path, line_num, key);
            fclose(f);
            config_free(cfg);
            return NULL;
        }
    }

    fclose(f);

    /* validate: [global] must be present and complete */
    if ((ctx.global_flags & G_ALL) != G_ALL) {
        fprintf(stderr, "config: '%s': missing required fields in [global]\n", path);
        config_free(cfg);
        return NULL;
    }

    /* validate: each GPU section must be complete */
    for (int i = 0; i < cfg->gpu_count; i++) {
        if ((ctx.gpu_flags[i] & GPU_ALL) != GPU_ALL) {
            fprintf(stderr, "config: '%s': missing or incomplete [gpu.%d] section\n", path, i);
            config_free(cfg);
            return NULL;
        }
    }

    /* must have at least one GPU */
    if (cfg->gpu_count <= 0) {
        fprintf(stderr, "config: '%s': no GPU sections defined\n", path);
        config_free(cfg);
        return NULL;
    }

    /* semantic validation of global config */
    if (cfg->global.poll_interval > MAX_POLL_INTERVAL_MS) {
        fprintf(stderr, "config: '%s': poll_interval must be <= %d\n", path, MAX_POLL_INTERVAL_MS);
        config_free(cfg);
        return NULL;
    }
    if (cfg->global.avg_samples > MAX_AVG_SAMPLES) {
        fprintf(stderr, "config: '%s': avg_samples must be <= %d\n", path, MAX_AVG_SAMPLES);
        config_free(cfg);
        return NULL;
    }
    if (cfg->global.draw_avg_samples > MAX_AVG_SAMPLES) {
        fprintf(stderr, "config: '%s': draw_avg_samples must be <= %d\n", path, MAX_AVG_SAMPLES);
        config_free(cfg);
        return NULL;
    }

    /* semantic validation of each GPU config */
    for (int i = 0; i < cfg->gpu_count; i++) {
        struct gpu_config *gpu = &cfg->gpus[i];
        if (gpu->temp_threshold_low >= gpu->temp_threshold_high) {
            fprintf(stderr, "config: '%s': [gpu.%d] temp_threshold_low (%d) >= temp_threshold_high (%d)\n",
                    path, i, gpu->temp_threshold_low, gpu->temp_threshold_high);
            config_free(cfg);
            return NULL;
        }
        if (gpu->min_power > gpu->max_power) {
            fprintf(stderr, "config: '%s': [gpu.%d] min_power (%d) > max_power (%d)\n",
                    path, i, gpu->min_power, gpu->max_power);
            config_free(cfg);
            return NULL;
        }
        if (gpu->power_draw_offset_up >= gpu->power_draw_offset_down) {
            fprintf(stderr, "config: '%s': [gpu.%d] power_draw_offset_up (%d) >= power_draw_offset_down (%d)\n",
                    path, i, gpu->power_draw_offset_up, gpu->power_draw_offset_down);
            config_free(cfg);
            return NULL;
        }
    }

    return cfg;
}

void config_free(struct config *cfg)
{
    free(cfg);
}
