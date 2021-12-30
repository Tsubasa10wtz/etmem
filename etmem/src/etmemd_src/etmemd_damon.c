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
 * Author: geruijun
 * Create: 2021-10-28
 * Description: Add etmemd damon engine.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_file.h"
#include "etmemd_engine.h"
#include "etmemd_task.h"
#include "etmemd_task_exp.h"
#include "etmemd_scan.h"
#include "etmemd_damon.h"

#define KERNEL_DAMON_PATH "/sys/kernel/debug/damon/"
#define DAMON_PARAM_TARGET_IDS "target_ids"
#define DAMON_PARAM_ATTRS "attrs"
#define DAMON_PARAM_SCHEMES "schemes"
#define DAMON_PARAM_MONITOR_ON "monitor_on"

#define ON_LEN 2
#define OFF_LEN 3
#define INT_MAX_LEN 10
#define NUM_OF_ATTRS 5
#define NUM_OF_SCHEMES 7

enum damos_action {
    DAMOS_WILLNEED,
    DAMOS_COLD,
    DAMOS_PAGEOUT,
    DAMOS_HUGEPAGE,
    DAMOS_NOHUGEPAGE,
    DAMOS_STAT,
};

struct action_item {
    char *action_str;
    enum damos_action action_type;
};

struct damon_eng_params {
    unsigned long min_sz_region;
    unsigned long max_sz_region;
    unsigned int min_nr_accesses;
    unsigned int max_nr_accesses;
    unsigned int min_age_region;
    unsigned int max_age_region;
    enum damos_action action;
};

static bool check_damon_exist(void)
{
    if (access(KERNEL_DAMON_PATH, F_OK) != 0) {
        return false;
    }
    return true;
}

static FILE *get_damon_file(const char *file)
{
    char *file_name = NULL;
    size_t file_str_size;
    FILE *fp = NULL;

    file_str_size = strlen(KERNEL_DAMON_PATH) + strlen(file) + 1;
    file_name = (char *)calloc(file_str_size, sizeof(char));
    if (file_name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for damon file %s fail\n", file);
        return NULL;
    }

    if (snprintf_s(file_name, file_str_size, file_str_size - 1, "%s%s",
                   KERNEL_DAMON_PATH, file) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for damon file %s fail\n", file);
        goto out;
    }

    fp = fopen(file_name, "r+");
    if (fp == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "open file %s fail\n", file_name);
    }

out:
    free(file_name);
    return fp;
}

static bool is_engs_valid(struct project *proj)
{
    struct engine *eng = proj->engs;

    while (eng != NULL) {
        if (strcmp(eng->name, "damon") != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "engine type %s not supported, only support damon engine in region scan\n",
                       eng->name);
            return false;
        }
        eng = eng->next;
    }

    return true;
}

static int get_damon_pids_val_and_num(struct task *tk, int *nr_tasks)
{
    while (tk != NULL) {
        if (etmemd_get_task_pids(tk, false) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "damon fail to get task pids\n");
            *nr_tasks = 0;
            return -1;
        }
        (*nr_tasks)++;
        tk = tk->next;
    }

    return 0;
}

static char *get_damon_pids_str(struct task *tk, const int nr_tasks)
{
    char *pids = NULL;
    size_t pids_size;
    char tmp_pid[PID_STR_MAX_LEN + 2] = {0}; // plus 2 for space and '\0' follow pid

    pids_size = (PID_STR_MAX_LEN + 1) * nr_tasks + 1;
    pids = (char *)calloc(pids_size, sizeof(char));
    if (pids == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for pids in damon fail\n");
        return NULL;
    }

    while (tk != NULL) {
        if (snprintf_s(tmp_pid, PID_STR_MAX_LEN + 2, PID_STR_MAX_LEN + 1,
                       "%u ", tk->pids->pid) == -1) {
            etmemd_log(ETMEMD_LOG_WARN, "snprintf pid %u in damon fail\n", tk->pids->pid);
            tk = tk->next;
            continue;
        }

        if (strcat_s(pids, pids_size, tmp_pid) != EOK) {
            etmemd_log(ETMEMD_LOG_WARN, "strcat pid %s fail\n", tmp_pid);
        }
        tk = tk->next;
    }

    return pids;
}

static int set_damon_target_ids(struct project *proj)
{
    FILE *fp = NULL;
    int nr_tasks = 0;
    struct task *tk = proj->engs->tasks;
    char *pids_str = NULL;
    size_t pids_len;
    int ret = -1;

    if (!is_engs_valid(proj)) {
        goto out;
    }

    fp = get_damon_file(DAMON_PARAM_TARGET_IDS);
    if (fp == NULL) {
        goto out;
    }

    if (get_damon_pids_val_and_num(tk, &nr_tasks) != 0) {
        goto out_close;
    }

    pids_str = get_damon_pids_str(tk, nr_tasks);
    if (pids_str == NULL) {
        goto out_close;
    }

    pids_len = strlen(pids_str);
    if (pids_len == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get all task pids fail in damon\n");
        goto out_free;
    }

    if (fwrite(pids_str, sizeof(char), pids_len + 1, fp) != pids_len + 1) {
        etmemd_log(ETMEMD_LOG_ERR, "write pids to damon fail\n");
        goto out_free;
    }
    ret = 0;

out_free:
    free(pids_str);
out_close:
    fclose(fp);
out:
    return ret;
}

