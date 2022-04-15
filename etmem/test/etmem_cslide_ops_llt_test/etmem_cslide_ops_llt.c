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
 * Create: 2021-12-10
 * Description: This is a source file of the unit test for cslide functions in etmem.
 ******************************************************************************/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <numa.h>

#include "test_common.h"
#include "etmemd_project.h"
#include "securec.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_cslide.c"

#define CHARNUM                     32

#define FLAGS                       (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#define PROTECTION                  (PROT_READ | PROT_WRITE)
#define TEST_HUGEPAGE_NUM           (10)
#define HUGE_SHIFT                  21
#define BYTE_TO_MB_SHIFT            20
#define HUGE_TO_MB                  2

#define DEFAULT_MIG_QUOTA           1024
#define MAX_PATH                    1024
#define MAX_CMD                     1024

#define TEST_LOOP                   3
#define TEST_LIMIT_HUGE             1
#define TEST_LIMIT_COLD_HUGE        4
#define TEST_LIMIT_HOT_FREE_HUGE    4
#define TEST_LIMIT_COLD_FREE_HUGE   1

#define NO_EXIST_PID                444444
static struct proj_test_param       g_proj_param;
static struct cslide_eng_test_param g_default_cslide_eng;

static char *g_vmflags_array[] = {
    "ht",
};

static void add_default_proj(void)
{
    init_proj_param(&g_proj_param);
    do_add_proj_test(&g_proj_param);
}

static void rm_default_proj(void)
{
    do_rm_proj_test(&g_proj_param);
}

/* add with a correct config */
void test_etmem_add_cslide_0001(void)
{
    struct cslide_eng_test_param cslide_param;
    GKeyFile *config = NULL;

    add_default_proj();
    init_cslide_eng(&cslide_param);

    config = construct_cslide_eng_config(&cslide_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);
    rm_default_proj();
}

void test_etmem_invalid_node_pair(struct cslide_eng_test_param *param)
{
    GKeyFile *config = NULL;

    /* lack of node */
    param->node_pair = "0,2";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* node out of range */
    param->node_pair = "0,2;1,3;4,5";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* hot node invalid */
    param->node_pair = "0x&,2";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* cold node invalid */
    param->node_pair = "0,x&2";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* empty */
    param->node_pair = "";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* node remap */
    param->node_pair = "0,2;0,3";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* cold node null */
    param->node_pair = "0,";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* hot node null */
    param->node_pair = ",;";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* reset with correcct node_pair */
    param->node_pair = "0,2;1,3";
}

