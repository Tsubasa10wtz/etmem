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

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <glib.h>

#include "etmemd_project.h"

#define CONFIG_NAME                         "name=%s\n"
#define CONFIG_SCAN_TYPE                    "scan_type=%s\n"
#define CONFIG_INTERVAL                     "interval=%s\n"
#define CONFIG_LOOP                         "loop=%s\n"
#define CONFIG_SLEEP                        "sleep=%s\n"
#define CONFIG_SYSMEM_THRESHOLD             "sysmem_threshold=%s\n"
#define CONFIG_SWAPCACHE_HIGH_WMARK         "swapcache_high_wmark=%s\n"
#define CONFIG_SWAPCACHE_LOW_WMARK          "swapcache_low_wmark=%s\n"
#define TMP_PROJ_CONFIG                     "proj_tmp.config"
#define DEFAULT_PROJ                        "default_proj"

#define CONFIG_PROJ                         "project=%s\n"
#define TMP_ENG_CONFIG                      "eng_tmp.config"

#define CONFIG_ENG                          "engine=%s\n"
#define CONFIG_TYPE                         "type=%s\n"
#define CONFIG_VALUE                        "value=%s\n"
#define TMP_TASK_CONFIG                     "task_tmp.config"
#define DEFAULT_TASK                        "default_task"

#define CONFIG_MAX_THREADS                  "max_threads=%s\n"
#define CONFIG_T                            "T=%s\n"
#define CONFIG_SWAP_FLAG                    "swap_flag=%s\n"
#define CONFIG_SWAP_THRESHOLD               "swap_threshold=%s\n"

#define CONFIG_NODE_PAIR                    "node_pair=%s\n"
#define CONFIG_THRESH                       "hot_threshold=%s\n"
#define CONFIG_QUOTA                        "node_mig_quota=%s\n"
#define CONFIG_RESV                         "node_hot_reserve=%s\n"

#define CONFIG_VM_FLAGS                     "vm_flags=%s\n"
#define CONFIG_ANON_ONLY                    "anon_only=%s\n"
#define CONFIG_IGN_HOST                     "ign_host=%s\n"

#define ONE_NODE_PAIR                       2
#define TWO_NODE_PAIR                       4
#define THREE_NODE_PAIR                     6


struct proj_test_param {
    const char *sleep;
    const char *interval;
    const char *loop;
    const char *sysmem_threshold;
    const char *swapcache_high_wmark;
    const char *swapcache_low_wmark;
    const char *proj_name;
    const char *file_name;
    enum opt_result expt;
};

struct eng_test_param {
    const char *name;
    const char *proj;
    const char *file_name;
};

struct task_test_param {
    const char *name;
    const char *proj;
    const char *eng;
    const char *type;
    const char *value;
    const char *file_name;
};

struct slide_task_test_param {
    struct task_test_param task_param;
    const char *max_threads;
    const char *T;
    const char *swap_flag;
    const char *swap_threshold;
};

struct cslide_eng_test_param {
    struct eng_test_param eng_param;
    const char *node_pair;
    const char *hot_threshold;
    const char *node_mig_quota;
    const char *node_hot_reserve;
};

struct cslide_task_test_param {
    struct task_test_param task_param;
    const char *vm_flags;
    const char *anon_only;
    const char *ign_host;
};

GKeyFile *load_config(const char *file_name);
void unload_config(GKeyFile *config);

void construct_proj_file(struct proj_test_param *param);
GKeyFile *construct_proj_config(struct proj_test_param *param);
void destroy_proj_config(GKeyFile *config);
void init_proj_param(struct proj_test_param *param);
void do_add_proj_test(struct proj_test_param *param);
void do_rm_proj_test(struct proj_test_param *param);

void construct_eng_file(struct eng_test_param *param);
GKeyFile *construct_eng_config(struct eng_test_param *param);
void destroy_eng_config(GKeyFile *config);

void init_task_param(struct task_test_param *param, const char *eng);
void construct_task_file(struct task_test_param *param);

void init_slide_task(struct slide_task_test_param *param);
void add_slide_task(struct slide_task_test_param *param);
GKeyFile *construct_slide_task_config(struct slide_task_test_param *param);
void destroy_slide_task_config(GKeyFile *config);
void init_slide_eng(struct eng_test_param *param);

void init_cslide_eng(struct cslide_eng_test_param *param);
void add_cslide_eng(struct cslide_eng_test_param *param);
GKeyFile *construct_cslide_eng_config(struct cslide_eng_test_param *param);
void destroy_cslide_eng_config(GKeyFile *config);

int get_first_cold_node(struct cslide_eng_test_param *param);
int get_first_hot_node(struct cslide_eng_test_param *param);
int get_second_cold_node(struct cslide_eng_test_param *param);
int get_second_hot_node(struct cslide_eng_test_param *param);

void init_cslide_task(struct cslide_task_test_param *param);
void add_cslide_task(struct cslide_task_test_param *param);
GKeyFile *construct_cslide_task_config(struct cslide_task_test_param *param);
void destroy_cslide_task_config(GKeyFile *config);
#endif
