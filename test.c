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
    assert(cfg->global.power_step == 1);
    assert(cfg->global.hysteresis == 3);

    assert(cfg->gpus[0].id == 0);
    assert(cfg->gpus[0].max_temp == 80);
    assert(cfg->gpus[0].max_power == 300);
    assert(cfg->gpus[0].min_power == 50);

    assert(cfg->gpus[1].id == 1);
    assert(cfg->gpus[1].max_temp == 75);
    assert(cfg->gpus[1].max_power == 250);
    assert(cfg->gpus[1].min_power == 50);

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
    assert(cfg->global.power_step == 2);
    assert(cfg->global.hysteresis == 5);

    assert(cfg->gpus[0].max_temp == 70);
    assert(cfg->gpus[0].max_power == 200);
    assert(cfg->gpus[0].min_power == 100);

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
    assert(cfg->global.power_step == 1);
    assert(cfg->global.hysteresis == 3);

    assert(cfg->gpus[0].max_temp == 80);
    assert(cfg->gpus[0].max_power == 300);
    assert(cfg->gpus[0].min_power == 50);

    config_free(cfg);
    return 0;
}

/* ==================== Regulation algorithm tests ==================== */

static struct gpu_config test_gpu_cfg;
static struct global_config test_global_cfg;

static void setup_regulate(void)
{
    test_gpu_cfg.id = 0;
    test_gpu_cfg.max_temp = 80;
    test_gpu_cfg.max_power = 300;
    test_gpu_cfg.min_power = 50;

    test_global_cfg.poll_interval = 1000;
    test_global_cfg.avg_samples = 5;
    test_global_cfg.power_step = 10;
    test_global_cfg.hysteresis = 3;
}

static int test_regulate_overheat(void)
{
    setup_regulate();
    /* temp 85 >= max_temp 80 -> decrease by 10 */
    int new_power = regulate_compute(85, &test_gpu_cfg, &test_global_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_overheat_at_boundary(void)
{
    setup_regulate();
    /* temp 80 == max_temp 80 -> decrease by 10 */
    int new_power = regulate_compute(80, &test_gpu_cfg, &test_global_cfg, 200);
    assert(new_power == 190);
    return 0;
}

static int test_regulate_overheat_clamped_to_min(void)
{
    setup_regulate();
    /* temp 85, current 55, step 10 -> 45, but min is 50 */
    int new_power = regulate_compute(85, &test_gpu_cfg, &test_global_cfg, 55);
    assert(new_power == 50);
    return 0;
}

static int test_regulate_overheat_already_at_min(void)
{
    setup_regulate();
    /* temp 85, current 50 (already min) -> stays 50 */
    int new_power = regulate_compute(85, &test_gpu_cfg, &test_global_cfg, 50);
    assert(new_power == 50);
    return 0;
}

static int test_regulate_cool(void)
{
    setup_regulate();
    /* temp 70 <= max_temp 80 - hysteresis 3 = 77 -> increase by 10 */
    int new_power = regulate_compute(70, &test_gpu_cfg, &test_global_cfg, 200);
    assert(new_power == 210);
    return 0;
}

static int test_regulate_cool_at_boundary(void)
{
    setup_regulate();
    /* temp 77 == max_temp 80 - hysteresis 3 -> increase by 10 */
    int new_power = regulate_compute(77, &test_gpu_cfg, &test_global_cfg, 200);
    assert(new_power == 210);
    return 0;
}

static int test_regulate_cool_clamped_to_max(void)
{
    setup_regulate();
    /* temp 70, current 305, step 10 -> 315, but max is 300 */
    int new_power = regulate_compute(70, &test_gpu_cfg, &test_global_cfg, 305);
    assert(new_power == 300);
    return 0;
}

static int test_regulate_cool_already_at_max(void)
{
    setup_regulate();
    /* temp 70, current 300 (already max) -> stays 300 */
    int new_power = regulate_compute(70, &test_gpu_cfg, &test_global_cfg, 300);
    assert(new_power == 300);
    return 0;
}

static int test_regulate_in_hysteresis_band(void)
{
    setup_regulate();
    /* temp 78: 78 < 80 and 78 > 77 -> no change */
    int new_power = regulate_compute(78, &test_gpu_cfg, &test_global_cfg, 200);
    assert(new_power == 200);
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

    printf("\nRegulation algorithm tests:\n");
    test("overheat", test_regulate_overheat);
    test("overheat_at_boundary", test_regulate_overheat_at_boundary);
    test("overheat_clamped_to_min", test_regulate_overheat_clamped_to_min);
    test("overheat_already_at_min", test_regulate_overheat_already_at_min);
    test("cool", test_regulate_cool);
    test("cool_at_boundary", test_regulate_cool_at_boundary);
    test("cool_clamped_to_max", test_regulate_cool_clamped_to_max);
    test("cool_already_at_max", test_regulate_cool_already_at_max);
    test("in_hysteresis_band", test_regulate_in_hysteresis_band);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
