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
 * Create: 2021-12-10
 * Description: This is a source file of the unit test for task functions in etmem.
 ******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "test_common.h"
#include "etmemd_thirdparty_export.h"

char *get_private_value(GKeyFile *config, char *group, char *key)
{
    if (g_key_file_has_key(config, group, key, NULL) == FALSE) {
        return NULL;
    }

    return g_key_file_get_string(config, group, key, NULL);
}

char *get_params_str(void *str)
{
    return str == NULL ? "NULL" : (char *)str;
}

int fill_eng_params(GKeyFile *config, struct engine *eng)
{
    /* engine_private_key is user defined key name in engine group in config file */
    eng->params = get_private_value(config, "engine", "engine_private_key");
    if (strcmp(eng->params, "1024") != 0) {
        return -1;
    }

    return 0;
}

void clear_eng_params(struct engine *eng)
{
    if (eng->params != NULL) {
        free(eng->params);
        eng->params = NULL;
    }
}

int fill_task_params(GKeyFile *config, struct task *task)
{
    /* task_private_key is user defined key name in task group in config file */
    task->params = get_private_value(config, "task", "task_private_key");
    if (strcmp(task->params, "2048") != 0) {
        return -1;
    }

    return 0;
}

void clear_task_params(struct task *task)
{
    if (task->params != NULL) {
        free(task->params);
        task->params = NULL;
    }
}

int start_task(struct engine *eng, struct task *task)
{
    return 0;
}

static int my_start_task(struct engine *eng, struct task *tk)
{
    return 0;
}

int error_start_task(struct engine *eng, struct task *task)
{
    return -1;
}

void stop_task(struct engine *eng, struct task *task)
{
}

int alloc_pid_params(struct engine *eng, struct task_pid **tk_pid)
{
    return 0;
}

void free_pid_params(struct engine *eng, struct task_pid **tk_pid)
{
}

int eng_mgt_func(struct engine *eng, struct task *task, char *cmd, int fd)
{
    char *msg = "msg to client\n";

    write(fd, msg, strlen(msg));
    return 0;
}

struct engine_ops my_engine_ops = {
    .fill_eng_params = fill_eng_params,
    .clear_eng_params = clear_eng_params,
    .fill_task_params = fill_task_params,
    .clear_task_params = clear_task_params,
    .start_task = start_task,
    .stop_task = stop_task,
    .alloc_pid_params = alloc_pid_params,
    .free_pid_params = free_pid_params,
    .eng_mgt_func = eng_mgt_func,
};

struct engine_ops start_error_ops = {
    .fill_eng_params = fill_eng_params,
    .clear_eng_params = clear_eng_params,
    .fill_task_params = fill_task_params,
    .clear_task_params = clear_task_params,
    .start_task = error_start_task,
    .stop_task = stop_task,
    .alloc_pid_params = alloc_pid_params,
    .free_pid_params = free_pid_params,
    .eng_mgt_func = eng_mgt_func,
};

struct engine_ops start_not_null_ops = {
    .fill_eng_params = NULL,
    .clear_eng_params = NULL,
    .fill_task_params = NULL,
    .clear_task_params = NULL,
    .start_task = my_start_task,
    .stop_task = NULL,
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
    .eng_mgt_func = NULL,
};

struct engine_ops start_null_ops = {
    .fill_eng_params = NULL,
    .clear_eng_params = NULL,
    .fill_task_params = NULL,
    .clear_task_params = NULL,
    .start_task = NULL,
    .stop_task = NULL,
    .alloc_pid_params = NULL,
    .free_pid_params = NULL,
    .eng_mgt_func = NULL,
};
