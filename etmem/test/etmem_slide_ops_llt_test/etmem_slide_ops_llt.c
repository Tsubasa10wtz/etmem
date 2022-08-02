/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: liubo
 * Create: 2021-11-29
 * Description: This is a source file of the unit test for project-related commands in etmem.
 ******************************************************************************/

#include <sys/file.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_project.h"
#include "etmemd_task.h"
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_cslide.h"
#include "etmemd_rpc.h"
#include "securec.h"

#include "test_common.h"

#include "etmemd_slide.c"

#define PID_STR_MAX_LEN         10
#define PID_TEST_NUM            100
#define PID_PROCESS_MEM         5000
#define PID_PROCESS_SLEEP_TIME  60
#define WATER_LINT_TEMP         3
#define RAND_STR_ARRAY_LEN      62

static void test_engine_name_invalid(void)
{
    struct eng_test_param slide_param;
    GKeyFile *config = NULL;

    init_slide_eng(&slide_param);

    slide_param.name = "engine no exist";
    config = construct_eng_config(&slide_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);

    slide_param.name = "";
    config = construct_eng_config(&slide_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);

    slide_param.name = NULL;
    config = construct_eng_config(&slide_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);
}

static void test_engine_proj_invalid(void)
{
    struct eng_test_param slide_param;
    GKeyFile *config = NULL;

    init_slide_eng(&slide_param);

    slide_param.proj = "proj no exist";
    config = construct_eng_config(&slide_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_PRO_NOEXIST);
    destroy_eng_config(config);

    slide_param.proj = "";
    config = construct_eng_config(&slide_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);

    slide_param.proj = NULL;
    config = construct_eng_config(&slide_param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);
}

void test_etmem_slide_engine_002(void)
{
    struct proj_test_param proj_param;
    struct eng_test_param slide_eng;
    struct cslide_eng_test_param cslide_eng;
    GKeyFile *config = NULL;

    init_proj_param(&proj_param);
    do_add_proj_test(&proj_param);

    init_cslide_eng(&cslide_eng);
    config = construct_cslide_eng_config(&cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* repeat add and remove engine */
    init_slide_eng(&slide_eng);
    config = construct_eng_config(&slide_eng);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_ENG_EXISTED);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_ENG_NOEXIST);
    destroy_eng_config(config);

    test_engine_name_invalid();
    test_engine_proj_invalid();

    init_cslide_eng(&cslide_eng);
    config = construct_cslide_eng_config(&cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    do_rm_proj_test(&proj_param);
}

void test_etmem_slide_engine_001(void)
{
    struct proj_test_param proj_param;
    struct eng_test_param slide_eng;
    struct slide_task_test_param slide_task;
    GKeyFile *eng_config = NULL;
    GKeyFile *task_config = NULL;

    init_proj_param(&proj_param);
    do_add_proj_test(&proj_param);

    init_slide_eng(&slide_eng);
    eng_config = construct_eng_config(&slide_eng);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(eng_config), OPT_SUCCESS);

    init_slide_task(&slide_task);
    task_config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(task_config), OPT_SUCCESS);
    destroy_slide_task_config(task_config);

    CU_ASSERT_EQUAL(etmemd_project_remove_engine(eng_config), OPT_SUCCESS);
    destroy_eng_config(eng_config);

    do_rm_proj_test(&proj_param);
}

