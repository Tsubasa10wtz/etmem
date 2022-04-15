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
 * Author: louhongxiang
 * Create: 2021-11-19
 * Description: This is a header file of the export data structure definition for page.
 ******************************************************************************/

#include <stdio.h>
#include <numa.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "etmemd_project.h"

#include "test_common.h"

#define FIRST_COLD_NODE_INDEX 2
#define FIRST_HOT_NODE_INDEX 0
#define SECOND_COLD_NODE_INDEX 6
#define SECOND_HOT_NODE_INDEX 4

GKeyFile *load_config(const char *file_name)
{
    GKeyFile *config = NULL;

    config = g_key_file_new();
    CU_ASSERT_PTR_NOT_NULL(config);
    CU_ASSERT_NOT_EQUAL(g_key_file_load_from_file(config, file_name, G_KEY_FILE_NONE, NULL), FALSE);
    return config;
}

void unload_config(GKeyFile *config)
{
    g_key_file_free(config);
}

void construct_proj_file(struct proj_test_param *param)
{
    FILE *file = NULL;

    file = fopen(param->file_name, "w+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, "[project]\n"), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SCAN_TYPE, "page"), -1);
    if (param->proj_name != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_NAME, param->proj_name), -1);
    }
    if (param->interval != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_INTERVAL, param->interval), -1);
    }
    if (param->loop != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_LOOP, param->loop), -1);
    }
    if (param->sleep != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SLEEP, param->sleep), -1);
    }
    if (param->sysmem_threshold != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SYSMEM_THRESHOLD,
                                    param->sysmem_threshold), -1);
    }
    if (param->swapcache_high_wmark != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SWAPCACHE_HIGH_WMARK,
                                    param->swapcache_high_wmark), -1);
    }
    if (param->swapcache_low_wmark != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SWAPCACHE_LOW_WMARK,
                                    param->swapcache_low_wmark), -1);
    }
    fclose(file);
}

GKeyFile *construct_proj_config(struct proj_test_param *param)
{
    construct_proj_file(param);

    return load_config(param->file_name);
}

void destroy_proj_config(GKeyFile *config)
{
    unload_config(config);
}

void init_proj_param(struct proj_test_param *param)
{
    param->sleep = "1";
    param->interval = "1";
    param->loop = "1";
    param->sysmem_threshold = NULL;
    param->swapcache_high_wmark = NULL;
    param->swapcache_low_wmark = NULL;
    param->file_name = TMP_PROJ_CONFIG;
    param->proj_name = DEFAULT_PROJ;
    param->expt = OPT_SUCCESS;
}

void do_add_proj_test(struct proj_test_param *param)
{
    GKeyFile *config = NULL;

    config = construct_proj_config(param);
    CU_ASSERT_EQUAL(etmemd_project_add(config), param->expt);
    destroy_proj_config(config);
}

void do_rm_proj_test(struct proj_test_param *param)
{
    GKeyFile *config = NULL;

    config = load_config(param->file_name);
    CU_ASSERT_EQUAL(etmemd_project_remove(config), param->expt);
    unload_config(config);
}

void construct_eng_file(struct eng_test_param *param)
{
    FILE *file = NULL;

    file = fopen(param->file_name, "w+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, "[engine]\n"), -1);
    if (param->name != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_NAME, param->name), -1);
    }
    if (param->proj != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_PROJ, param->proj), -1);
    }
    fclose(file);
}

GKeyFile *construct_eng_config(struct eng_test_param *param)
{
    construct_eng_file(param);

    return load_config(param->file_name);
}

void destroy_eng_config(GKeyFile *config)
{
    unload_config(config);
}

void init_task_param(struct task_test_param *param, const char *eng)
{
    param->name = DEFAULT_TASK;
    param->proj = DEFAULT_PROJ;
    param->eng = eng;
    param->type = "pid";
    param->value = "1";
    param->file_name = TMP_TASK_CONFIG;
}

void construct_task_file(struct task_test_param *param)
{
    FILE *file = NULL;

    file = fopen(param->file_name, "w+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, "[task]\n"), -1);
    if (param->name != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_NAME, param->name), -1);
    }
    if (param->proj != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_PROJ, param->proj), -1);
    }
    if (param->eng != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_ENG, param->eng), -1);
    }
    if (param->type != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_TYPE, param->type), -1);
    }
    if (param->value != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_VALUE, param->value), -1);
    }
    fclose(file);
}