void test_etmem_invalid_hot_threshold(struct cslide_eng_test_param *param)
{
    GKeyFile *config = NULL;
    char test_thresh[CHARNUM];

    param->hot_threshold = "";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->hot_threshold = "1abc";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->hot_threshold = "-1";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->hot_threshold = "0";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    if (snprintf_s(test_thresh, CHARNUM, CHARNUM - 1, "%d", INT_MAX) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->hot_threshold = test_thresh;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);


    if (snprintf_s(test_thresh, CHARNUM, CHARNUM - 1, "%lld", (long long)INT_MAX + 1) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->hot_threshold = test_thresh;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* reset with correct hot_threshold */
    param->hot_threshold = "1";
}

void test_etmem_invalid_mig_quota(struct cslide_eng_test_param *param)
{
    GKeyFile *config = NULL;
    char test_quota[CHARNUM];

    param->node_mig_quota = "";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->node_mig_quota = "-1";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->node_mig_quota = "0";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    if (snprintf_s(test_quota, CHARNUM, CHARNUM - 1, "%d", INT_MAX) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->node_mig_quota = test_quota;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    if (snprintf_s(test_quota, CHARNUM, CHARNUM - 1, "%lld", (long long)INT_MAX + 1) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->node_mig_quota = test_quota;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* reset with correct node_mig_quota */
    param->node_mig_quota = "1024";
}

void test_etmem_invalid_hot_reserve(struct cslide_eng_test_param *param)
{
    GKeyFile *config = NULL;
    char test_resv[CHARNUM];

    param->node_hot_reserve = "";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->node_hot_reserve = "1abc";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->node_hot_reserve = "-1";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    param->node_hot_reserve = "0";
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    if (snprintf_s(test_resv, CHARNUM, CHARNUM - 1, "%d", INT_MAX) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->node_hot_reserve = test_resv;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    if (snprintf_s(test_resv, CHARNUM, CHARNUM - 1, "%lld", (long long)INT_MAX + 1) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    param->node_hot_reserve = test_resv;
    config = construct_cslide_eng_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);

    /* reset with correct node_hot_reserve */
    param->node_hot_reserve = "1024";
}

/* add with a wrong config */
void test_etmem_add_cslide_0002(void)
{
    struct cslide_eng_test_param cslide_param;
    GKeyFile *config = NULL;

    add_default_proj();
    init_cslide_eng(&cslide_param);

    test_etmem_invalid_node_pair(&cslide_param);
    test_etmem_invalid_hot_threshold(&cslide_param);
    test_etmem_invalid_mig_quota(&cslide_param);
    test_etmem_invalid_hot_reserve(&cslide_param);
    rm_default_proj();
}

/* delete cslide with correct config */
void test_etmem_del_cslide_0001(void)
{
    struct cslide_eng_test_param cslide_param;
    GKeyFile *config = NULL;

    add_default_proj();
    init_cslide_eng(&cslide_param);

    config = construct_cslide_eng_config(&cslide_param);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);
    rm_default_proj();
}

/* delete cslide with wrong config */
void test_etmem_del_cslide_0002(void)
{
    struct cslide_eng_test_param cslide_param;
    GKeyFile *config = NULL;

    add_default_proj();
    init_cslide_eng(&cslide_param);

    /* delete not existed cslide */
    config = construct_cslide_eng_config(&cslide_param);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_ENG_NOEXIST);
    destroy_cslide_eng_config(config);
    rm_default_proj();
}

static void add_default_cslide_eng(void)
{
    GKeyFile *config = NULL;

    init_cslide_eng(&g_default_cslide_eng);

    config = construct_cslide_eng_config(&g_default_cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_add_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);
}

static void rm_default_cslide_eng(void)
{
    GKeyFile *config = NULL;

    config = construct_cslide_eng_config(&g_default_cslide_eng);
    CU_ASSERT_EQUAL(etmemd_project_remove_engine(config), OPT_SUCCESS);
    destroy_cslide_eng_config(config);
}

static void fill_random(void *addr, size_t len)
{
    size_t i;
    for (i = 0; i < len / sizeof(int); i++) {
        ((int *)addr)[i] = rand();
    }
}

static int check_addr_node(void *addr, pid_t pid, int node, int num)
{
    unsigned int i;
    void *pages[TEST_HUGEPAGE_NUM];
    int status[TEST_HUGEPAGE_NUM];

    for (i = 0; i < num; i++) {
        pages[i] = (void *)((uint64_t)addr + (1 << HUGE_SHIFT) * i);
    }

    if (move_pages(pid, TEST_HUGEPAGE_NUM, pages, NULL, status, MPOL_MF_MOVE_ALL) < 0) {
        perror("move_page:");
        return -1;
    }

    for (i = 0; i < num; i++) {
        if (status[i] != node) {
            printf("hugepage not in expect node\n");
            return -1;
        }
    }

    return 0;
}

static int page_refs_num(struct page_refs *page_refs)
{
    int ret = 0;

    while (page_refs != NULL) {
        ret++;
        page_refs = page_refs->next;
    }

    printf("page_refs num %d\n", ret);
    return ret;
}

static void *get_hugepage(unsigned int num, int node, pid_t pid)
{
    void *addr = NULL;
    unsigned long nodemask;
    size_t len = num << HUGE_SHIFT;
    int ret;
    void *pages[TEST_HUGEPAGE_NUM];
    int status[TEST_HUGEPAGE_NUM];
    int nodes[TEST_HUGEPAGE_NUM];
    int i;

    addr = mmap(NULL, len, PROTECTION, FLAGS, -1, 0);
    if (addr == NULL) {
        return addr;
    }
    fill_random(addr, len);

    for (i = 0; i < num; i++) {
        pages[i] = (void *)((uint64_t)addr + (1 << HUGE_SHIFT) * i);
        nodes[i] = node;
    }

    if (move_pages(pid, num, pages, nodes, status, MPOL_MF_MOVE_ALL) != 0) {
        perror("move_pages fail: ");
        return NULL;
    }

    if (check_addr_node(addr, pid, node, num) != 0) {
        printf("init place hugepage fail\n");
        return NULL;
    }

    return addr;
}

static int free_hugepage(void *addr, unsigned int num)
{
    return munmap(addr, num << HUGE_SHIFT);
}

static int init_hugepage(void)
{
    int node_num = numa_num_configured_nodes();
    char cmd[MAX_CMD];
    int i;

    for (i = 0; i < node_num; i++) {
        if (snprintf_s(cmd, MAX_CMD, MAX_CMD - 1, 
                       "echo 10000 > /sys/devices/system/node/node%d/hugepages/hugepages-2048kB/nr_hugepages", i) <= 0) {
            printf("snprintf_s fail\n");
            return -1;
        }
        if (system(cmd) != 0) {
            return -1;
        }
    }

    return 0;
}

void test_etmem_add_task_0001(void)
{
    struct cslide_task_test_param cslide_task;
    char pid_char[CHARNUM];
    GKeyFile *config = NULL;
    pid_t pid;
    void *addr = NULL;
    int cold_node, hot_node;

    add_default_proj();
    add_default_cslide_eng();

    init_cslide_task(&cslide_task);

    config = construct_cslide_task_config(&cslide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    pid = getpid();
    if (snprintf_s(pid_char, CHARNUM, CHARNUM - 1, "%d", pid) <= 0) {
        CU_FAIL("snprintf_s fail");
    }
    cslide_task.task_param.value = pid_char;
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    addr = get_hugepage(TEST_HUGEPAGE_NUM, cold_node, pid);
    CU_ASSERT_PTR_NOT_NULL(addr);

    config = construct_cslide_task_config(&cslide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_migrate_start(DEFAULT_PROJ), OPT_SUCCESS);
    sleep(10);
    CU_ASSERT_EQUAL(etmemd_migrate_stop(DEFAULT_PROJ), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(check_addr_node(addr, pid, 0, TEST_HUGEPAGE_NUM), 0);

    hot_node = get_first_hot_node(&g_default_cslide_eng);
    CU_ASSERT_EQUAL(free_hugepage(addr, TEST_HUGEPAGE_NUM), hot_node);
    rm_default_cslide_eng();
    rm_default_proj();
}

/* Cslide only support "ht" now. Any other value is invalid */
static void test_etmem_invalid_vm_flags(struct cslide_task_test_param *param)
{
    GKeyFile *config = NULL;

    param->vm_flags = "hts";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->vm_flags = "";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->vm_flags = "rd";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->vm_flags = "ht rd";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    /* reset to normal vm flag */
    param->vm_flags = "ht";
}

static void test_etmem_invalid_anon_only(struct cslide_task_test_param *param)
{
    GKeyFile *config = NULL;

    param->anon_only = "wrong";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->anon_only = "";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->anon_only = "yes";
    config = construct_cslide_task_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);
}

static void test_etmem_invalid_ign_host(struct cslide_task_test_param *param)
{
    GKeyFile *config = NULL;

    param->ign_host = "wrong";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->ign_host = "";
    config = construct_cslide_task_config(param);
    CU_ASSERT_NOT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    param->ign_host = "yes";
    config = construct_cslide_task_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);
}

void test_etmem_add_task_0002(void)
{
    struct cslide_task_test_param cslide_task;

    add_default_proj();
    add_default_cslide_eng();

    init_cslide_task(&cslide_task);

    test_etmem_invalid_vm_flags(&cslide_task);
    test_etmem_invalid_anon_only(&cslide_task);
    test_etmem_invalid_ign_host(&cslide_task);

    rm_default_cslide_eng();
    rm_default_proj();
}

void test_etmem_del_task_0001(void)
{
    struct cslide_task_test_param cslide_task;
    GKeyFile *config = NULL;

    add_default_proj();
    add_default_cslide_eng();

    init_cslide_task(&cslide_task);

    config = construct_cslide_task_config(&cslide_task);
    CU_ASSERT_EQUAL(etmemd_project_add_task(config), OPT_SUCCESS);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    rm_default_cslide_eng();
    rm_default_proj();
}

void test_etmem_del_task_0002(void)
{
    struct cslide_task_test_param cslide_task;
    GKeyFile *config = NULL;

    add_default_proj();
    add_default_cslide_eng();

    init_cslide_task(&cslide_task);

    /* delete not existed task */
    config = construct_cslide_task_config(&cslide_task);
    CU_ASSERT_EQUAL(etmemd_project_remove_task(config), OPT_TASK_NOEXIST);
    destroy_cslide_task_config(config);

    cslide_task.task_param.name = "";
    config = construct_cslide_task_config(&cslide_task);
    CU_ASSERT_NOT_EQUAL(etmemd_project_remove_task(config), OPT_SUCCESS);
    destroy_cslide_task_config(config);

    rm_default_cslide_eng();
    rm_default_proj();
}

static void init_cslide_params(struct cslide_pid_params **pid_params,
                               struct cslide_task_params *task_params,
                               struct cslide_eng_params *eng_params)
{
    int hot_node, cold_node;

    CU_ASSERT_EQUAL(memset_s(task_params, sizeof(*task_params), 0, sizeof(*task_params)), EOK);
    CU_ASSERT_EQUAL(memset_s(eng_params, sizeof(*eng_params), 0, sizeof(*eng_params)), EOK);

    /* init eng_params */
    CU_ASSERT_EQUAL(init_cslide_eng_params(eng_params), 0);
    eng_params->loop = TEST_LOOP;
    eng_params->interval = 1;
    eng_params->sleep = 1;
    eng_params->hot_threshold = 1;
    eng_params->mig_quota = DEFAULT_MIG_QUOTA;
    eng_params->hot_reserve = 0;
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    add_node_pair(&eng_params->node_map, cold_node, hot_node);
    if (numa_num_configured_nodes() > ONE_NODE_PAIR) {
        hot_node = get_second_hot_node(&g_default_cslide_eng);
        cold_node = get_second_cold_node(&g_default_cslide_eng);
        add_node_pair(&eng_params->node_map, cold_node, hot_node);
    }

    /* init task_params with scan info */
    task_params->anon_only = false;
    task_params->vmflags_num = 1;
    task_params->vmflags_array = g_vmflags_array;
    task_params->scan_flags |= SCAN_AS_HUGE | SCAN_IGN_HOST;

    *pid_params = alloc_pid_params(eng_params);
    CU_ASSERT_PTR_NOT_NULL(*pid_params);
    /* init pid_params */
    (*pid_params)->pid = getpid();
    (*pid_params)->eng_params = eng_params;
    (*pid_params)->task_params = task_params;
    factory_add_pid_params(&eng_params->factory, *pid_params);
    factory_update_pid_params(&eng_params->factory);
}

static void set_page_count(struct cslide_pid_params *pid_params, int count,
                           void *start_addr, void *end_addr)
{
    int i;
    struct page_refs *page_refs = NULL;
    int page_count = 0;

    for (i = 0; i < pid_params->vmas->vma_cnt; i++) {
        page_refs = pid_params->vma_pf[i].page_refs;
        for (; page_refs != NULL; page_refs = page_refs->next) {
            if ((uint64_t)page_refs->addr < (uint64_t)end_addr &&
                (uint64_t)page_refs->addr >= (uint64_t)start_addr) {
                page_refs->count = count;
                page_count++;
            }
        }
    }

    printf("page_count: %d\n", page_count);
}

/* test move hot pages */
static void test_etmem_cslide_policy_001(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *hot_addr = NULL;
    void *cold_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    hot_addr = get_hugepage(TEST_HUGEPAGE_NUM, cold_node, pid);
    get_sys_mem(&eng_params.mem);

    /* unlimited hot move */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_HUGEPAGE_NUM);
    cslide_clean_params(&eng_params);

    /* hot move is not limited by hot_reserve */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.hot_reserve = eng_params.mem.node_mem[hot_node].huge_total >> BYTE_TO_MB_SHIFT;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_HUGEPAGE_NUM);
    cslide_clean_params(&eng_params);
    eng_params.hot_reserve = 0;

    /* hot move is limited by mig_quota with 1 */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT));
    set_page_count(pid_params, TEST_LOOP - 1, hot_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT), 
                   hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.mig_quota = TEST_LIMIT_HUGE * HUGE_TO_MB;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_LIMIT_HUGE);
    cslide_clean_params(&eng_params);
    eng_params.mig_quota = DEFAULT_MIG_QUOTA;

    /* limited by "free space" + "cold page space" in hot node */
    cold_addr = get_hugepage(TEST_LIMIT_COLD_HUGE, hot_node, pid);
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_LIMIT_COLD_HUGE << HUGE_SHIFT));
    eng_params.mem.node_mem[hot_node].huge_free = TEST_LIMIT_HOT_FREE_HUGE << HUGE_SHIFT;
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));

    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), 
                    TEST_LIMIT_COLD_HUGE + TEST_LIMIT_HOT_FREE_HUGE);
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_LIMIT_COLD_HUGE);
    cslide_clean_params(&eng_params);

    /* limited by "hot node free space" + "cold node free space" */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_LIMIT_COLD_HUGE << HUGE_SHIFT));
    eng_params.mem.node_mem[hot_node].huge_free = TEST_LIMIT_HOT_FREE_HUGE << HUGE_SHIFT;
    eng_params.mem.node_mem[cold_node].huge_free = TEST_LIMIT_COLD_FREE_HUGE << HUGE_SHIFT;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), 
                    TEST_LIMIT_COLD_FREE_HUGE + TEST_LIMIT_HOT_FREE_HUGE);
    CU_ASSERT_EQUAL(free_hugepage(cold_addr, TEST_LIMIT_COLD_HUGE), 0);

    /* policy no exist task */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    pid_params->pid = NO_EXIST_PID;
    CU_ASSERT_NOT_EQUAL(cslide_policy(&eng_params), 0);
    cslide_clean_params(&eng_params);
    pid_params->pid = pid;

    /* clean up */
    CU_ASSERT_EQUAL(free_hugepage(hot_addr, TEST_HUGEPAGE_NUM), 0);
}

