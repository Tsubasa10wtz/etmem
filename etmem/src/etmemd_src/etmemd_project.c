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
 * Create: 2019-12-10
 * Description: Etmemd project API.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include "securec.h"
#include "etmemd_project.h"
#include "etmemd_engine.h"
#include "etmemd_damon.h"
#include "etmemd_common.h"
#include "etmemd_file.h"
#include "etmemd_log.h"

#define MAX_INTERVAL_VALUE              1200
#define MAX_SLEEP_VALUE                 1200
#define MAX_LOOP_VALUE                  120
#define MAX_SYSMEM_THRESHOLD_VALUE      100

#define MAX_OBJ_NAME_LEN                64
#define MIN_NR_MIN_VAL                  3

static SLIST_HEAD(project_list, project) g_projects = SLIST_HEAD_INITIALIZER(g_projects);

static struct project *get_proj_by_name(const char *name)
{
    struct project *proj = NULL;

    SLIST_FOREACH(proj, &g_projects, entry) {
        if (strcmp(proj->name, name) == 0) {
            return proj;
        }
    }

    return NULL;
}

static struct engine *get_eng_by_name(struct project *proj, const char *name)
{
    struct engine *eng = proj->engs;

    while (eng != NULL) {
        if (strcmp(eng->name, name) == 0) {
            return eng;
        }
        eng = eng->next;
    }

    return NULL;
}

static struct task *get_task_by_name(struct project *proj, struct engine *eng, const char *name)
{
    struct task *tk = eng->tasks;

    while (tk != NULL) {
        if (strcmp(tk->name, name) == 0) {
            return tk;
        }
        tk = tk->next;
    }

    return NULL;
}

static const char *get_obj_key(const char *obj_name, const char *group_name)
{
    if (strcmp(obj_name, group_name) == 0) {
        return "name";
    } else {
        return obj_name;
    }
}

static enum opt_result get_name_by_key(GKeyFile *config, const char *group_name, const char *key, char **name)
{
    if (g_key_file_has_key(config, group_name, key, NULL) == FALSE) {
        etmemd_log(ETMEMD_LOG_ERR, "key %s is not set in group %s\n", key, group_name);
        return OPT_INVAL;
    }

    *name = g_key_file_get_string(config, group_name, key, NULL);
    if (*name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get value of key %s from group %s fail\n", key, group_name);
        return OPT_INVAL;
    }
    if (strlen(*name) > MAX_OBJ_NAME_LEN) {
        etmemd_log(ETMEMD_LOG_ERR, "name len should not be greater than %d\n", MAX_OBJ_NAME_LEN);
        free(*name);
        return OPT_INVAL;
    }

    return OPT_SUCCESS;
}

static enum opt_result get_obj_name(GKeyFile *config, const char *group_name, const char *obj, char **name)
{
    const char *key = get_obj_key(obj, group_name);

    return get_name_by_key(config, group_name, key, name);
}

static enum opt_result project_of_group(GKeyFile *config, const char *group_name, struct project **proj)
{
    char *proj_name = NULL;
    enum opt_result ret;

    ret = get_obj_name(config, group_name, PROJ_GROUP, &proj_name);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    *proj = get_proj_by_name(proj_name);
    free(proj_name);
    return OPT_SUCCESS;
}

static enum opt_result engine_of_group(GKeyFile *config, const char *group_name,
        struct project *proj, struct engine **eng)
{
    char *eng_name = NULL;
    enum opt_result ret;

    ret = get_obj_name(config, group_name, ENG_GROUP, &eng_name);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    // real engine name is set with "eng_name" for thirdparty engine
    if (strcmp(eng_name, "thirdparty") == 0 && strcmp(group_name, ENG_GROUP) == 0) {
        free(eng_name);
        ret = get_name_by_key(config, ENG_GROUP, "eng_name", &eng_name);
        if (ret != OPT_SUCCESS) {
            return ret;
        }
    }

    *eng = get_eng_by_name(proj, eng_name);
    free(eng_name);
    return OPT_SUCCESS;
}