static char *get_damon_attrs_str(struct project *proj)
{
    char *attrs = NULL;
    size_t attrs_size;
    struct region_scan *reg_scan = (struct region_scan *)proj->scan_param;

    attrs_size = (INT_MAX_LEN + 1) * NUM_OF_ATTRS;
    attrs = (char *)calloc(attrs_size, sizeof(char));
    if (attrs == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for attrs in damon fail\n");
        return NULL;
    }

    if (snprintf_s(attrs, attrs_size, attrs_size - 1, "%lu %lu %lu %lu %lu",
                   reg_scan->sample_interval, reg_scan->aggr_interval,
                   reg_scan->update_interval, reg_scan->min_nr_regions,
                   reg_scan->max_nr_regions) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for attrs fail\n");
        free(attrs);
        return NULL;
    }

    return attrs;
}

static int set_damon_attrs(struct project *proj)
{
    FILE *fp = NULL;
    char *attrs_str = NULL;
    size_t attrs_len;
    int ret = -1;
    
    fp = get_damon_file(DAMON_PARAM_ATTRS);
    if (fp == NULL) {
        goto out;
    }

    attrs_str = get_damon_attrs_str(proj);
    if (attrs_str == NULL) {
        goto out_close;
    }

    attrs_len = strlen(attrs_str);
    if (fwrite(attrs_str, sizeof(char), attrs_len + 1, fp) != attrs_len + 1) {
        etmemd_log(ETMEMD_LOG_ERR, "write attrs to damon fail\n");
        goto out_free;
    }
    ret = 0;

out_free:
    free(attrs_str);
out_close:
    fclose(fp);
out:
    return ret;
}

static char *get_damon_schemes_str(struct project *proj)
{
    char *schemes = NULL;
    size_t schemes_size;
    struct damon_eng_params *params = (struct damon_eng_params *)proj->engs->params;

    schemes_size = (INT_MAX_LEN + 1) * NUM_OF_SCHEMES;
    schemes = (char *)calloc(schemes_size, sizeof(char));
    if (schemes == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for schemes in damon fail\n");
        return NULL;
    }

    if (snprintf_s(schemes, schemes_size, schemes_size - 1, "%lu %lu %u %u %u %u %d",
                   params->min_sz_region, params->max_sz_region,
                   params->min_nr_accesses, params->max_nr_accesses,
                   params->min_age_region, params->max_age_region,
                   params->action) == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for schemes fail\n");
        free(schemes);
        return NULL;
    }

    return schemes;
}

static int set_damon_schemes(struct project *proj)
{
    FILE *fp = NULL;
    char *schemes_str = NULL;
    size_t schemes_len;
    int ret = -1;

    fp = get_damon_file(DAMON_PARAM_SCHEMES);
    if (fp == NULL) {
        goto out;
    }

    schemes_str = get_damon_schemes_str(proj);
    if (schemes_str == NULL) {
        goto out_close;
    }

    schemes_len = strlen(schemes_str);
    if (fwrite(schemes_str, sizeof(char), schemes_len + 1, fp) != schemes_len + 1) {
        etmemd_log(ETMEMD_LOG_ERR, "write schemes to damon fail\n");
        goto out_free;
    }
    ret = 0;

out_free:
    free(schemes_str);
out_close:
    fclose(fp);
out:
    return ret;
}

static char *get_damon_monitor_on_str(bool start)
{
    char *switch_str = NULL;
    size_t switch_size;

    switch_size = start ? ON_LEN + 1 : OFF_LEN + 1;
    switch_str = (char *)calloc(switch_size, sizeof(char));
    if (switch_str == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for switch in damon fail\n");
        return NULL;
    }
    if (snprintf_s(switch_str, switch_size, switch_size - 1, start ? "on" : "off") == -1) {
        etmemd_log(ETMEMD_LOG_ERR, "snprintf for switch fail\n");
        free(switch_str);
        return NULL;
    }

    return switch_str;
}

