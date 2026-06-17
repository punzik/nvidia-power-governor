#define _GNU_SOURCE
#include "config.h"
#include "regulate.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run;
static int tests_passed;

static void test(const char *name, int (*fn)(void))
{
    tests_run++;
    if (fn() == 0) {
        tests_passed++;
        printf("  PASS %s\n", name);
    } else {
        printf("  FAIL %s\n", name);
    }
}

/* Helper: check config loaded and fields match */
static struct config *load_or_fail(const char *path)
{
    struct config *cfg = config_load(path);
    return cfg;
}

/* ==================== Config parser tests ==================== */

static int test_valid_conf(void)
{
    struct config *cfg = load_or_fail("tests/valid.conf");
    assert(cfg != NULL);
    assert(cfg->gpu_count == 2);

    assert(cfg->global.poll_interval == 1000);
    assert(cfg->global.avg_samples == 5);
    assert(cfg->global.draw_avg_samples == 5);

    assert(cfg->gpus[0].temp_threshold_high == 80);
    assert(cfg->gpus[0].temp_threshold_low == 65);
    assert(cfg->gpus[0].max_power == 300);
    assert(cfg->gpus[0].min_power == 50);
    assert(cfg->gpus[0].power_step_down_temp == 10);
    assert(cfg->gpus[0].power_step_down_draw == 10);
    assert(cfg->gpus[0].power_step_up_draw == 10);
    assert(cfg->gpus[0].power_draw_offset_down == 20);
    assert(cfg->gpus[0].power_draw_offset_up == 10);

    assert(cfg->gpus[1].temp_threshold_high == 75);
    assert(cfg->gpus[1].temp_threshold_low == 60);
    assert(cfg->gpus[1].max_power == 250);
    assert(cfg->gpus[1].min_power == 50);
    assert(cfg->gpus[1].power_step_down_temp == 10);
    assert(cfg->gpus[1].power_step_down_draw == 10);
    assert(cfg->gpus[1].power_step_up_draw == 10);
    assert(cfg->gpus[1].power_draw_offset_down == 20);
    assert(cfg->gpus[1].power_draw_offset_up == 10);

    config_free(cfg);
    return 0;
}

static int test_valid_single_conf(void)
{
    struct config *cfg = load_or_fail("tests/valid_single.conf");
    assert(cfg != NULL);
    assert(cfg->gpu_count == 1);

    assert(cfg->global.poll_interval == 500);
    assert(cfg->global.avg_samples == 3);
    assert(cfg->global.draw_avg_samples == 3);

    assert(cfg->gpus[0].temp_threshold_high == 70);
    assert(cfg->gpus[0].temp_threshold_low == 55);
    assert(cfg->gpus[0].max_power == 200);
    assert(cfg->gpus[0].min_power == 100);
    assert(cfg->gpus[0].power_step_down_temp == 10);
    assert(cfg->gpus[0].power_step_down_draw == 5);
    assert(cfg->gpus[0].power_step_up_draw == 10);
    assert(cfg->gpus[0].power_draw_offset_down == 20);
    assert(cfg->gpus[0].power_draw_offset_up == 10);

    config_free(cfg);
    return 0;
}