static enum opt_result task_of_group(GKeyFile *config, const char *group_name,
        struct project *proj, struct engine *eng, struct task **tk)
{
    char *task_name = NULL;
    enum opt_result ret;

    ret = get_obj_name(config, group_name, TASK_GROUP, &task_name);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    *tk = get_task_by_name(proj, eng, task_name);
    free(task_name);
    return OPT_SUCCESS;
}

static enum opt_result get_group_objs(GKeyFile *config, char *group_name,
        struct project **proj, struct engine **eng, struct task **tk)
{
    enum opt_result ret;

    /* get project */
    ret = project_of_group(config, group_name, proj);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    /* get engine */
    if (eng == NULL) {
        return OPT_SUCCESS;
    }
    if (*proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project of group %s not existed\n", group_name);
        return OPT_PRO_NOEXIST;
    }

    ret = engine_of_group(config, group_name, *proj, eng);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    /* get task */
    if (tk == NULL) {
        return OPT_SUCCESS;
    }
    if (*eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine of group %s not existed\n", group_name);
        return OPT_ENG_NOEXIST;
    }

    ret = task_of_group(config, group_name, *proj, *eng, tk);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    return OPT_SUCCESS;
}

static void project_insert_task(struct project *proj, struct engine *eng, struct task *task)
{
    task->next = eng->tasks;
    eng->tasks = task;
    task->eng = eng;
}

static void project_remove_task(struct engine *eng, struct task *task)
{
    struct task **iter = &eng->tasks;

    for (; *iter != NULL && *iter != task; iter = &(*iter)->next) {
        ;
    }

    if (*iter == NULL) {
        return;
    }

    *iter = (*iter)->next;
    task->next = NULL;
    task->eng = NULL;
}

static void do_remove_task(struct project *proj, struct engine *eng, struct task *tk)
{
    if (proj->start && eng->ops->stop_task != NULL) {
        eng->ops->stop_task(eng, tk);
    }
    if (eng->ops->clear_task_params != NULL) {
        eng->ops->clear_task_params(tk);
    }

    project_remove_task(eng, tk);
    etmemd_remove_task(tk);
}

enum opt_result etmemd_project_add_task(GKeyFile *config)
{
    struct task *tk = NULL;
    struct project *proj = NULL;
    struct engine *eng = NULL;
    enum opt_result ret;

    ret = get_group_objs(config, TASK_GROUP, &proj, &eng, &tk);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    if (tk != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task already exist\n");
        return OPT_TASK_EXISTED;
    }

    tk = etmemd_add_task(config);
    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "create new task fail\n");
        return OPT_INTER_ERR;
    }

    project_insert_task(proj, eng, tk);

    if (eng->ops->fill_task_params != NULL && eng->ops->fill_task_params(config, tk) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill task param fail\n");
        goto remove_task;
    }

    if (proj->start && eng->ops->start_task != NULL && eng->ops->start_task(eng, tk) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "start added task %s fail\n", tk->name);
        goto clear_task;
    }

    return OPT_SUCCESS;

clear_task:
    if (eng->ops->clear_task_params != NULL) {
        eng->ops->clear_task_params(tk);
    }

remove_task:
    project_remove_task(eng, tk);
    etmemd_remove_task(tk);

    return OPT_INTER_ERR;
}

enum opt_result etmemd_project_remove_task(GKeyFile *config)
{
    struct task *tk = NULL;
    struct project *proj = NULL;
    struct engine *eng = NULL;
    enum opt_result ret;

    ret = get_group_objs(config, TASK_GROUP, &proj, &eng, &tk);
    if (ret != OPT_SUCCESS) {
        return ret;
    }
    if (tk == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "task not exsit\n");
        return OPT_TASK_NOEXIST;
    }

    do_remove_task(proj, eng, tk);
    return OPT_SUCCESS;
}

static void project_insert_engine(struct project *proj, struct engine *eng)
{
    eng->next = proj->engs;
    proj->engs = eng;
    eng->proj = proj;
}

