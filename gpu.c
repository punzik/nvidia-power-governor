#define _GNU_SOURCE
#include "gpu.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

/* Read a single integer value from nvidia-smi csv output.
 * Returns 0 on success, -1 on error. */
static int query_gpu_int(const char *cmd, int *out)
{
    FILE *f = popen(cmd, "r");
    if (!f)
        return -1;

    char line[64];
    if (!fgets(line, sizeof(line), f)) {
        pclose(f);
        return -1;
    }

    int rc = pclose(f);
    if (rc != 0)
        return -1;

    char *end;
    long val = strtol(line, &end, 10);
    /* allow trailing whitespace/newline */
    while (*end == ' ' || *end == '\n' || *end == '\r')
        end++;
    if (*end != '\0')
        return -1;

    *out = (int)val;
    return 0;
}

/* Read a single float value from nvidia-smi csv output, round to int.
 * Returns 0 on success, -1 on error. */
static int query_gpu_float(const char *cmd, int *out)
{
    FILE *f = popen(cmd, "r");
    if (!f)
        return -1;

    char line[64];
    if (!fgets(line, sizeof(line), f)) {
        pclose(f);
        return -1;
    }

    int rc = pclose(f);
    if (rc != 0)
        return -1;

    char *end;
    double val = strtod(line, &end);
    /* allow trailing whitespace/newline */
    while (*end == ' ' || *end == '\n' || *end == '\r')
        end++;
    if (*end != '\0')
        return -1;

    *out = (int)(val + 0.5);
    return 0;
}

int gpu_count(void)
{
    FILE *f = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits", "r");
    if (!f)
        die("failed to run nvidia-smi");

    int count = 0;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        /* skip empty lines */
        if (line[0] != '\n' && line[0] != '\r' && line[0] != '\0')
            count++;
    }

    int rc = pclose(f);
    if (rc != 0)
        die("nvidia-smi failed");

    return count;
}

int gpu_read_temp(int id)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "nvidia-smi -i %d --query-gpu=temperature.gpu --format=csv,noheader,nounits", id);

    int temp;
    if (query_gpu_int(cmd, &temp) != 0)
        die("failed to read temperature for GPU %d", id);

    return temp;
}

int gpu_set_power(int id, int power_w, char *out, int out_size)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "nvidia-smi -i %d -pl %d 2>&1", id, power_w);

    FILE *f = popen(cmd, "r");
    if (!f) {
        die("failed to run nvidia-smi for GPU %d", id);
        return -1;
    }

    if (out && out_size > 0) {
        int i = 0;
        int ch;
        while ((ch = fgetc(f)) != EOF && i < out_size - 1) {
            out[i++] = (char)ch;
        }
        out[i] = '\0';
    } else {
        /* discard output */
        while (fgetc(f) != EOF)
            ;
    }

    int rc = pclose(f);
    if (rc != 0) {
        die("failed to set power limit for GPU %d", id);
        return -1;
    }

    return 0;
}

int gpu_default_power(int id)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd),
             "nvidia-smi -i %d --query-gpu=power.default_limit --format=csv,noheader,nounits", id);

    int power_w;
    if (query_gpu_float(cmd, &power_w) != 0)
        die("failed to read default power limit for GPU %d", id);

    return power_w;
}

struct gpu_state **gpu_state_init(const struct config *cfg)
{
    size_t state_size = sizeof(struct gpu_state) +
                        cfg->global.avg_samples * sizeof(int);

    struct gpu_state **states = calloc(cfg->gpu_count, sizeof(*states));
    if (!states)
        die("failed to allocate gpu_state array");

    for (int i = 0; i < cfg->gpu_count; i++) {
        states[i] = calloc(1, state_size);
        if (!states[i])
            die("failed to allocate gpu_state for GPU %d", i);

        states[i]->id = i;
        states[i]->current_power = cfg->gpus[i].max_power;
        states[i]->temp_index = 0;
        states[i]->temp_count = 0;
    }

    return states;
}

void gpu_state_free(struct gpu_state **states, int count)
{
    for (int i = 0; i < count; i++)
        free(states[i]);
    free(states);
}