static int test_empty_conf(void)
{
    struct config *cfg = load_or_fail("tests/empty.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_missing_file(void)
{
    struct config *cfg = load_or_fail("tests/nonexistent.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_comments_only_conf(void)
{
    struct config *cfg = load_or_fail("tests/comments.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_bad_value_conf(void)
{
    struct config *cfg = load_or_fail("tests/bad_value.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_bad_key_conf(void)
{
    struct config *cfg = load_or_fail("tests/bad_key.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_missing_section_conf(void)
{
    struct config *cfg = load_or_fail("tests/missing_section.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_no_value_conf(void)
{
    /* poll_interval= (empty value) -> strtol fails */
    struct config *cfg = load_or_fail("tests/no_value.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_no_key_conf(void)
{
    /* =42 (empty key) -> unknown key -> NULL */
    struct config *cfg = load_or_fail("tests/no_key.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_multi_equals_conf(void)
{
    /* poll_interval=1000=500 -> strtol stops at '=' -> NULL */
    struct config *cfg = load_or_fail("tests/multi_equals.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_spaces_in_key_conf(void)
{
    /* "poll interval" (space instead of underscore) -> unknown key -> NULL */
    struct config *cfg = load_or_fail("tests/spaces_in_key.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_long_value_conf(void)
{
    /* 512-char value -> exceeds line buffer -> NULL */
    struct config *cfg = load_or_fail("tests/long_value.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_long_key_conf(void)
{
    /* 512-char section name -> exceeds line buffer -> NULL */
    struct config *cfg = load_or_fail("tests/long_key.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_long_line_conf(void)
{
    /* 1024-char line -> exceeds line buffer -> NULL */
    struct config *cfg = load_or_fail("tests/long_line.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_whitespace_conf(void)
{
    /* spaces at line start, around '=', and at line end -> should parse OK */
    struct config *cfg = load_or_fail("tests/whitespace.conf");
    assert(cfg != NULL);
    assert(cfg->gpu_count == 1);

    assert(cfg->global.poll_interval == 1000);
    assert(cfg->global.avg_samples == 5);
    assert(cfg->global.draw_avg_samples == 5);

    assert(cfg->gpus[0].temp_threshold_high == 80);
    assert(cfg->gpus[0].temp_threshold_low == 65);
    assert(cfg->gpus[0].max_power == 300);
    assert(cfg->gpus[0].min_power == 50);
    assert(cfg->gpus[0].power_step_down_temp == 10);
    assert(cfg->gpus[0].power_step_down_draw == 10);
    assert(cfg->gpus[0].power_step_up_draw == 10);
    assert(cfg->gpus[0].power_draw_offset_down == 20);
    assert(cfg->gpus[0].power_draw_offset_up == 10);

    config_free(cfg);
    return 0;
}

static int test_empty_gpu_value_conf(void)
{
    /* temp_threshold_high= (empty value in [gpu.N]) -> parse_int rejects empty string -> NULL */
    struct config *cfg = load_or_fail("tests/empty_gpu_value.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_empty_gpu_section_conf(void)
{
    /* [gpu.] (no index after 'gpu.') -> must be rejected -> NULL */
    struct config *cfg = load_or_fail("tests/empty_gpu_section.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_min_gt_max_conf(void)
{
    /* min_power > max_power -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_min_gt_max.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_threshold_low_ge_high_conf(void)
{
    /* temp_threshold_low >= temp_threshold_high -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_threshold_low_ge_high.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_draw_offset_up_ge_down_conf(void)
{
    /* power_draw_offset_up >= power_draw_offset_down -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_draw_offset_up_ge_down.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_draw_offset_up_eq_down_conf(void)
{
    /* power_draw_offset_up == power_draw_offset_down -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_draw_offset_up_eq_down.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_poll_interval_conf(void)
{
    /* poll_interval=70000 (> 60000) -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_poll_interval.conf");
    assert(cfg == NULL);
    return 0;
}

static int test_invalid_avg_samples_conf(void)
{
    /* avg_samples=200 (> 100) -> rejected -> NULL */
    struct config *cfg = load_or_fail("tests/invalid_avg_samples.conf");
    assert(cfg == NULL);
    return 0;
}

/* ==================== Regulation algorithm tests ==================== */

static struct gpu_config test_gpu_cfg;

static void setup_regulate(void)
{
    test_gpu_cfg.temp_threshold_high = 80;
    test_gpu_cfg.temp_threshold_low = 65;
    test_gpu_cfg.max_power = 300;
    test_gpu_cfg.min_power = 50;
    test_gpu_cfg.power_step_down_temp = 10;
    test_gpu_cfg.power_step_down_draw = 10;
    test_gpu_cfg.power_step_up_draw = 15;
    test_gpu_cfg.power_draw_offset_down = 20;
    test_gpu_cfg.power_draw_offset_up = 10;
}

static int test_regulate_overheat(void)
{
    setup_regulate();
    /* temp 85 >= threshold_high 80 -> decrease by step_down_temp 10 */
    int new_power = regulate_compute(85, 250, &test_gpu_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_overheat_at_boundary(void)
{
    setup_regulate();
    /* temp 80 == threshold_high 80 -> decrease by step_down_temp 10 */
    int new_power = regulate_compute(80, 250, &test_gpu_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_overheat_clamped_to_min(void)
{
    setup_regulate();
    /* temp 85, power_limit 55, step_down_temp 10 -> 45, but min is 50 */
    int new_power = regulate_compute(85, 250, &test_gpu_cfg, 55);
    assert(new_power == 50);
    return 0;
}

static int test_regulate_overheat_already_at_min(void)
{
    setup_regulate();
    /* temp 85, power_limit 50 (already min) -> stays 50 */
    int new_power = regulate_compute(85, 250, &test_gpu_cfg, 50);
    assert(new_power == 50);
    return 0;
}

static int test_regulate_cool_low_draw(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * threshold_down = 200 - 20 = 180, threshold_up = 200 - 10 = 190
     * draw 170 <= 180 -> decrease by step_down_draw 10 */
    int new_power = regulate_compute(60, 170, &test_gpu_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_cool_high_draw(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * threshold_down = 200 - 20 = 180, threshold_up = 200 - 10 = 190
     * draw 192 >= 190 -> increase by step_up_draw 15 */
    int new_power = regulate_compute(60, 192, &test_gpu_cfg, 200);
    assert(new_power == 215);
    return 0;
}

static int test_regulate_cool_draw_hysteresis(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * threshold_down = 200 - 20 = 180, threshold_up = 200 - 10 = 190
     * draw 185: 185 > 180 and 185 < 190 -> no change */
    int new_power = regulate_compute(60, 185, &test_gpu_cfg, 200);
    assert(new_power == 200);
    return 0;
}

static int test_regulate_cool_idle(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * threshold_down = 200 - 20 = 180
     * draw 0 <= 180 -> decrease by step_down_draw 10 */
    int new_power = regulate_compute(60, 0, &test_gpu_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_middle_zone(void)
{
    setup_regulate();
    /* temp 72: 65 < 72 < 80 -> middle zone, no change regardless of draw */
    int new_power = regulate_compute(72, 250, &test_gpu_cfg, 200);
    assert(new_power == 200);
    return 0;
}

static int test_regulate_temp_priority(void)
{
    setup_regulate();
    /* temp 82 >= threshold_high 80 -> overheat zone, temperature wins
     * draw 295 is very high (would want increase) but temp has priority */
    int new_power = regulate_compute(82, 295, &test_gpu_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_draw_clamped_to_min(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * power_limit 55, threshold_down = 55 - 20 = 35
     * draw 10 <= 35 -> decrease: 55 - 10 = 45, clamped to min 50 */
    int new_power = regulate_compute(60, 10, &test_gpu_cfg, 55);
    assert(new_power == 50);
    return 0;
}

static int test_regulate_draw_clamped_to_max(void)
{
    setup_regulate();
    /* temp 60 <= threshold_low 65 -> draw zone
     * power_limit 295, threshold_up = 295 - 10 = 285
     * draw 288 >= 285 -> increase: 295 + 15 = 310, clamped to max 300 */
    int new_power = regulate_compute(60, 288, &test_gpu_cfg, 295);
    assert(new_power == 300);
    return 0;
}

/* ==================== Average computation tests ==================== */

static int test_avg_temp_empty(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    assert(compute_avg_temp(&s) == 0);
    return 0;
}

static int test_avg_temp_partial(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    /* fill 3 of 5 samples: 80, 82, 78 */
    s.temp_buffer[0] = 80;
    s.temp_buffer[1] = 82;
    s.temp_buffer[2] = 78;
    s.temp_index = 3;
    s.temp_count = 3;
    /* (80 + 82 + 78) / 3 = 240 / 3 = 80 */
    assert(compute_avg_temp(&s) == 80);
    return 0;
}

static int test_avg_temp_full(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    /* fill all 5 samples: 80, 82, 78, 81, 79 */
    s.temp_buffer[0] = 80;
    s.temp_buffer[1] = 82;
    s.temp_buffer[2] = 78;
    s.temp_buffer[3] = 81;
    s.temp_buffer[4] = 79;
    s.temp_index = 5;
    s.temp_count = 5;
    /* (80 + 82 + 78 + 81 + 79) / 5 = 400 / 5 = 80 */
    assert(compute_avg_temp(&s) == 80);
    return 0;
}

static int test_avg_temp_wrap_around(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    /* simulate ring buffer that wrapped: capacity=3, index=1, count=3
     * write order: buf[0]=80, buf[1]=82, buf[2]=78, then wrap to buf[0]=81
     * so buf = {81, 82, 78}, count=3 */
    s.temp_buffer[0] = 81;
    s.temp_buffer[1] = 82;
    s.temp_buffer[2] = 78;
    s.temp_index = 1;
    s.temp_count = 3;
    /* (81 + 82 + 78) / 3 = 241 / 3 = 80 (integer division) */
    assert(compute_avg_temp(&s) == 80);
    return 0;
}

static int test_avg_draw_empty(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    assert(compute_avg_draw(&s) == 0);
    return 0;
}

static int test_avg_draw_partial(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    /* fill 2 of 5 samples: 200, 210 */
    s.draw_buffer[0] = 200;
    s.draw_buffer[1] = 210;
    s.draw_index = 2;
    s.draw_count = 2;
    /* (200 + 210) / 2 = 205 */
    assert(compute_avg_draw(&s) == 205);
    return 0;
}

static int test_avg_draw_full(void)
{
    struct gpu_state s;
    memset(&s, 0, sizeof(s));
    /* fill all 5 samples: 200, 210, 190, 205, 195 */
    s.draw_buffer[0] = 200;
    s.draw_buffer[1] = 210;
    s.draw_buffer[2] = 190;
    s.draw_buffer[3] = 205;
    s.draw_buffer[4] = 195;
    s.draw_index = 5;
    s.draw_count = 5;
    /* (200 + 210 + 190 + 205 + 195) / 5 = 1000 / 5 = 200 */
    assert(compute_avg_draw(&s) == 200);
    return 0;
}

/* ==================== Main ==================== */

int main(void)
{
    printf("Config parser tests:\n");
    test("valid_conf", test_valid_conf);
    test("valid_single_conf", test_valid_single_conf);
    test("empty_conf", test_empty_conf);
    test("missing_file", test_missing_file);
    test("comments_only_conf", test_comments_only_conf);
    test("bad_value_conf", test_bad_value_conf);
    test("bad_key_conf", test_bad_key_conf);
    test("missing_section_conf", test_missing_section_conf);
    test("no_value_conf", test_no_value_conf);
    test("no_key_conf", test_no_key_conf);
    test("multi_equals_conf", test_multi_equals_conf);
    test("spaces_in_key_conf", test_spaces_in_key_conf);
    test("long_value_conf", test_long_value_conf);
    test("long_key_conf", test_long_key_conf);
    test("long_line_conf", test_long_line_conf);
    test("whitespace_conf", test_whitespace_conf);
    test("empty_gpu_value_conf", test_empty_gpu_value_conf);
    test("empty_gpu_section_conf", test_empty_gpu_section_conf);
    test("invalid_min_gt_max_conf", test_invalid_min_gt_max_conf);
    test("invalid_threshold_low_ge_high_conf", test_invalid_threshold_low_ge_high_conf);
    test("invalid_draw_offset_up_ge_down_conf", test_invalid_draw_offset_up_ge_down_conf);
    test("invalid_draw_offset_up_eq_down_conf", test_invalid_draw_offset_up_eq_down_conf);
    test("invalid_poll_interval_conf", test_invalid_poll_interval_conf);
    test("invalid_avg_samples_conf", test_invalid_avg_samples_conf);

    printf("\nRegulation algorithm tests:\n");
    test("overheat", test_regulate_overheat);
    test("overheat_at_boundary", test_regulate_overheat_at_boundary);
    test("overheat_clamped_to_min", test_regulate_overheat_clamped_to_min);
    test("overheat_already_at_min", test_regulate_overheat_already_at_min);
    test("cool_low_draw", test_regulate_cool_low_draw);
    test("cool_high_draw", test_regulate_cool_high_draw);
    test("cool_draw_hysteresis", test_regulate_cool_draw_hysteresis);
    test("cool_idle", test_regulate_cool_idle);
    test("middle_zone", test_regulate_middle_zone);
    test("temp_priority", test_regulate_temp_priority);
    test("draw_clamped_to_min", test_regulate_draw_clamped_to_min);
    test("draw_clamped_to_max", test_regulate_draw_clamped_to_max);

    printf("\nAverage computation tests:\n");
    test("avg_temp_empty", test_avg_temp_empty);
    test("avg_temp_partial", test_avg_temp_partial);
    test("avg_temp_full", test_avg_temp_full);
    test("avg_temp_wrap_around", test_avg_temp_wrap_around);
    test("avg_draw_empty", test_avg_draw_empty);
    test("avg_draw_partial", test_avg_draw_partial);
    test("avg_draw_full", test_avg_draw_full);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