static void project_remove_engine(struct project *proj, struct engine *eng)
{
    struct engine **iter = &(proj->engs);

    for (; *iter != NULL && *iter != eng; iter = &(*iter)->next) {
        ;
    }

    if (*iter == NULL) {
        return;
    }

    *iter = (*iter)->next;
    eng->next = NULL;
    eng->proj = NULL;
}

static int project_start_engine(struct project *proj, struct engine *eng)
{
    struct task *tk = eng->tasks;
    int ret = 0;

    if (eng->ops->start_task == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s not support start\n", eng->name);
        return -1;
    }

    for (; tk != NULL; tk = tk->next) {
        if (eng->ops->start_task(eng, tk) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "start task %s fail.\n", tk->value);
            ret = -1;
        }
    }
    return ret;
}

static void project_stop_engine(struct project *proj, struct engine *eng)
{
    struct task *tk = eng->tasks;

    if (eng->ops->stop_task == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s not support stop\n", eng->name);
        return;
    }
    for (; tk != NULL; tk = tk->next) {
        eng->ops->stop_task(eng, tk);
    }
}

static void do_remove_engine_tasks(struct project *proj, struct engine *eng)
{
    while (eng->tasks != NULL) {
        do_remove_task(proj, eng, eng->tasks);
    }
}

static void do_remove_engine(struct project *proj, struct engine *eng)
{
    do_remove_engine_tasks(proj, eng);
    if (eng->ops->clear_eng_params != NULL) {
        eng->ops->clear_eng_params(eng);
    }

    project_remove_engine(proj, eng);
    etmemd_engine_remove(eng);
}

enum opt_result etmemd_project_add_engine(GKeyFile *config)
{
    struct engine *eng = NULL;
    struct project *proj = NULL;
    enum opt_result ret;

    ret = get_group_objs(config, ENG_GROUP, &proj, &eng, NULL);
    if (ret != OPT_SUCCESS) {
        return ret;
    }
    if (eng != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s exists\n", eng->name);
        return OPT_ENG_EXISTED;
    }

    eng = etmemd_engine_add(config);
    if (eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "create engine fail\n");
        return OPT_INTER_ERR;
    }

    project_insert_engine(proj, eng);
    if (eng->ops->fill_eng_params != NULL && eng->ops->fill_eng_params(config, eng) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill %s engine params fail\n", eng->name);
        project_remove_engine(proj, eng);
        etmemd_engine_remove(eng);
        return OPT_INTER_ERR;
    }

    return OPT_SUCCESS;
}

enum opt_result etmemd_project_remove_engine(GKeyFile *config)
{
    struct engine *eng = NULL;
    struct project *proj = NULL;
    enum opt_result ret;

    ret = get_group_objs(config, ENG_GROUP, &proj, &eng, NULL);
    if (ret != OPT_SUCCESS) {
        return ret;
    }
    if (eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "remove engine is not existed\n");
        return OPT_ENG_NOEXIST;
    }

    do_remove_engine(proj, eng);
    return OPT_SUCCESS;
}

static int fill_project_name(void *obj, void *val)
{
    struct project *proj = (struct project *)obj;
    char *name = (char *)val;
    proj->name = name;
    return 0;
}

static int fill_page_scan_loop(void *obj, void *val)
{
    struct page_scan *scan = (struct page_scan *)obj;
    int loop = parse_to_int(val);
    if (loop < 1 || loop > MAX_LOOP_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project loop value %d, it must be between 1 and %d.\n",
                   loop, MAX_LOOP_VALUE);
        return -1;
    }

    scan->loop = loop;
    return 0;
}

static int fill_page_scan_interval(void *obj, void *val)
{
    struct page_scan *scan = (struct page_scan *)obj;
    int interval = parse_to_int(val);
    if (interval < 1 || interval > MAX_INTERVAL_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project interval value %d, it must be between 1 and %d.\n",
                   interval, MAX_INTERVAL_VALUE);
        return -1;
    }

    scan->interval = interval;
    return 0;
}

