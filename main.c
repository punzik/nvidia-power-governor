#define _GNU_SOURCE
#include "config.h"
#include "gpu.h"
#include "regulate.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAIN_OUTPUT_BUF 256

/* Sleep for the given number of milliseconds, handling EINTR. */
static int safe_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
        ;

    return 0;
}

static void emit_timestamp(void)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    fprintf(stdout, "[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fflush(stdout);
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Options:\n"
            "  -c, --config FILE    Configuration file (required)\n"
            "  -h, --help           Show this help and exit\n"
            "  -t, --timestamp      Show timestamps in log output\n"
            "  -v, --verbose        Enable verbose output\n"
            "      --set-max        Set all GPUs to max power from config and exit\n"
            "      --set-min        Set all GPUs to min power from config and exit\n"
            "      --set-factory    Set all GPUs to factory default power and exit\n",
            prog);
}

static int verbose;
static int show_timestamp;
static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void emit_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (show_timestamp)
        emit_timestamp();
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
    fflush(stdout);
}

static void emit_verbose(const char *fmt, ...)
{
    if (!verbose)
        return;
    va_list ap;
    va_start(ap, fmt);
    if (show_timestamp)
        emit_timestamp();
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
    fflush(stdout);
}

/* Print each line of a multi-line buffer with two-space indentation and per-line timestamps. */
static void emit_indented(const char *buf)
{
    const char *p = buf;
    while (*p) {
        const char *nl = strchr(p, '\n');
        if (nl) {
            if (show_timestamp)
                emit_timestamp();
            fwrite("  ", 1, 2, stdout);
            fwrite(p, 1, (size_t)(nl - p), stdout);
            fprintf(stdout, "\n");
            p = nl + 1;
        } else {
            /* last line without newline */
            if (*p) {
                if (show_timestamp)
                    emit_timestamp();
                fwrite("  ", 1, 2, stdout);
                fwrite(p, 1, strlen(p), stdout);
                fprintf(stdout, "\n");
            }
            break;
        }
    }
    fflush(stdout);
}


static void do_set_power(const struct config *cfg, int mode)
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
            if (power < 0) {
                fprintf(stderr, "error: failed to read default power for GPU %d\n", i);
                exit(1);
            }
            break;
        default:
            return;
        }
        char buf[MAIN_OUTPUT_BUF] = {0};
        emit_log("GPU %d: setting power to %d W", i, power);
        if (gpu_set_power(i, power, verbose ? buf : NULL, sizeof(buf)) != 0) {
            fprintf(stderr, "error: failed to set power for GPU %d\n", i);
            exit(1);
        }
        if (verbose && buf[0])
            emit_indented(buf);
    }
}