void init_slide_task(struct slide_task_test_param *param)
{
    init_task_param(&param->task_param, "slide");
    param->max_threads = "1";
    param->T = "1";
    param->swap_flag = NULL;
    param->swap_threshold = NULL;
}

void add_slide_task(struct slide_task_test_param *param)
{
    FILE *file = NULL;
    const char *file_name = param->task_param.file_name;

    file = fopen(file_name, "a+");
    CU_ASSERT_PTR_NOT_NULL(file);
    if (param->max_threads != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_MAX_THREADS, param->max_threads), -1);
    }
    if (param->T != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_T, param->T), -1);
    }
    if (param->swap_flag != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SWAP_FLAG, param->swap_flag), -1);
    }
    if (param->swap_threshold != NULL) {
        CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_SWAP_THRESHOLD, param->swap_threshold), -1);
    }
    fclose(file);
}

GKeyFile *construct_slide_task_config(struct slide_task_test_param *param)
{
    struct task_test_param *task_param = &param->task_param;

    construct_task_file(task_param);
    add_slide_task(param);

    return load_config(task_param->file_name);
}

void destroy_slide_task_config(GKeyFile *config)
{
    unload_config(config);
}

void init_slide_eng(struct eng_test_param *param)
{
    param->name = "slide";
    param->proj = DEFAULT_PROJ;
    param->file_name = TMP_ENG_CONFIG;
}

static char *get_node_pair(void)
{
    int node_num = numa_num_configured_nodes();

    switch (node_num) {
        case ONE_NODE_PAIR:
            return "0,1";
        case TWO_NODE_PAIR:
            return "0,2;1,3";
        case THREE_NODE_PAIR:
            return "0,3;1,4;2,5";
        default:
            return NULL;
    }
}

int get_first_cold_node(struct cslide_eng_test_param *param)
{
    return param->node_pair[FIRST_COLD_NODE_INDEX] - '0';
}

int get_first_hot_node(struct cslide_eng_test_param *param)
{
    return param->node_pair[FIRST_HOT_NODE_INDEX] - '0';
}

int get_second_cold_node(struct cslide_eng_test_param *param)
{
    return param->node_pair[SECOND_COLD_NODE_INDEX] - '0';
}

int get_second_hot_node(struct cslide_eng_test_param *param)
{
    return param->node_pair[SECOND_HOT_NODE_INDEX] - '0';
}

void init_cslide_eng(struct cslide_eng_test_param *param)
{
    param->eng_param.name = "cslide";
    param->eng_param.proj = DEFAULT_PROJ;
    param->eng_param.file_name = TMP_ENG_CONFIG;
    param->node_pair = get_node_pair();
    param->hot_threshold = "2";
    param->node_mig_quota = "1024";
    param->node_hot_reserve = "1024";
}

void add_cslide_eng(struct cslide_eng_test_param *param)
{
    FILE *file = NULL;
    const char *file_name = param->eng_param.file_name;

    file = fopen(file_name, "a+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_NODE_PAIR, param->node_pair), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_THRESH, param->hot_threshold), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_QUOTA, param->node_mig_quota), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_RESV, param->node_hot_reserve), -1);
    fclose(file);
}

GKeyFile *construct_cslide_eng_config(struct cslide_eng_test_param *param)
{
    struct eng_test_param *eng_param = &param->eng_param;

    construct_eng_file(eng_param);
    add_cslide_eng(param);

    return load_config(eng_param->file_name);
}

void destroy_cslide_eng_config(GKeyFile *config)
{
    unload_config(config);
}

void init_cslide_task(struct cslide_task_test_param *param)
{
    init_task_param(&param->task_param, "cslide");
    param->vm_flags = "ht";
    param->anon_only = "no";
    param->ign_host = "yes";
}

void add_cslide_task(struct cslide_task_test_param *param)
{
    FILE *file = NULL;
    const char *file_name = param->task_param.file_name;

    file = fopen(file_name, "a+");
    CU_ASSERT_PTR_NOT_NULL(file);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_VM_FLAGS, param->vm_flags), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_ANON_ONLY, param->anon_only), -1);
    CU_ASSERT_NOT_EQUAL(fprintf(file, CONFIG_IGN_HOST, param->ign_host), -1);
    fclose(file);
}

GKeyFile *construct_cslide_task_config(struct cslide_task_test_param *param)
{
    struct task_test_param *task_param = &param->task_param;

    construct_task_file(task_param);
    add_cslide_task(param);

    return load_config(task_param->file_name);
}

void destroy_cslide_task_config(GKeyFile *config)
{
    unload_config(config);
}