static int fill_page_scan_sleep(void *obj, void *val)
{
    struct page_scan *scan = (struct page_scan *)obj;
    int sleep = parse_to_int(val);
    if (sleep < 1 || sleep > MAX_SLEEP_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project sleep value %d, it must be between 1 and %d.\n",
                   sleep, MAX_SLEEP_VALUE);
        return -1;
    }

    scan->sleep = sleep;
    return 0;
}

struct config_item g_page_scan_config_items[] = {
    {"loop", INT_VAL, fill_page_scan_loop, false},
    {"interval", INT_VAL, fill_page_scan_interval, false},
    {"sleep", INT_VAL, fill_page_scan_sleep, false},
};

static int fill_region_scan_samp_interval(void *obj, void *val)
{
    struct region_scan *scan = (struct region_scan *)obj;
    unsigned long samp_intvl = parse_to_ulong(val);

    scan->sample_interval = samp_intvl;
    return 0;
}

static int fill_region_scan_aggr_interval(void *obj, void *val)
{
    struct region_scan *scan = (struct region_scan *)obj;
    unsigned long aggr_intvl = parse_to_ulong(val);

    scan->aggr_interval = aggr_intvl;
    return 0;
}

static int fill_region_scan_updt_interval(void *obj, void *val)
{
    struct region_scan *scan = (struct region_scan *)obj;
    unsigned long updt_intvl = parse_to_ulong(val);

    scan->update_interval = updt_intvl;
    return 0;
}

static int fill_region_scan_min_nr(void *obj, void *val)
{
    struct region_scan *scan = (struct region_scan *)obj;
    unsigned long min_nr = parse_to_ulong(val);

    if (min_nr < MIN_NR_MIN_VAL) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid minimum nr value %d, it should not be smaller than %d.\n",
                   min_nr, MIN_NR_MIN_VAL);
        return -1;
    }

    scan->min_nr_regions = min_nr;
    return 0;
}

static int fill_region_scan_max_nr(void *obj, void *val)
{
    struct region_scan *scan = (struct region_scan *)obj;
    unsigned long max_nr = parse_to_ulong(val);

    if (max_nr < MIN_NR_MIN_VAL) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid maximum nr value %d, it should not be smaller than %d.\n",
                   max_nr, MIN_NR_MIN_VAL);
        return -1;
    }

    scan->max_nr_regions = max_nr;
    return 0;
}

struct config_item g_region_scan_config_items[] = {
    {"sample_interval", INT_VAL, fill_region_scan_samp_interval, false},
    {"aggr_interval", INT_VAL, fill_region_scan_aggr_interval, false},
    {"update_interval", INT_VAL, fill_region_scan_updt_interval, false},
    {"min_nr_regions", INT_VAL, fill_region_scan_min_nr, false},
    {"max_nr_regions", INT_VAL, fill_region_scan_max_nr, false},
};

int scan_fill_by_conf(GKeyFile *config, struct project *proj)
{
    struct region_scan *rg_scan = NULL;

    if (proj->type == PAGE_SCAN) {
        if (parse_file_config(config, PROJ_GROUP, g_page_scan_config_items,
                              ARRAY_SIZE(g_page_scan_config_items), proj->scan_param) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "parse page scan config fail.\n");
            return -1;
        }
    } else if (proj->type == REGION_SCAN) {
        if (parse_file_config(config, PROJ_GROUP, g_region_scan_config_items,
                              ARRAY_SIZE(g_region_scan_config_items), proj->scan_param) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "parse region scan config fail.\n");
            return -1;
        }
        rg_scan = (struct region_scan *)proj->scan_param;
        if (rg_scan->min_nr_regions >= rg_scan->max_nr_regions) {
            etmemd_log(ETMEMD_LOG_ERR, "min_nr_regions %d should be smaller than max_nr_regions %d.\n",
                       rg_scan->min_nr_regions, rg_scan->max_nr_regions);
            return -1;
        }
    }

    return 0;
}