/* test prefetch cold pages */
static void test_etmem_cslide_policy_002(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *prefetch_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    prefetch_addr = get_hugepage(TEST_HUGEPAGE_NUM, cold_node, pid);
    get_sys_mem(&eng_params.mem);

    /* unlimited prefetch */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, prefetch_addr, prefetch_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_HUGEPAGE_NUM);
    cslide_clean_params(&eng_params);

    /* prefetch is limited by hot_reserve */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, prefetch_addr, prefetch_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.hot_reserve = (eng_params.mem.node_mem[hot_node].huge_free >> BYTE_TO_MB_SHIFT) - HUGE_TO_MB;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), 1);
    cslide_clean_params(&eng_params);
    eng_params.hot_reserve = 0;

    /* prefetch is limited by mig_quota with 1 */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, prefetch_addr, prefetch_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT));
    set_page_count(pid_params, 1, prefetch_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT), 
                   prefetch_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.mig_quota = TEST_LIMIT_HUGE * HUGE_TO_MB;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_LIMIT_HUGE);
    cslide_clean_params(&eng_params);
    eng_params.mig_quota = DEFAULT_MIG_QUOTA;

    /* clean up */
    CU_ASSERT_EQUAL(free_hugepage(prefetch_addr, TEST_HUGEPAGE_NUM), 0);
}

