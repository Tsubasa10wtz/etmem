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
 * Description: test for etmem threadpool operations
 ******************************************************************************/
#include <sys/file.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <glib.h>
#include <pthread.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_threadpool.h"
#include "etmemd_pool_adapter.h"
#include "etmemd_project.h"
#include "etmemd_file.h"
#include "etmemd_common.h"
#include "etmemd_engine.h"
#include "etmemd_slide.h"
#include "etmemd_cslide.h"
#include "securec.h"

#define ADD_WORKER_NUM 100
typedef void *(*add_worker_exector)(void *);

static struct task_executor g_test_exec;
static int g_task_exe_time = 0;
static struct engine_ops g_test_eng_ops = {
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
};

static void get_threadpool_create_num(int num, int exp)
{
    thread_pool *pool = NULL;

    pool = threadpool_create(num);
    CU_ASSERT_PTR_NOT_NULL(pool);

    CU_ASSERT_EQUAL(pool->max_thread_cap, exp);
    threadpool_stop_and_destroy(&pool);
}

static void test_threadpool_create(void)
{
    int core = get_nprocs();

    get_threadpool_create_num(1, 1);
    get_threadpool_create_num(2, 2);
    get_threadpool_create_num(2 * core, 2 * core);
    get_threadpool_create_num(2 * core + 1, 2 * core + 1);

    threadpool_notify(NULL);
    threadpool_reset_status(NULL);
}

static void test_threadpool_delete(void)
{
    thread_pool *pool = NULL;
    threadpool_stop_and_destroy(&pool);

    pool = threadpool_create(1);
    CU_ASSERT_PTR_NOT_NULL(pool);

    threadpool_stop_and_destroy(&pool);
    CU_ASSERT_PTR_NULL(pool);
}

static void *add_worker_fun(void *str)
{
    char *temp_str = str;
    printf("str: %s \n", temp_str);
    return NULL;
}

static unsigned int get_workerlist_num(const thread_pool *pool)
{
    unsigned int num = 0;
    thread_worker *worker = pool->worker_list;

    while (worker) {
        num++;
        worker = worker->next_node;
    }

    return num;
}

static void test_thpool_addwk_single(void)
{
    char *args = "for add worker test.\n";
    thread_pool *pool = NULL;
    add_worker_exector exector = add_worker_fun;

    pool = threadpool_create(1);
    CU_ASSERT_PTR_NOT_NULL(pool);

    CU_ASSERT_EQUAL(threadpool_add_worker(pool, NULL, NULL), -1);
    CU_ASSERT_EQUAL(threadpool_add_worker(pool, exector, NULL), -1);
    CU_ASSERT_EQUAL(threadpool_add_worker(pool, NULL, args), -1);
    CU_ASSERT_EQUAL(threadpool_add_worker(pool, exector, args), 0);

    CU_ASSERT_PTR_NOT_NULL(pool->worker_list);
    CU_ASSERT_EQUAL(get_workerlist_num(pool), 1);

    threadpool_stop_and_destroy(&pool);
    CU_ASSERT_PTR_NULL(pool);
}

static void test_thpool_addwk_mul(void)
{
    char *args = "for add worker test.\n";
    thread_pool *pool = NULL;
    int add_num;
    add_worker_exector exector = add_worker_fun;

    pool = threadpool_create(1);
    CU_ASSERT_PTR_NOT_NULL(pool);

    for (add_num = 0; add_num < ADD_WORKER_NUM; add_num++) {
        CU_ASSERT_EQUAL(threadpool_add_worker(pool, exector, args), 0);
    }

    CU_ASSERT_PTR_NOT_NULL(pool->worker_list);
    CU_ASSERT_EQUAL(__atomic_load_n(&pool->scheduing_size, __ATOMIC_SEQ_CST), ADD_WORKER_NUM);

    threadpool_stop_and_destroy(&pool);
    CU_ASSERT_PTR_NULL(pool);
}

static void init_thpool_objs(struct project *proj, struct engine *eng, struct task *tk)
{
    struct page_scan *page_scan = (struct page_scan *)calloc(1, sizeof(struct page_scan));
    CU_ASSERT_PTR_NOT_NULL(page_scan);
    proj->scan_param = page_scan;
    proj->type = PAGE_SCAN;
    page_scan->interval = 1;
    page_scan->loop = 1;
    page_scan->sleep = 1;
    proj->start = true;
    proj->name = "test_project";
    eng->proj = proj;
    eng->ops = &g_test_eng_ops;
    tk->eng = eng;
    tk->type = "pid";
    tk->value = "1";
    tk->max_threads = 10;
}

static void test_thpool_start_error(void)
{
    struct task tk;
    struct engine eng;
    struct project proj;

    init_thpool_objs(&proj, &eng, &tk);

    tk.max_threads = 0;
    g_test_exec.tk = &tk;

    CU_ASSERT_EQUAL(start_threadpool_work(&g_test_exec), -1);
    free(proj.scan_param);
}

static void *task_executor(void *arg)
{
    g_task_exe_time++;
    return NULL;
}

static void test_thpool_start_stop(void)
{
    struct task tk = {0};
    struct engine eng = {0};
    struct project proj = {0};

    init_thpool_objs(&proj, &eng, &tk);

    g_test_exec.tk = &tk;
    g_test_exec.func = task_executor;

    CU_ASSERT_EQUAL(start_threadpool_work(&g_test_exec), 0);
    /* wait threadpool to work */
    sleep(2);
    stop_and_delete_threadpool_work(&tk);
    free(proj.scan_param);
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

    suite = CU_add_suite("etmem_threadpool_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_threadpool_create) == NULL ||
        CU_ADD_TEST(suite, test_threadpool_delete) == NULL ||
        CU_ADD_TEST(suite, test_thpool_addwk_single) == NULL ||
        CU_ADD_TEST(suite, test_thpool_addwk_mul) == NULL ||
        CU_ADD_TEST(suite, test_thpool_start_stop) == NULL ||
        CU_ADD_TEST(suite, test_thpool_start_error) == NULL) {
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_threadpool.c");
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