static int fill_project_scan_type(void *obj, void *val)
{
    struct project *proj = (struct project *)obj;
    char *scan_type = (char *)val;

    if (strcmp(scan_type, "page") != 0 && strcmp(scan_type, "region") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid scan type %s, must be page or region\n",
                   scan_type);
        return -1;
    }

    if (strcmp(scan_type, "page") == 0) {
        struct page_scan *scan = NULL;

        scan = (struct page_scan *)calloc(1, sizeof(struct page_scan));
        if (scan == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "malloc fail\n");
            return -1;
        }
        proj->scan_param = (void *)scan;
        proj->type = PAGE_SCAN;
    } else {
        struct region_scan *scan = NULL;

        scan = (struct region_scan *)calloc(1, sizeof(struct region_scan));
        if (scan == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "malloc fail\n");
            return -1;
        }
        proj->scan_param = (void *)scan;
        proj->type = REGION_SCAN;
    }

    return 0;
}

/* fill the project parameter: sysmem_threshold
 * sysmem_threshold: [0, 100]. do not swap any memory out if system free memory is higher than sysmem_threshold */
static int fill_project_sysmem_threshold(void *obj, void *val)
{
    struct project *proj = (struct project *)obj;
    int sysmem_threshold = parse_to_int(val);

    if (sysmem_threshold < 0 || sysmem_threshold > MAX_SYSMEM_THRESHOLD_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invaild project sysmem_threshold value %d, it must between 0 and 100.\n",
                   sysmem_threshold);
        return -1;
    }

    proj->sysmem_threshold = sysmem_threshold;
    return 0;
}

/* fill the project parameter: swapcache_low_wmark
 * swapcache_low_wmark: (0, 100]. */
static int fill_project_swapcache_low_wmark(void *obj, void *val)
{
    struct project *proj = (struct project *)obj;
    int swapcache_low_wmark = parse_to_int(val);

    if (swapcache_low_wmark <= 0 || swapcache_low_wmark > MAX_SWAPCACHE_WMARK_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invaild project swapcache_low_wmark value %d, it must between 0 and 100.\n",
                   swapcache_low_wmark);
        return -1;
    }

    proj->swapcache_low_wmark = swapcache_low_wmark;
    return 0;
}

/* fill the project parameter: swapcache_high_wmark
 * swapcache_high_wmark: (0, 100]. */
static int fill_project_swapcache_high_wmark(void *obj, void *val)
{
    struct project *proj = (struct project *)obj;
    int swapcache_high_wmark = parse_to_int(val);

    if (swapcache_high_wmark <= 0 || swapcache_high_wmark > MAX_SWAPCACHE_WMARK_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invaild project swapcache_high_wmark value %d, it must between 0 and 100.\n",
                   swapcache_high_wmark);
        return -1;
    }

    proj->swapcache_high_wmark = swapcache_high_wmark;
    return 0;
}

static bool check_swapcache_wmark_valid(struct project *proj)
{
    if (proj->swapcache_high_wmark == -1 && proj->swapcache_low_wmark == -1) {
        return true;
    }

    if ((proj->swapcache_high_wmark > 0) &&
        (proj->swapcache_low_wmark > 0) &&
        (proj->swapcache_high_wmark > proj->swapcache_low_wmark)) {
        return true;
    }

    return false;
}

static struct config_item g_project_config_items[] = {
    {"name", STR_VAL, fill_project_name, false},
    {"scan_type", STR_VAL, fill_project_scan_type, false},
    {"sysmem_threshold", INT_VAL, fill_project_sysmem_threshold, true},
    {"swapcache_high_wmark", INT_VAL, fill_project_swapcache_high_wmark, true},
    {"swapcache_low_wmark", INT_VAL, fill_project_swapcache_low_wmark, true},
};

static void clear_project(struct project *proj)
{
    if (proj->name != NULL) {
        free(proj->name);
        proj->name = NULL;
    }

    if (proj->scan_param != NULL) {
        free(proj->scan_param);
        proj->scan_param = NULL;
    }
}