static void test_etmem_cslide_policy_003(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *cold_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    cold_addr = get_hugepage(TEST_HUGEPAGE_NUM, hot_node, pid);
    get_sys_mem(&eng_params.mem);
    eng_params.hot_reserve = eng_params.mem.node_mem[hot_node].huge_total >> BYTE_TO_MB_SHIFT;

    /* unlimited cold move */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_HUGEPAGE_NUM);
    cslide_clean_params(&eng_params);

    /* cold move is limited by mig_quota with 1 */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT));
    set_page_count(pid_params, 1, cold_addr + ((TEST_HUGEPAGE_NUM / 2) << HUGE_SHIFT), 
                   cold_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.mig_quota = TEST_LIMIT_HUGE * HUGE_TO_MB;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_LIMIT_HUGE);
    cslide_clean_params(&eng_params);
    eng_params.mig_quota = DEFAULT_MIG_QUOTA;

    /* cold move is limited by free space in cold node */
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    eng_params.mem.node_mem[cold_node].huge_free = TEST_LIMIT_COLD_HUGE << HUGE_SHIFT;
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_LIMIT_COLD_HUGE);
    cslide_clean_params(&eng_params);

    /* clean up */
    CU_ASSERT_EQUAL(free_hugepage(cold_addr, TEST_HUGEPAGE_NUM), 0);
}