static int set_damon_monitor_on(bool start)
{
    FILE *fp = NULL;
    int ret = -1;
    char *switch_str = NULL;
    size_t switch_len;
    
    fp = get_damon_file(DAMON_PARAM_MONITOR_ON);
    if (fp == NULL) {
        goto out;
    }

    switch_str = get_damon_monitor_on_str(start);
    if (switch_str == NULL) {
        goto out_close;
    }

    switch_len = strlen(switch_str);
    if (fwrite(switch_str, sizeof(char), switch_len + 1, fp) != switch_len + 1) {
        etmemd_log(ETMEMD_LOG_ERR, "write monitor_on to damon fail\n");
        goto out_free;
    }
    ret = 0;

out_free:
    free(switch_str);
out_close:
    fclose(fp);
out:
    return ret;
}

int etmemd_start_damon(struct project *proj)
{
    bool start = true;

    if (proj == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "proj should not be NULL\n");
        return -1;
    }

    if (!check_damon_exist()) {
        etmemd_log(ETMEMD_LOG_ERR, "kernel damon module not exist\n");
        return -1;
    }

    if (set_damon_target_ids(proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set damon pids fail\n");
        return -1;
    }

    if (set_damon_attrs(proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set damon attrs fail\n");
        return -1;
    }

    if (set_damon_schemes(proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set damon schemes fail\n");
        return -1;
    }

    if (set_damon_monitor_on(start) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set damon monitor_on to on fail\n");
        return -1;
    }

    return 0;
}

int etmemd_stop_damon(void)
{
    bool start = false;

    if (!check_damon_exist()) {
        etmemd_log(ETMEMD_LOG_ERR, "kernel damon module not exist\n");
        return -1;
    }

    if (set_damon_monitor_on(start) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set damon monitor_on to off fail\n");
        return -1;
    }

    return 0;
}

static int fill_min_size(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned long min_size = parse_to_ulong(val);

    params->min_sz_region = min_size;
    return 0;
}

static int fill_max_size(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned long max_size = parse_to_ulong(val);

    params->max_sz_region = max_size;
    return 0;
}

static int fill_min_acc(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned int min_acc = parse_to_uint(val);

    params->min_nr_accesses = min_acc;
    return 0;
}

static int fill_max_acc(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned int max_acc = parse_to_uint(val);

    params->max_nr_accesses = max_acc;
    return 0;
}

static int fill_min_age(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned int min_age = parse_to_uint(val);

    params->min_age_region = min_age;
    return 0;
}

static int fill_max_age(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    unsigned int max_age = parse_to_uint(val);

    params->max_age_region = max_age;
    return 0;
}

static struct action_item damon_action_items[] = {
    {"willneed", DAMOS_WILLNEED},
    {"cold", DAMOS_COLD},
    {"pageout", DAMOS_PAGEOUT},
    {"hugepage", DAMOS_HUGEPAGE},
    {"nohugepage", DAMOS_NOHUGEPAGE},
    {"stat", DAMOS_STAT},
};

static int fill_action(void *obj, void *val)
{
    struct damon_eng_params *params = (struct damon_eng_params *)obj;
    char *action = (char *)val;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(damon_action_items); i++) {
        if (strcmp(action, damon_action_items[i].action_str) == 0) {
            params->action = damon_action_items[i].action_type;
            free(action);
            return 0;
        }
    }

    free(action);
    return -1;
}

static struct config_item damon_eng_config_items[] = {
    {"min_size", INT_VAL, fill_min_size, false},
    {"max_size", INT_VAL, fill_max_size, false},
    {"min_acc", INT_VAL, fill_min_acc, false},
    {"max_acc", INT_VAL, fill_max_acc, false},
    {"min_age", INT_VAL, fill_min_age, false},
    {"max_age", INT_VAL, fill_max_age, false},
    {"action", STR_VAL, fill_action, false},
};

static int damon_fill_eng(GKeyFile *config, struct engine *eng)
{
    struct damon_eng_params *params = calloc(1, sizeof(struct damon_eng_params));

    if (params == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "alloc damon engine params fail\n");
        return -1;
    }

    if (parse_file_config(config, ENG_GROUP, damon_eng_config_items,
        ARRAY_SIZE(damon_eng_config_items), (void *)params) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "damon fill engine params fail.\n");
        free(params);
        return -1;
    }

    eng->params = (void *)params;
    return 0;
}

static void damon_clear_eng(struct engine *eng)
{
    struct damon_eng_params *eng_params = (struct damon_eng_params *)eng->params;

    if (eng_params == NULL) {
        return;
    }

    free(eng_params);
    eng->params = NULL;
}

struct engine_ops g_damon_eng_ops = {
    .fill_eng_params = damon_fill_eng,
    .clear_eng_params = damon_clear_eng,
    .fill_task_params = NULL,
    .clear_task_params = NULL,
    .start_task = NULL,
    .stop_task = NULL,
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
    .eng_mgt_func = NULL,
};

int fill_engine_type_damon(struct engine *eng, GKeyFile *config)
{
    eng->ops = &g_damon_eng_ops;
    eng->engine_type = DAMON_ENGINE;
    eng->name = "damon";
    return 0;
}
