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
 * Description: File operation API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "securec.h"
#include "etmemd_log.h"
#include "etmemd_common.h"
#include "etmemd_project.h"
#include "etmemd_engine.h"
#include "etmemd_file.h"

#define MAX_INTERVAL_VALUE 1200
#define MAX_SLEEP_VALUE 1200
#define MAX_LOOP_VALUE 120

static int fill_project_interval(struct project *proj, const char *val)
{
    int value;

    if (get_int_value(val, &value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project interval value.\n");
        return -1;
    }

    if (value < 1 || value > MAX_INTERVAL_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project interval value, must between 1 and 1200.\n");
        return -1;
    }

    proj->interval = value;

    return 0;
}

static int fill_project_loop(struct project *proj, const char *val)
{
    int value;

    if (get_int_value(val, &value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project loop value.\n");
        return -1;
    }

    if (value < 1 || value > MAX_LOOP_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project loop value, must between 1 and 120.\n");
        return -1;
    }

    proj->loop = value;

    return 0;
}

static int fill_project_sleep(struct project *proj, const char *val)
{
    int value;

    if (get_int_value(val, &value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project sleep value.\n");
        return -1;
    }

    if (value < 1 || value > MAX_SLEEP_VALUE) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid project sleep value, must between 1 and 1200.\n");
        return -1;
    }

    proj->sleep = value;

    return 0;
}

static struct project_item g_project_items[] = {
    {"interval", fill_project_interval, false, false},
    {"loop", fill_project_loop, false, false},
    {"sleep", fill_project_sleep, false, false},
    {NULL, NULL, false, false},
};

static int fill_project_params(struct project *proj, const char *key,
                               const char *val)
{
    int ret = -1;
    int i = 0;

    while (g_project_items[i].proj_sec_name != NULL) {
        if (strcmp(key, g_project_items[i].proj_sec_name) == 0) {
            ret = g_project_items[i].fill_proj_func(proj, val);
            break;
        }

        i++;
    }

    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse %s project config section fail\n", key);
        return -1;
    }

    g_project_items[i].set = true;
    return 0;
}

static int fill_task_type(struct task *new_task, const char *val)
{
    if (strcmp(val, "pid") != 0 && strcmp(val, "name") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid task type, must be pid or name.\n");
        return -1;
    }

    if (new_task->type != NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "duplicate config for task type.\n");
        return 0;
    }

    new_task->type = calloc(1, strlen(val) + 1);
    if (new_task->type == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc task type fail.\n");
        return -1;
    }

    if (strncpy_s(new_task->type, strlen(val) + 1, val, strlen(val)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "strncpy_s for task type fail.\n");
        free(new_task->type);
        new_task->type = NULL;
        return -1;
    }

    return 0;
}

static int fill_task_value(struct task *new_task, const char *val)
{
    if (new_task->value != NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "duplicate config for task value.\n");
        return 0;
    }

    new_task->value = calloc(1, strlen(val) + 1);
    if (new_task->value == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "malloc task value fail.\n");
        return -1;
    }

    if (strncpy_s(new_task->value, strlen(val) + 1, val, strlen(val)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "strncpy_s for task value fail.\n");
        free(new_task->value);
        new_task->value = NULL;
        return -1;
    }

    return 0;
}

static int fill_task_engine(struct task *new_task, const char *val)
{
    if (new_task->eng != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "engine is already configured\n");
        return -1;
    }

    new_task->eng = (struct engine *)calloc(1, sizeof(struct engine));
    if (new_task->eng == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc engine fail\n");
        return -1;
    }

    new_task->eng->task = (void *)new_task;

    if (fill_engine_type(new_task->eng, val) != 0) {
        free(new_task->eng);
        new_task->eng = NULL;
        return -1;
    }

    return 0;
}

static int fill_task_max_threads(struct task *new_task, const char *val)
{
    int value;
    int core;

    if (get_int_value(val, &value) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid task max_threads value.\n");
        return -1;
    }

    if (value <= 0) {
        etmemd_log(ETMEMD_LOG_WARN,
                   "Thread count is abnormal, set the default minimum of current thread count to 1\n");
        value = 1;
    }

    core = get_nprocs();
    /*
     * For IO intensive businesses,
     * max-threads is limited to 2N+1 of the maximum number of threads
     * */
    if (value > 2 * core + 1) {
        etmemd_log(ETMEMD_LOG_WARN,
                   "max-threads is limited to 2N+1 of the maximum number of threads\n");
        value = 2 * core + 1;
    }

    new_task->max_threads = value;

    return 0;
}

static struct task_item g_task_items[] = {
    {"type", fill_task_type, false, false},
    {"value", fill_task_value, false, false},
    {"engine", fill_task_engine, false, false},
    {"max_threads", fill_task_max_threads, true, false},
    {NULL, NULL, false, false},
};