static void task_test_init(void)
{
    struct proj_test_param proj_param;
    struct eng_test_param eng_param;
    struct cslide_eng_test_param cslide_eng;
    GKeyFile *config = NULL;

    init_proj_param(&proj_param);
    do_add_proj_test(&proj_param);

    init_cslide_eng(&cslide_eng);
    config = construct_cslide_eng_config(&cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    init_slide_eng(&eng_param);
    config = construct_eng_config(&eng_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);
}

static void task_test_fini(void)
{
    struct proj_test_param proj_param;
    struct eng_test_param eng_param;
    struct cslide_eng_test_param cslide_eng;
    GKeyFile *config = NULL;

    init_cslide_eng(&cslide_eng);
    config = construct_cslide_eng_config(&cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    init_slide_eng(&eng_param);
    config = construct_eng_config(&eng_param);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_eng_config(config);

    init_proj_param(&proj_param);
    do_rm_proj_test(&proj_param);
}

static void test_etmem_slide_task_001(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    task_test_init();

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task2";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);
    init_slide_task(&slide_task);
    slide_task.task_param.name = "task2";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    task_test_fini();
}

static void test_etmem_task_swap_flag_error(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    task_test_init();

    /* empty value of swap_flag */
    init_slide_task(&slide_task);
    slide_task.swap_flag = "";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of swap_flag */
    init_slide_task(&slide_task);
    slide_task.swap_flag = "wrong";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of swap_flag */
    init_slide_task(&slide_task);
    slide_task.swap_flag = "YES";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of swap_flag */
    init_slide_task(&slide_task);
    slide_task.swap_flag = "NO";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of pid which is equal to 0 */
    init_slide_task(&slide_task);
    slide_task.task_param.value = "0";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    /* sleep 10 seconds to run etmemd project for coverage of slide_executor */
    sleep(10);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* run slide_do_migrate fail */
    CU_ASSERT_EQUAL(slide_do_migrate(1, NULL), -1);

    task_test_fini();
}

static void test_etmem_task_swap_flag_ok(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    task_test_init();

    /* empty value of swap_flag */
    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1_swap_flag_yes";
    slide_task.swap_flag = "yes";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1_swap_flag_no";
    slide_task.swap_flag = "no";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    task_test_fini();
}

static void test_etmem_task_swap_threshold_error(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    task_test_init();

    /* empty value of swap_threshold */
    init_slide_task(&slide_task);
    slide_task.swap_threshold = "";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* swap_threshold too long */
    init_slide_task(&slide_task);
    slide_task.swap_threshold = "12345678910234g";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* swap_threshold para is wrong*/
    init_slide_task(&slide_task);
    slide_task.swap_threshold = "12hhG";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* swap_threshold para is wrong*/
    init_slide_task(&slide_task);
    slide_task.swap_threshold = "1234";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of pid which is equal to 0 */
    init_slide_task(&slide_task);
    slide_task.task_param.value = "0";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    /* sleep 10 seconds to run etmemd project for coverage of slide_executor */
    sleep(10);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* run slide_do_migrate fail */
    CU_ASSERT_EQUAL(slide_do_migrate(1, NULL), -1);

    task_test_fini();
}

static void test_etmem_task_swap_threshold_ok(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    task_test_init();

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1_swap_threshold_5G";
    slide_task.swap_threshold = "5G";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task1_swap_threshold_5g";
    slide_task.swap_threshold = "5g";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task2_swap_threshold";
    slide_task.swap_threshold = "123456789g";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    slide_task.task_param.name = "task3_swap_threshold";
    slide_task.swap_threshold = "999999999g";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    task_test_fini();
}

static void test_task_common_invalid_config(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;
    char *long_name = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

    /* repeat add */
    init_slide_task(&slide_task);
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_TASK_EXISTED);
    destroy_slide_task_config(config);

    /* no exist remove */
    slide_task.task_param.name = "task no exists";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_TASK_NOEXIST);
    destroy_slide_task_config(config);

    /* repeat remove */
    init_slide_task(&slide_task);
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_TASK_NOEXIST);
    destroy_slide_task_config(config);

    /* no project exist */
    init_slide_task(&slide_task);
    slide_task.task_param.proj = "proj no exists";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_PRO_NOEXIST);
    destroy_slide_task_config(config);

    /* not project set */
    slide_task.task_param.proj = NULL;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* long project name */
    slide_task.task_param.proj = long_name;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* no engine exist */
    init_slide_task(&slide_task);
    slide_task.task_param.eng = "task no exists";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_ENG_NOEXIST);
    destroy_slide_task_config(config);

    /* no engine set */
    slide_task.task_param.eng = NULL;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* long engine name */
    slide_task.task_param.eng = long_name;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* no name set for task */
    init_slide_task(&slide_task);
    slide_task.task_param.name = NULL;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* task name too long */
    slide_task.task_param.name = long_name;
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_INVAL);
    destroy_slide_task_config(config);

    /* type invalid */
    init_slide_task(&slide_task);
    slide_task.task_param.type = "invalid";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* task value invalid */
    init_slide_task(&slide_task);
    slide_task.task_param.value = "";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* task value invalid */
    init_slide_task(&slide_task);
    slide_task.task_param.type = "name*^";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);
}

static void test_task_slide_invalid_config(void)
{
    struct slide_task_test_param slide_task;
    GKeyFile *config = NULL;

    init_slide_task(&slide_task);

    /* enpty value of T */
    slide_task.T = "";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of T which is bigger than WEIGHT * loop times */
    slide_task.T = "10";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of T which is less than 0 */
    slide_task.T = "-1";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    /* wrong value of max_threads which is equal to 0, slide will correct it
     * to valid value
     */
    slide_task.max_threads = "0";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of max_threads which is equal to -1, slide will correct it
     * to valid value
     */
    slide_task.max_threads = "-1";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* wrong value of max_threads which is too big, slide will correct it
     * to valid value
     */
    slide_task.max_threads = "10000";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);
    destroy_slide_task_config(config);

    init_slide_task(&slide_task);
    /* wrong value of pid which is equal to 0 */
    slide_task.task_param.value = "0";
    config = construct_slide_task_config(&slide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    /* sleep 10 seconds to run etmemd project for coverage of slide_executor */
    sleep(10);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_slide_task_config(config);

    /* run slide_do_migrate fail */
    CU_ASSERT_EQUAL(slide_do_migrate(1, NULL), -1);
}

void test_etmem_slide_task_002(void)
{
    task_test_init();

    test_task_common_invalid_config();
    test_task_slide_invalid_config();

    task_test_fini();
}

typedef enum {
    CUNIT_SCREEN = 0,
    CUNIT_XMLFILE,
    CUNIT_CONSOLE
} cu_run_mode;

static void test_slide(void)
{
    CU_ASSERT_EQUAL(system("../etmem_slide_ops_llt_test/test_slide_ops.sh"), 0);
}

int main(int argc, const char **argv)
{
    CU_pSuite suite;
    CU_pTest pTest;
    unsigned int num_failures;
    cu_run_mode cunit_mode = CUNIT_SCREEN;
    int error_num;

    if (argc > 1) {
        cunit_mode = atoi(argv[1]);
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return -CU_get_error();
    }

    suite = CU_add_suite("etmem_slide_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_etmem_slide_task_002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_slide_task_001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_slide_engine_002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_slide_engine_001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_task_swap_flag_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_task_swap_flag_ok) == NULL ||
        CU_ADD_TEST(suite, test_etmem_task_swap_threshold_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_task_swap_threshold_ok) == NULL ||
        CU_ADD_TEST(suite, test_slide) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_slide.c");
            CU_automated_run_tests();
            break;
        case CUNIT_CONSOLE:
            CU_console_run_tests();
            break;
        default:
            printf("not support cunit mode, only support: "
                   "0 for CUNIT_SCREEN, 1 for CUNIT_XMLFILE, 2 for CUNIT_CONSOLE\n");
            goto ERROR;
    }

    num_failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return num_failures;

ERROR:
    error_num = CU_get_error();
    CU_cleanup_registry();
    return -error_num;
}