static void do_test_cslide_query(void)
{
    char cmd[MAX_CMD];

    init_cslide_eng(&g_default_cslide_eng);
    if (snprintf_s(cmd, MAX_CMD, MAX_CMD - 1, "sh ../etmem_cslide_ops_llt_test/cslide_showpage_test.sh \"%s\"",
                   g_default_cslide_eng.node_pair) <= 0) {
        CU_FAIL("snprintf_s fail");
        return;
    }
    printf("run %s\n", cmd);
    CU_ASSERT_EQUAL(system(cmd), 0);
}

/* test query task page info */
static void test_etmem_cslide_query_001(void)
{
    do_test_cslide_query();
}

/* test query host page info */
static void test_etmem_cslide_query_002(void)
{
    do_test_cslide_query();
}

static void test_etmem_cslide_mig_001(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *hot_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    get_sys_mem(&eng_params.mem);

    /* move hot pages normally */
    hot_addr = get_hugepage(TEST_HUGEPAGE_NUM, cold_node, pid);
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_HUGEPAGE_NUM);
    CU_ASSERT_EQUAL(cslide_do_migrate(&eng_params), 0);
    CU_ASSERT_EQUAL(check_addr_node(hot_addr, pid, hot_node, TEST_HUGEPAGE_NUM), 0);
    cslide_clean_params(&eng_params);
    CU_ASSERT_EQUAL(free_hugepage(hot_addr, TEST_HUGEPAGE_NUM), 0);
}