int main(int argc, char *argv[])
{
    const char *config_path = NULL;
    int set_power_mode = 0;  /* 0=run loop, 'M'=max, 'm'=min, 'F'=factory */

    static struct option long_options[] = {
        {"config",        required_argument, 0, 'c'},
        {"help",          no_argument,       0, 'h'},
        {"timestamp",     no_argument,       0, 't'},
        {"verbose",       no_argument,       0, 'v'},
        {"set-max",   no_argument,       0, 1},
        {"set-min",   no_argument,       0, 2},
        {"set-factory",no_argument,      0, 3},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:htv", long_options, NULL)) != -1) {
        switch (opt) {
        case 'c':
            config_path = optarg;
            break;
        case 't':
            show_timestamp = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case 1:
            set_power_mode = 'M';
            break;
        case 2:
            set_power_mode = 'm';
            break;
        case 3:
            set_power_mode = 'F';
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
    if (!cfg)
        return 1;

    emit_verbose("config loaded: %d GPU(s), poll %d ms, temp_samples %d, draw_samples %d",
                 cfg->gpu_count, cfg->global.poll_interval, cfg->global.temp_avg_samples,
                 cfg->global.power_avg_samples);

    int sys_gpu_count = gpu_count();
    if (sys_gpu_count < 0) {
        fprintf(stderr, "error: failed to detect GPUs\n");
        config_free(cfg);
        return 1;
    }
    emit_verbose("system GPU count: %d", sys_gpu_count);

    if (sys_gpu_count != cfg->gpu_count) {
        fprintf(stderr,
                "error: system has %d GPU(s) but config defines %d\n",
                sys_gpu_count, cfg->gpu_count);
        config_free(cfg);
        return 1;
    }

    if (set_power_mode) {
        do_set_power(cfg, set_power_mode);
        config_free(cfg);
        return 0;
    }

    /* --- main loop --- */
    struct gpu_state **states = gpu_state_init(cfg);
    if (!states) {
        fprintf(stderr, "error: failed to initialize GPU state\n");
        config_free(cfg);
        return 1;
    }

    /* set up signal handlers for graceful shutdown */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* set initial power to min */
    for (int i = 0; i < cfg->gpu_count; i++) {
        emit_log("GPU %d: initial power %d W (max %d, min %d, step_down_temp %d, step_down_draw %d, step_up_draw %d, thresh_high %d C, thresh_low %d C, limit_k %f/%f)",
            i, states[i]->power_limit,
            cfg->gpus[i].max_power, cfg->gpus[i].min_power,
            cfg->gpus[i].power_step_down_temp,
            cfg->gpus[i].power_step_down_draw,
            cfg->gpus[i].power_step_up_draw,
            cfg->gpus[i].temp_threshold_high,
            cfg->gpus[i].temp_threshold_low,
            cfg->gpus[i].power_limit_down_k,
            cfg->gpus[i].power_limit_up_k);
        if (gpu_set_power(i, states[i]->power_limit, NULL, 0) != 0) {
            fprintf(stderr, "error: failed to set initial power for GPU %d\n", i);
            gpu_state_free(states, cfg->gpu_count);
            config_free(cfg);
            return 1;
        }
    }

    emit_log("starting regulation loop (poll %d ms, temp_samples %d, draw_samples %d)",
        cfg->global.poll_interval, cfg->global.temp_avg_samples, cfg->global.power_avg_samples);

    while (!stop_requested) {
        /* read temperatures and power draw */
        int iteration_ok = 1;
        for (int i = 0; i < cfg->gpu_count; i++) {
            int temp = gpu_read_temp(i);
            if (temp < 0) {
                fprintf(stderr, "warning: failed to read temperature for GPU %d, skipping iteration\n", i);
                iteration_ok = 0;
                break;
            }
            int draw = gpu_read_power_draw(i);
            if (draw < 0) {
                fprintf(stderr, "warning: failed to read power draw for GPU %d, skipping iteration\n", i);
                iteration_ok = 0;
                break;
            }
            struct gpu_state *s = states[i];
            int temp_samples = cfg->global.temp_avg_samples;
            int draw_samples = cfg->global.power_avg_samples;

            s->temp_buffer[s->temp_index] = temp;
            s->temp_index = (s->temp_index + 1) % temp_samples;
            if (s->temp_count < temp_samples)
                s->temp_count++;

            s->draw_buffer[s->draw_index] = draw;
            s->draw_index = (s->draw_index + 1) % draw_samples;
            if (s->draw_count < draw_samples)
                s->draw_count++;

            emit_verbose("GPU %d: temp %d C, draw %d W (temp_sample %d/%d, draw_sample %d/%d)",
                         i, temp, draw, s->temp_count, temp_samples, s->draw_count, draw_samples);
        }

        if (!iteration_ok) {
            safe_sleep_ms(cfg->global.poll_interval);
            continue;
        }

        /* compute and apply regulation */
        for (int i = 0; i < cfg->gpu_count; i++) {
            if (stop_requested)
                break;
            struct gpu_state *s = states[i];
            int avg_temp = compute_avg_temp(s);
            int avg_draw = compute_avg_draw(s);

            int new_power = regulate_compute(avg_temp, avg_draw, &cfg->gpus[i], s->power_limit);

            emit_verbose("GPU %d: avg_temp %d C, avg_draw %d W, power %d -> %d W",
                         i, avg_temp, avg_draw, s->power_limit, new_power);

            if (new_power != s->power_limit) {
                emit_log("GPU %d: avg_temp %d C, avg_draw %d W -> power %d -> %d W",
                    i, avg_temp, avg_draw, s->power_limit, new_power);
                s->power_limit = new_power;
                char buf[MAIN_OUTPUT_BUF] = {0};
                if (gpu_set_power(i, new_power, buf, sizeof(buf)) != 0) {
                    fprintf(stderr, "warning: failed to set power for GPU %d, will retry next iteration\n", i);
                    if (buf[0])
                        fprintf(stderr, "  nvidia-smi: %s", buf);
                } else if (verbose && buf[0]) {
                    emit_indented(buf);
                }
            }
        }

        safe_sleep_ms(cfg->global.poll_interval);
    }

    emit_log("shutting down");
    gpu_state_free(states, cfg->gpu_count);
    config_free(cfg);
    return 0;
}
