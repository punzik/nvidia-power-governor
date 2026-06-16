#define _GNU_SOURCE
#include "config.h"
#include "gpu.h"
#include "regulate.h"

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAIN_OUTPUT_BUF 256
#define MS_PER_SEC      1000

static void emit_timestamp(void)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    fprintf(stdout, "[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Options:\n"
            "  -c, --config FILE    Configuration file (required)\n"
            "  -h, --help           Show this help and exit\n"
            "  -v, --verbose        Enable verbose output\n"
            "  --restore-max        Set all GPUs to max power from config and exit\n"
            "  --restore-min        Set all GPUs to min power from config and exit\n"
            "  --restore-factory    Set all GPUs to factory default power and exit\n",
            prog);
}

static int verbose;

static void emit_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    emit_timestamp();
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
}

static void emit_verbose(const char *fmt, ...)
{
    if (!verbose)
        return;
    va_list ap;
    va_start(ap, fmt);
    emit_timestamp();
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
}

/* Print each line of a multi-line buffer with two-space indentation. */
static void emit_indented(const char *buf)
{
    const char *p = buf;
    int first = 1;
    while (*p) {
        const char *nl = strchr(p, '\n');
        if (nl) {
            if (first) {
                emit_timestamp();
                first = 0;
            }
            fwrite("  ", 1, 2, stdout);
            fwrite(p, 1, (size_t)(nl - p), stdout);
            fprintf(stdout, "\n");
            p = nl + 1;
        } else {
            /* last line without newline */
            if (*p) {
                if (first) {
                    emit_timestamp();
                    first = 0;
                }
                fwrite("  ", 1, 2, stdout);
                fwrite(p, 1, strlen(p), stdout);
                fprintf(stdout, "\n");
            }
            break;
        }
    }
}


static int compute_avg_temp(struct gpu_state *state)
{
    int count = state->temp_count;
    if (count == 0)
        return 0;

    long sum = 0;
    for (int i = 0; i < count; i++)
        sum += state->temp_buffer[i];

    return (int)(sum / count);
}

static void do_restore(const struct config *cfg, int mode)
{
    for (int i = 0; i < cfg->gpu_count; i++) {
        int power;
        switch (mode) {
        case 'M':
            power = cfg->gpus[i].max_power;
            break;
        case 'm':
            power = cfg->gpus[i].min_power;
            break;
        case 'F':
            power = gpu_default_power(i);
            break;
        default:
            return;
        }
        char buf[MAIN_OUTPUT_BUF] = {0};
        emit_log("GPU %d: setting power to %d W", i, power);
        gpu_set_power(i, power, verbose ? buf : NULL, sizeof(buf));
        if (verbose && buf[0])
            emit_indented(buf);
    }
}

int main(int argc, char *argv[])
{
    const char *config_path = NULL;
    int restore_mode = 0;  /* 0=run loop, 'M'=max, 'm'=min, 'F'=factory */

    static struct option long_options[] = {
        {"config",        required_argument, 0, 'c'},
        {"help",          no_argument,       0, 'h'},
        {"verbose",       no_argument,       0, 'v'},
        {"restore-max",   no_argument,       0, 1},
        {"restore-min",   no_argument,       0, 2},
        {"restore-factory",no_argument,      0, 3},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:hv", long_options, NULL)) != -1) {
        switch (opt) {
        case 'c':
            config_path = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case 1:
            restore_mode = 'M';
            break;
        case 2:
            restore_mode = 'm';
            break;
        case 3:
            restore_mode = 'F';
            break;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!config_path) {
        fprintf(stderr, "error: configuration file is required\n");
        print_usage(argv[0]);
        return 1;
    }

    struct config *cfg = config_load(config_path);
    if (!cfg) {
        fprintf(stderr, "error: failed to load config '%s'\n", config_path);
        return 1;
    }

    emit_verbose("config loaded: %d GPU(s), poll %d ms, samples %d, hysteresis %d C",
                 cfg->gpu_count, cfg->global.poll_interval, cfg->global.avg_samples,
                 cfg->global.hysteresis);

    int sys_gpu_count = gpu_count();
    emit_verbose("system GPU count: %d", sys_gpu_count);

    if (sys_gpu_count != cfg->gpu_count) {
        fprintf(stderr,
                "error: system has %d GPU(s) but config defines %d\n",
                sys_gpu_count, cfg->gpu_count);
        config_free(cfg);
        return 1;
    }

    if (restore_mode) {
        do_restore(cfg, restore_mode);
        config_free(cfg);
        return 0;
    }

    /* --- main loop --- */
    struct gpu_state **states = gpu_state_init(cfg);

    /* set initial power to max */
    for (int i = 0; i < cfg->gpu_count; i++) {
        emit_log("GPU %d: initial power %d W (max %d, min %d, step_up %d, step_down %d, max_temp %d C)",
            i, states[i]->current_power,
            cfg->gpus[i].max_power, cfg->gpus[i].min_power,
            cfg->gpus[i].power_step_up, cfg->gpus[i].power_step_down,
            cfg->gpus[i].max_temp);
        gpu_set_power(i, states[i]->current_power, NULL, 0);
    }

    emit_log("starting regulation loop (poll %d ms, samples %d, hysteresis %d C)",
        cfg->global.poll_interval, cfg->global.avg_samples,
        cfg->global.hysteresis);

    while (1) {
        /* read temperatures */
        for (int i = 0; i < cfg->gpu_count; i++) {
            int temp = gpu_read_temp(i);
            struct gpu_state *s = states[i];
            int samples = cfg->global.avg_samples;

            s->temp_buffer[s->temp_index] = temp;
            s->temp_index = (s->temp_index + 1) % samples;
            if (s->temp_count < samples)
                s->temp_count++;

            emit_verbose("GPU %d: temp %d C (sample %d/%d)",
                         i, temp, s->temp_count, samples);
        }

        /* compute and apply regulation */
        for (int i = 0; i < cfg->gpu_count; i++) {
            struct gpu_state *s = states[i];
            int avg = compute_avg_temp(s);

            int new_power = regulate_compute(avg, &cfg->gpus[i], cfg->global.hysteresis,
                                             s->current_power);

            emit_verbose("GPU %d: avg %d C, power %d -> %d W",
                         i, avg, s->current_power, new_power);

            if (new_power != s->current_power) {
                emit_log("GPU %d: temp avg %d C -> power %d -> %d W",
                    i, avg, s->current_power, new_power);
                s->current_power = new_power;
                char buf[MAIN_OUTPUT_BUF] = {0};
                gpu_set_power(i, new_power, verbose ? buf : NULL, sizeof(buf));
                if (verbose && buf[0])
                    emit_indented(buf);
            }
        }

        usleep(cfg->global.poll_interval * MS_PER_SEC);
    }

    /* unreachable, but for clarity */
    gpu_state_free(states, cfg->gpu_count);
    config_free(cfg);
    return 0;
}