static void test_etmem_cslide_mig_002(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *hot_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    get_sys_mem(&eng_params.mem);

    /* move hot pages with no exist task */
    hot_addr = get_hugepage(TEST_HUGEPAGE_NUM, cold_node, pid);
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, TEST_LOOP, hot_addr, hot_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->hot_pages), TEST_HUGEPAGE_NUM);
    pid_params->pid = NO_EXIST_PID;
    CU_ASSERT_NOT_EQUAL(cslide_do_migrate(&eng_params), 0);
    cslide_clean_params(&eng_params);
    CU_ASSERT_EQUAL(free_hugepage(hot_addr, TEST_HUGEPAGE_NUM), 0);
    pid_params->pid = pid;
}

static void test_etmem_cslide_mig_003(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *cold_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    get_sys_mem(&eng_params.mem);
    eng_params.hot_reserve = eng_params.mem.node_mem[hot_node].huge_total >> BYTE_TO_MB_SHIFT;

    /* move cold pages normally */
    cold_addr = get_hugepage(TEST_HUGEPAGE_NUM, hot_node, pid);
    get_sys_mem(&eng_params.mem);
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_HUGEPAGE_NUM);
    CU_ASSERT_EQUAL(cslide_do_migrate(&eng_params), 0);
    CU_ASSERT_EQUAL(check_addr_node(cold_addr, pid, cold_node, TEST_HUGEPAGE_NUM), 0);
    cslide_clean_params(&eng_params);
    CU_ASSERT_EQUAL(free_hugepage(cold_addr, TEST_HUGEPAGE_NUM), 0);
}