static int project_fill_by_conf(GKeyFile *config, struct project *proj)
{
    if (parse_file_config(config, PROJ_GROUP, g_project_config_items, ARRAY_SIZE(g_project_config_items), proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse project config fail\n");
        clear_project(proj);
        return -1;
    }

    if (!check_swapcache_wmark_valid(proj)) {
        etmemd_log(ETMEMD_LOG_ERR, "swapcache wmark is not valid, low wmark: %d, high wmark: %d",
                    proj->swapcache_low_wmark, proj->swapcache_high_wmark);
        clear_project(proj);
        return -1;
    }
    return 0;
}

static void do_remove_project(struct project *proj)
{
    SLIST_REMOVE(&g_projects, proj, project, entry);
    while (proj->engs != NULL) {
        do_remove_engine(proj, proj->engs);
    }
    clear_project(proj);
    free(proj);
}

enum opt_result etmemd_project_add(GKeyFile *config)
{
    struct project *proj = NULL;
    enum opt_result ret;

    ret = project_of_group(config, PROJ_GROUP, &proj);
    if (ret != OPT_SUCCESS) {
        return ret;
    }
    if (proj != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project %s existes\n", proj->name);
        return OPT_PRO_EXISTED;
    }

    proj = (struct project *)calloc(1, sizeof(struct project));
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc memory for project fail\n");
        return OPT_INTER_ERR;
    }

    proj->sysmem_threshold = -1;
    proj->swapcache_high_wmark = -1;
    proj->swapcache_low_wmark = -1;

    if (project_fill_by_conf(config, proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill project from configuration file fail\n");
        free(proj);
        proj = NULL;
        return OPT_INVAL;
    }
    if (scan_fill_by_conf(config, proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "fill scan parameter from configuration file fail\n");
        clear_project(proj);
        free(proj);
        proj = NULL;
        return OPT_INVAL;
    }

    SLIST_INSERT_HEAD(&g_projects, proj, entry);
    return OPT_SUCCESS;
}

enum opt_result etmemd_project_remove(GKeyFile *config)
{
    struct project *proj = NULL;
    enum opt_result ret;

    ret = get_group_objs(config, PROJ_GROUP, &proj, NULL, NULL);
    if (ret != OPT_SUCCESS) {
        return ret;
    }

    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "remove project not existed\n");
        return OPT_PRO_NOEXIST;
    }

    do_remove_project(proj);
    return OPT_SUCCESS;
}

static void etmemd_project_print(int fd, const struct project *proj)
{
    struct engine *eng = NULL;

    dprintf_all(fd, "project: %s\n", proj->name);
    dprintf_all(fd, "%-8s %-8s %-16s %-16s %-16s %-8s\n",
                "number",
                "type",
                "value",
                "name",
                "engine",
                "started");
    for (eng = proj->engs; eng != NULL; eng = eng->next) {
        etmemd_print_tasks(fd, eng->tasks, eng->name, proj->start);
    }
}

enum opt_result etmemd_project_show(const char *project_name, int sock_fd)
{
    struct project *proj = NULL;
    bool exists = false;

    SLIST_FOREACH(proj, &g_projects, entry) {
        if (project_name != NULL && strcmp(project_name, proj->name) != 0) {
            continue;
        }
        etmemd_project_print(sock_fd, proj);
        exists = true;
    }

    if (!exists) {
        if (project_name == NULL) {
            etmemd_log(ETMEMD_LOG_DEBUG, "no project exists\n");
            dprintf_all(sock_fd, "no project exists\n\n"); 
        } else {
            etmemd_log(ETMEMD_LOG_DEBUG, "project: project %s is not existed\n\n", project_name);
            dprintf_all(sock_fd, "project: project %s is not existed\n\n", project_name);
        }
    }

    return OPT_SUCCESS;
}

