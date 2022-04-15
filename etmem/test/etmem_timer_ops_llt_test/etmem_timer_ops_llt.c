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
 * Create: 2020-08-13
 * Description: test for etmem timer operations
 ******************************************************************************/
#include <sys/file.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_threadtimer.h"

static int g_timer_exec_time = 0;

typedef void *(*timer_exector)(void *);

static void get_timer_expired_time(int time, int exp)
{
    timer_thread *timer = NULL;

    timer = thread_timer_create(time);
    CU_ASSERT_PTR_NOT_NULL(timer);

    CU_ASSERT_EQUAL(timer->expired_time, exp);
    thread_timer_destroy(&timer);
}

static void test_timer_create_delete(void)
{
    timer_thread *timer = NULL;
    thread_timer_destroy(&timer);

    get_timer_expired_time(1, 1);
    get_timer_expired_time(2, 2);
    get_timer_expired_time(50, 50);
    get_timer_expired_time(1199, 1199);
    get_timer_expired_time(1201, 1201);
}

static void *threadtimer_exector(void *str)
{
    char *temp_str = str;
    printf("threadtimer_exector: %s\n", temp_str);
    g_timer_exec_time++;
    return NULL;
}

static void test_timer_start_error(void)
{
    char *timer_args = "for timer start test.\n";
    timer_exector exector = threadtimer_exector;
    timer_thread *timer = NULL;

    timer = thread_timer_create(60);
    CU_ASSERT_PTR_NOT_NULL(timer);

    CU_ASSERT_EQUAL(thread_timer_start(NULL, NULL, NULL), -1);
    CU_ASSERT_EQUAL(thread_timer_start(timer, NULL, NULL), -1);
    CU_ASSERT_EQUAL(thread_timer_start(timer, exector, NULL), -1);
    CU_ASSERT_EQUAL(thread_timer_start(timer, NULL, timer_args), -1);
    CU_ASSERT_EQUAL(thread_timer_start(timer, exector, timer_args), 0);
    CU_ASSERT_FALSE(timer->down);
    CU_ASSERT_EQUAL(thread_timer_start(timer, exector, timer_args), 0);

    thread_timer_destroy(&timer);
    thread_timer_stop(timer);
}

static void test_timer_start_ok(void)
{
    char *timer_args = "for timer start test.\n";
    timer_exector exector = threadtimer_exector;
    timer_thread *timer = NULL;

    timer = thread_timer_create(1);
    CU_ASSERT_PTR_NOT_NULL(timer);

    CU_ASSERT_EQUAL(thread_timer_start(timer, exector, timer_args), 0);
    CU_ASSERT_FALSE(timer->down);
 
    sleep(2);
    CU_ASSERT_NOT_EQUAL(g_timer_exec_time, 0);

    thread_timer_stop(timer);
    thread_timer_destroy(&timer);
}

static void test_timer_stop(void)
{
    char *timer_args = "for timer start test.\n";
    timer_exector exector = threadtimer_exector;
    timer_thread *timer = NULL;

    thread_timer_stop(timer);

    timer = thread_timer_create(60);
    CU_ASSERT_PTR_NOT_NULL(timer);

    CU_ASSERT_EQUAL(thread_timer_start(timer, exector, timer_args), 0);
    CU_ASSERT_FALSE(timer->down);

    thread_timer_stop(timer);
    CU_ASSERT_TRUE(timer->down);
    thread_timer_destroy(&timer);
}

typedef enum {
    CUNIT_SCREEN = 0,
    CUNIT_XMLFILE,
    CUNIT_CONSOLE
} cu_run_mode;

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
        return CU_get_error();
    }

    suite = CU_add_suite("etmem_timer_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_timer_create_delete) == NULL ||
        CU_ADD_TEST(suite, test_timer_start_error) == NULL ||
        CU_ADD_TEST(suite, test_timer_start_ok) == NULL ||
        CU_ADD_TEST(suite, test_timer_stop) == NULL) {
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_threadtimer.c");
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