static void test_etmem_cslide_mig_004(void)
{
    struct cslide_pid_params *pid_params = NULL;
    struct cslide_task_params task_params;
    struct cslide_eng_params eng_params;
    void *cold_addr = NULL;
    pid_t pid = getpid();
    struct memory_grade *memory_grade = NULL;
    int cold_node, hot_node;

    /* init */
    init_cslide_eng(&g_default_cslide_eng);
    init_cslide_params(&pid_params, &task_params, &eng_params);
    cold_node = get_first_cold_node(&g_default_cslide_eng);
    hot_node = get_first_hot_node(&g_default_cslide_eng);
    get_sys_mem(&eng_params.mem);
    eng_params.hot_reserve = eng_params.mem.node_mem[hot_node].huge_total >> BYTE_TO_MB_SHIFT;

    /* move cold pages with no exist task */
    cold_addr = get_hugepage(TEST_HUGEPAGE_NUM, hot_node, pid);
    get_sys_mem(&eng_params.mem);
    CU_ASSERT_EQUAL(cslide_do_scan(&eng_params), 0);
    set_page_count(pid_params, 0, cold_addr, cold_addr + (TEST_HUGEPAGE_NUM << HUGE_SHIFT));
    CU_ASSERT_EQUAL(cslide_policy(&eng_params), 0);
    memory_grade = &pid_params->memory_grade[0];
    CU_ASSERT_EQUAL(page_refs_num(memory_grade->cold_pages), TEST_HUGEPAGE_NUM);
    pid_params->pid = NO_EXIST_PID;
    CU_ASSERT_NOT_EQUAL(cslide_do_migrate(&eng_params), 0);
    cslide_clean_params(&eng_params);
    CU_ASSERT_EQUAL(free_hugepage(cold_addr, TEST_HUGEPAGE_NUM), 0);
    pid_params->pid = pid;
}

static int init_env(void)
{
    if (init_g_page_size() != 0) {
        return -1;
    }
    if (init_hugepage() != 0) {
        return -1;
    }

    return 0;
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

    suite = CU_add_suite("etmem_cslide_ops", init_env, NULL);
    if (suite == NULL) {
        goto ERROR;
    }

    if (CU_ADD_TEST(suite, test_etmem_cslide_policy_001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_policy_002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_policy_003) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_query_001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_query_002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_mig_001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_mig_002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_mig_003) == NULL ||
        CU_ADD_TEST(suite, test_etmem_cslide_mig_004) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_cslide_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_cslide_0002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_cslide_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_cslide_0002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_task_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_add_task_0002) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_task_0001) == NULL ||
        CU_ADD_TEST(suite, test_etmem_del_task_0002) == NULL) {
            printf("CU_ADD_TEST fail. \n");
            goto ERROR;
    }

    switch (cunit_mode) {
        case CUNIT_SCREEN:
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
            break;
        case CUNIT_XMLFILE:
            CU_set_output_filename("etmemd_cslide.c");
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
