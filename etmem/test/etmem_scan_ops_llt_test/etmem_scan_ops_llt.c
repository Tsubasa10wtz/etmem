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
 * Author: shikemeng
 * Create: 2021-11-30
 * Description: test for export scan
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_scan.h"
#include "etmemd_project.h"
#include "etmemd_engine.h"

static struct task_pid *alloc_tkpid(unsigned int pid, struct task *tk)
{
    struct task_pid *tpid = NULL;

    tpid = (struct task_pid *)calloc(1, sizeof(struct task_pid));
    CU_ASSERT_PTR_NOT_NULL(tpid);
    tpid->pid = pid;
    tpid->tk = tk;

    return tpid;
}


static struct task *alloc_tk(int loop, int sleep)
{
    struct task *tk = NULL;
    struct project *proj = NULL;
    struct engine *eng = NULL;
    struct page_scan *page_scan = NULL;

    page_scan = (struct page_scan *)calloc(1, sizeof(struct page_scan));
    CU_ASSERT_PTR_NOT_NULL(page_scan);

    proj = (struct project *)calloc(1, sizeof(struct project));
    CU_ASSERT_PTR_NOT_NULL(proj);

    proj->scan_param = page_scan;
    proj->type = PAGE_SCAN;
    page_scan->loop = loop;
    page_scan->sleep = sleep;

    tk = (struct task *)calloc(1, sizeof(struct task));
    CU_ASSERT_PTR_NOT_NULL(tk);

    eng = calloc(1, sizeof(struct engine));
    eng->name = "cslide";

    eng->proj = proj;
    tk->eng = eng;

    return tk;
}

void check_vmas(struct vmas *vmas)
{
    CU_ASSERT_NOT_EQUAL(vmas->vma_cnt, 0);

    int i;
    struct vma *curr_vma = NULL;

    curr_vma = vmas->vma_list;
    for (i = 0; i < vmas->vma_cnt; i++) {
        CU_ASSERT_PTR_NOT_NULL(curr_vma);
        curr_vma = curr_vma->next;
    }
}

static void test_get_vmas_invalid(void)
{
    const char *pid = "1";
    char *vmflags_array[10] = {"rd"};
    int vmflag_num = 1;
    int is_anon_only = false;
    struct vmas *vmas = NULL;

    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    // non-exist pid
    vmas = etmemd_get_vmas("0", vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // pid is NULL
    vmas = etmemd_get_vmas(NULL, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // pid contains invalid characters
    vmas = etmemd_get_vmas("1-", vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // vmflags contains space
    vmflags_array[0] = "r ";
    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // vmflags length is not 2
    vmflags_array[0] = "r";
    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // vmflags is NULL
    vmflags_array[0] = NULL;
    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NULL(vmas);

    // test free NULL
    vmas = NULL;
    etmemd_free_vmas(vmas);

    etmemd_scan_exit();
}


static void test_get_vmas_valid(void)
{
    const char *pid = "1";
    char *vmflags_array[10] = {"rd"};
    int vmflag_num = 1;
    int is_anon_only = false;
    struct vmas *vmas = NULL;
    struct vma *curr_vma = NULL;
    int i;

    // get vmas without init
    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);

    CU_ASSERT_PTR_NOT_NULL(vmas);
    check_vmas(vmas);

    etmemd_free_vmas(vmas);

    // get vmas with init
    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);

    CU_ASSERT_PTR_NOT_NULL(vmas);
    check_vmas(vmas);

    etmemd_free_vmas(vmas);

    vmas = get_vmas("1");
    CU_ASSERT_PTR_NOT_NULL(vmas);
    CU_ASSERT_NOT_EQUAL(vmas->vma_cnt, 0);

    curr_vma = vmas->vma_list;
    for (i = 0; i < vmas->vma_cnt; i++) {
        CU_ASSERT_PTR_NOT_NULL(curr_vma);
        curr_vma = curr_vma->next;
    }

    free_vmas(vmas);
    etmemd_scan_exit();
}

static void test_get_vmas(void)
{
    test_get_vmas_invalid();
    test_get_vmas_valid();
}

static void test_get_page_refs_invalid(void)
{
    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    const char *pid = "1";
    char *vmflags_array[10] = {"rd"};
    int vmflag_num = 1;
    int is_anon_only = false;
    struct vmas *vmas = NULL;
    struct page_refs *page_refs = NULL;
    int flags = SCAN_AS_HUGE | SCAN_IGN_HOST;

    // free null pointer
    etmemd_free_page_refs(page_refs);

    // vmas is NULL
    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, pid, &page_refs, flags), -1);

    // vmas address range invalid
    vmas = (struct vmas *)calloc(1, sizeof(struct vmas));
    CU_ASSERT_PTR_NOT_NULL(vmas);
    vmas->vma_cnt = 1;

    struct vma *vma = (struct vma *)calloc(1, sizeof(struct vma));
    CU_ASSERT_PTR_NOT_NULL(vma);
    vma->start = 0x0ff;
    vma->end = 0x000;
    vma->next = NULL;
    vmas->vma_list = vma;

    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, pid, &page_refs, flags), -1);
    etmemd_free_vmas(vmas);

    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NOT_NULL(vmas);
    check_vmas(vmas);

    // pid not exist
    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, "0", &page_refs, flags), -1);
    CU_ASSERT_PTR_NULL(page_refs);

    // pid is NULL
    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, NULL, &page_refs, flags), -1);
    CU_ASSERT_PTR_NULL(page_refs);

    // pid contains invalid chars
    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, "--", &page_refs, flags), -1);
    CU_ASSERT_PTR_NULL(page_refs);

    etmemd_free_page_refs(page_refs);
    etmemd_free_vmas(vmas);
    etmemd_scan_exit();
}

