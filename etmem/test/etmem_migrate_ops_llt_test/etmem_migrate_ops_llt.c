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
 * Create: 2021-08-14
 * Description: This is a source file of the unit test for log functions in etmem.
 ******************************************************************************/

#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "etmemd.h"
#include "etmemd_migrate.h"
#include "etmemd_scan.h"
#include "etmemd_project_exp.h"
#include "etmemd_engine_exp.h"
#include "etmemd_task_exp.h"
#include "etmemd_task.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#define WATER_LINE_TEMP 2

/* Function replacement used for mock test. This function is used only in dt. */
int get_mem_from_proc_file(const char *pid, const char *file_name,
                           unsigned long *data, const char *cmpstr)
{
    *data = 100;
    return 0;
}

void init_task_pid_param(struct task_pid *param)
{
    param->pid = 1;
    param->rt_swapin_rate = 0.0;
    param->params = NULL;
    param->next = NULL;
}

static struct memory_grade *get_memory_grade(void)
{
    struct memory_grade *memory_grade = NULL;
    struct vmas *vmas = NULL;
    struct page_refs *page_refs = NULL;
    const char *pid = "1";

    init_g_page_size();
    vmas = get_vmas(pid);
    CU_ASSERT_PTR_NOT_NULL(vmas);
    CU_ASSERT_EQUAL(get_page_refs(vmas, pid, &page_refs, NULL, NULL), 0);
    free(vmas);
    vmas = NULL;

    memory_grade = (struct memory_grade *)calloc(1, sizeof(struct memory_grade));
    CU_ASSERT_PTR_NOT_NULL(memory_grade);

    while (page_refs != NULL) {
        if ((page_refs)->count >= WATER_LINE_TEMP) {
            page_refs = add_page_refs_into_memory_grade(page_refs, &memory_grade->hot_pages);
            continue;
        }
        page_refs = add_page_refs_into_memory_grade(page_refs, &memory_grade->cold_pages);
    }

    return memory_grade;
}

static void test_etmem_migrate_error(void)
{
    struct memory_grade *memory_grade = NULL;

    memory_grade = (struct memory_grade *)calloc(1, sizeof(struct memory_grade));
    CU_ASSERT_PTR_NOT_NULL(memory_grade);

    CU_ASSERT_EQUAL(etmemd_grade_migrate("", memory_grade), 0);
    CU_ASSERT_EQUAL(etmemd_grade_migrate("no123", memory_grade), 0);

    free(memory_grade);

    memory_grade = get_memory_grade();
    CU_ASSERT_PTR_NOT_NULL(memory_grade);
    CU_ASSERT_EQUAL(etmemd_grade_migrate("", memory_grade), -1);
    CU_ASSERT_EQUAL(etmemd_grade_migrate("no123", memory_grade), -1);

    clean_memory_grade_unexpected(&memory_grade);
    CU_ASSERT_PTR_NULL(memory_grade);
}

static void test_etmem_migrate_ok(void)
{
    struct memory_grade *memory_grade = NULL;

    memory_grade = get_memory_grade();
    CU_ASSERT_PTR_NOT_NULL(memory_grade);
    CU_ASSERT_EQUAL(etmemd_grade_migrate("1", memory_grade), 0);

    clean_memory_grade_unexpected(&memory_grade);
    CU_ASSERT_PTR_NULL(memory_grade);
}

static void test_etmemd_reclaim_swapcache_error(void)
{
    struct project proj = {0};
    struct engine eng = {0};
    struct task tk = {0};
    struct task_pid tk_pid = {0};

    proj.swapcache_high_wmark = 5;
    proj.swapcache_low_wmark = 3;
    tk_pid.tk = &tk;
    tk.eng = &eng;
    eng.proj = &proj;

    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(NULL), -1);

    init_task_pid_param(&tk_pid);
    tk_pid.pid = 0;
    proj.wmark_set = false;
    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(&tk_pid), -1);

    tk_pid.pid = 0;
    proj.wmark_set = true;
    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(&tk_pid), -1);
}

static void test_etmemd_reclaim_swapcache_ok(void)
{
    struct project proj = {0};
    struct engine eng = {0};
    struct task tk = {0};
    struct task_pid tk_pid = {0};

    tk_pid.tk = &tk;
    tk.eng = &eng;
    eng.proj = &proj;

    init_task_pid_param(&tk_pid);
    proj.swapcache_high_wmark = -1;
    proj.swapcache_low_wmark = -1;
    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(&tk_pid), 0);

    proj.swapcache_high_wmark = 100;
    proj.swapcache_low_wmark = 3;
    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(&tk_pid), 0);

    init_task_pid_param(&tk_pid);
    proj.swapcache_high_wmark = 5;
    proj.swapcache_low_wmark = 3;
    proj.wmark_set = false;
    CU_ASSERT_EQUAL(etmemd_reclaim_swapcache(&tk_pid), 0);
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
        return -CU_get_error();
    }

    suite = CU_add_suite("etmem_migrate_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_etmem_migrate_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_migrate_ok) == NULL ||
        CU_ADD_TEST(suite, test_etmemd_reclaim_swapcache_error) == NULL ||
        CU_ADD_TEST(suite, test_etmemd_reclaim_swapcache_ok) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_migrate.c");
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