static int start_tasks(struct project *proj)
{
    struct engine *eng = NULL;
    int ret = 0;

    for (eng = proj->engs; eng != NULL; eng = eng->next) {
        if (project_start_engine(proj, eng) != 0) {
            ret = -1;
        }
    }

    return ret;
}

static void stop_tasks(struct project *proj)
{
    struct engine *eng = NULL;

    for (eng = proj->engs; eng != NULL; eng = eng->next) {
        project_stop_engine(proj, eng);
    }
}

enum opt_result etmemd_migrate_start(const char *project_name)
{
    struct project *proj = NULL;

    if (project_name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project is not set for start cmd\n");
        return OPT_INVAL;
    }

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get project %s fail.\n", project_name);
        return OPT_PRO_NOEXIST;
    }

    if (proj->start) {
        etmemd_log(ETMEMD_LOG_WARN, "project %s has been started already\n", proj->name);
        return OPT_PRO_STARTED;
    }

    switch (proj->type) {
        case PAGE_SCAN:
            if (start_tasks(proj) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "some task of project %s start fail\n", project_name);
                return OPT_INTER_ERR;
            }
            break;
        case REGION_SCAN:
            if (etmemd_start_damon(proj) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "start damon of project %s fail\n", project_name);
                return OPT_INTER_ERR;
            }
            break;
        default:
            etmemd_log(ETMEMD_LOG_ERR, "scan type %d not support\n", proj->type);
            return OPT_INVAL;
    }

    proj->start = true;
    return OPT_SUCCESS;
}

enum opt_result etmemd_migrate_stop(const char *project_name)
{
    struct project *proj = NULL;

    if (project_name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project is not set for stop cmd\n");
        return OPT_INVAL;
    }

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "get project fail.\n");
        return OPT_PRO_NOEXIST;
    }

    if (!proj->start) {
        etmemd_log(ETMEMD_LOG_WARN, "project %s has been stopped already or not start yet\n",
                   proj->name);
        return OPT_PRO_STOPPED;
    }

    switch (proj->type) {
        case PAGE_SCAN:
            stop_tasks(proj);
            break;
        case REGION_SCAN:
            if (etmemd_stop_damon() != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "stop damon of project %s fail\n", project_name);
                return OPT_INTER_ERR;
            }
            break;
        default:
            etmemd_log(ETMEMD_LOG_ERR, "scan type %d not support\n", proj->type);
            return OPT_INVAL;
    }

    proj->start = false;
    return OPT_SUCCESS;
}

enum opt_result etmemd_project_mgt_engine(const char *project_name, const char *eng_name, char *cmd, char *task_name,
        int sock_fd)
{
    struct engine *eng = NULL;
    struct project *proj = NULL;
    struct task *tk = NULL;

    if (project_name == NULL || eng_name == NULL || cmd == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project, engine and cmd must all be given\n");
        return OPT_INVAL;
    }

    proj = get_proj_by_name(project_name);
    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "project %s is not existed\n", project_name);
        return OPT_PRO_NOEXIST;
    }
    if (!proj->start) {
        etmemd_log(ETMEMD_LOG_ERR, "project %s is not started\n", project_name);
        return OPT_INVAL;
    }

    eng = get_eng_by_name(proj, eng_name);
    if (eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s is not existed\n", eng_name);
        return OPT_ENG_NOEXIST;
    }
    if (task_name != NULL) {
        tk = get_task_by_name(proj, eng, task_name);
        if (tk == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "task %s not found\n", task_name);
            return OPT_TASK_NOEXIST;
        }
    }

    if (eng->ops->eng_mgt_func == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine %s does not support eng_mgt_func\n", eng->name);
        return OPT_INVAL;
    }

    if (eng->ops->eng_mgt_func(eng, tk, cmd, sock_fd) != 0) {
        return OPT_INVAL;
    }
    return OPT_SUCCESS;
}

void etmemd_stop_all_projects(void)
{
    struct project *proj = NULL;

    while (!SLIST_EMPTY(&g_projects)) {
        proj = SLIST_FIRST(&g_projects);
        do_remove_project(proj);
    }
}