static void test_get_page_refs_valid()
{
    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    const char *pid = "1";
    char *vmflags_array[10] = {"wr"};
    int vmflag_num = 1;
    int is_anon_only = false;
    struct vmas *vmas = NULL;
    struct page_refs *page_refs = NULL;
    int flags = SCAN_AS_HUGE | SCAN_IGN_HOST;

    vmas = etmemd_get_vmas(pid, vmflags_array, vmflag_num, is_anon_only);
    CU_ASSERT_PTR_NOT_NULL(vmas);
    check_vmas(vmas);

    CU_ASSERT_EQUAL(etmemd_get_page_refs(vmas, pid, &page_refs, flags), 0);
    CU_ASSERT_PTR_NOT_NULL(page_refs);

    unsigned long use_rss;

    CU_ASSERT_EQUAL(get_page_refs(vmas, pid, &page_refs, &use_rss, NULL), 0);
    CU_ASSERT_PTR_NOT_NULL(page_refs);
    CU_ASSERT_NOT_EQUAL(use_rss, 0);

    etmemd_free_page_refs(page_refs);
    etmemd_free_vmas(vmas);
    etmemd_scan_exit();
}

static void test_get_page_refs(void)
{
    test_get_page_refs_invalid();
    test_get_page_refs_valid();
}

static void test_scan_error(void)
{
    unsigned int pid_error = 1111111111;
    int loop = 1;
    int sleep = 1;
    struct task_pid *tpid = NULL;
    struct task *tk = NULL;

    tk = alloc_tk(loop, sleep);
    tpid = alloc_tkpid(pid_error, tk);

    CU_ASSERT_PTR_NULL(etmemd_do_scan(tpid, NULL));
    CU_ASSERT_PTR_NULL(etmemd_do_scan(tpid, tk));

    free(tk->eng->proj->scan_param);
    free(tk->eng->proj);
    free(tk->eng);
    free(tk);
    free(tpid);
}

static void test_etmem_scan_ok(void)
{
    unsigned int pid_ok = 1;
    int loop = 1;
    int sleep = 1;
    struct page_refs *page_refs = NULL;
    struct task_pid *tpid = NULL;
    struct task *tk = NULL;

    tk = alloc_tk(loop, sleep);
    tpid = alloc_tkpid(pid_ok, tk);

    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    page_refs = etmemd_do_scan(tpid, tk);
    CU_ASSERT_PTR_NOT_NULL(page_refs);
    free(tk->eng->proj->scan_param);
    free(tk->eng->proj);
    free(tk->eng);
    free(tk);
    free(tpid);
    clean_page_refs_unexpected(&page_refs);
    CU_ASSERT_PTR_NULL(page_refs);
    etmemd_scan_exit();
}

static void test_add_pg_to_mem_grade()
{
    const char *pid = "1";
    struct vmas *vma = NULL;
    struct page_refs *page_refs = NULL;
    struct page_refs *list = NULL;

    CU_ASSERT_EQUAL(etmemd_scan_init(), 0);

    vma = get_vmas(pid);
    CU_ASSERT_EQUAL(get_page_refs(vma, pid, &page_refs, NULL, NULL), 0);
    page_refs = add_page_refs_into_memory_grade(page_refs, &list);
    CU_ASSERT_PTR_NOT_NULL(page_refs);
    CU_ASSERT_PTR_NOT_NULL(list);

    free(list);
    etmemd_free_page_refs(page_refs);
    free_vmas(vma);
    etmemd_scan_exit();
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

    suite = CU_add_suite("etmem_scan_ops", NULL, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_get_vmas) == NULL ||
        CU_ADD_TEST(suite, test_get_page_refs) == NULL ||
        CU_ADD_TEST(suite, test_scan_error) == NULL ||
        CU_ADD_TEST(suite, test_etmem_scan_ok) == NULL ||
        CU_ADD_TEST(suite, test_add_pg_to_mem_grade) == NULL) {
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_scan.c");
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