static int fill_task_params(struct task *new_task, const char *key,
                            const char *val)
{
    int ret = -1;
    int i = 0;

    while (g_task_items[i].task_sec_name != NULL) {
        if (strcmp(key, g_task_items[i].task_sec_name) == 0) {
            ret = g_task_items[i].fill_task_func(new_task, val);
            break;
        }

        i++;
    }

    if (ret != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse %s task config section fail\n", key);
        return -1;
    }

    g_task_items[i].set = true;
    return 0;
}

static int process_engine_param_keyword(const char *get_line, struct engine *eng,
                                        FILE *conf_file, int *is_param)
{
    if (strcmp(get_line, "param") == 0) {
        if (eng == NULL) {
            etmemd_log(ETMEMD_LOG_ERR, "must configure engine type first\n");
            return -1;
        }

        if (eng->parse_param_conf(eng, conf_file) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "parse engine parameters fail\n");
            return -1;
        }

        *is_param = 1;
    }

    return 0;
}

static bool etmemd_check_task_params(void)
{
    int i = 0;

    while (g_task_items[i].task_sec_name != NULL) {
        /* do not check for the parameter which is optional */
        if (g_task_items[i].optional) {
            i++;
            continue;
        }

        /* and the other parameters must be set */
        if (!g_task_items[i].set) {
            etmemd_log(ETMEMD_LOG_ERR, "%s section must be set for task parameters\n",
                       g_task_items[i].task_sec_name);
            return false;
        }

        /* reset the flag of set of the section, and no need to do this for the optional ones */
        g_task_items[i].set = false;
        i++;
    }

    return true;
}

/*
 * new_task created in this function is needed during the whole
 * process life cycle of etmemd, and will be released when
 * etmemd exit in function etmemd_free_task_struct
 * */
static int get_task_params(FILE *conf_file, struct project *proj)
{
    struct task *new_task = NULL;
    char key[KEY_VALUE_MAX_LEN] = {};
    char value[KEY_VALUE_MAX_LEN] = {};
    char *get_line = NULL;
    int is_param = 0;

    new_task = (struct task *)calloc(1, sizeof(struct task));
    if (new_task == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc task fail\n");
        return -1;
    }

    new_task->proj = proj;
    new_task->next = proj->tasks;
    /* set default count of the thread pool to 1 */
    new_task->max_threads = 1;

    while ((get_line = skip_blank_line(conf_file)) != NULL) {
        if (strcmp(get_line, "policies") != 0) {
            if (process_engine_param_keyword(get_line, new_task->eng,
                                             conf_file, &is_param) != 0) {
                goto out_err;
            }

            if (is_param == 1) {
                is_param = 0;
                continue;
            }

            if (get_keyword_and_value(get_line, key, value) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "get task keyword and value fail\n");
                goto out_err;
            }

            if (fill_task_params(new_task, key, value) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "fill task parameter fail\n");
                goto out_err;
            }

            continue;
        }

        goto next_task;
    }

next_task:
    if (etmemd_check_task_params()) {
        proj->tasks = new_task;
    } else {
        goto out_err;
    }

    if (get_line == NULL) {
        return 0;
    }

    return get_task_params(conf_file, proj);

out_err:
    etmemd_free_task_struct(&new_task);
    return -1;
}

static int get_project_params(FILE *conf_file, struct project *proj)
{
    char key[KEY_VALUE_MAX_LEN] = {};
    char value[KEY_VALUE_MAX_LEN] = {};
    char *get_line = NULL;

    while ((get_line = skip_blank_line(conf_file)) != NULL) {
        if (strcmp(get_line, "policies") != 0) {
            if (get_keyword_and_value(get_line, key, value) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "get project keyword and value fail\n");
                return -1;
            }

            if (fill_project_params(proj, key, value) != 0) {
                etmemd_log(ETMEMD_LOG_ERR, "fill project parameter fail\n");
                return -1;
            }

            continue;
        }

        if (get_task_params(conf_file, proj) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get task parameters fail\n");
            return -1;
        }
    }

    return 0;
}

static bool etmemd_check_project_params(void)
{
    int i = 0;

    while (g_project_items[i].proj_sec_name != NULL) {
        /* do not check for the parameter which is optional */
        if (g_project_items[i].optional) {
            i++;
            continue;
        }

        /* and the other parameters must be set */
        if (!g_project_items[i].set) {
            etmemd_log(ETMEMD_LOG_ERR, "%s section must be set for project parameters\n",
                       g_project_items[i].proj_sec_name);
            return false;
        }

        /* reset the flag of set of the section, and no need to do this for the optional ones */
        g_project_items[i].set = false;
        i++;
    }

    return true;
}

int etmemd_fill_proj_by_conf(struct project *proj, FILE *conf_file)
{
    char *get_line = NULL;

    get_line = skip_blank_line(conf_file);
    if (get_line == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid config file, should not be empty\n");
        return -1;
    }

    if (strcmp(get_line, "options") != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "invalid config file, must begin with \"options\"\n");
        return -1;
    }

    if (get_project_params(conf_file, proj) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "get project parameters fail\n");
        return -1;
    }

    if (!etmemd_check_project_params()) {
        return -1;
    }

    return 0;
}
