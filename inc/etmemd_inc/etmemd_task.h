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
 * Description: This is a header file of the function declaration for etmemd task function
 *              and the data structure definition for etmem task.
 ******************************************************************************/

#ifndef ETMEMD_TASK_H
#define ETMEMD_TASK_H

#include <stdbool.h>
#include <glib.h>
#include "etmemd_threadpool.h"
#include "etmemd_threadtimer.h"
#include "etmemd_task_exp.h"

struct task_pid {
    unsigned int pid;
    float rt_swapin_rate;   /* real time swapin rate */
    void *params;           /* pid personal parameter */
    struct task *tk;        /* point to its task */
    struct task_pid *next;
};

int etmemd_get_task_pids(struct task *tk, bool recursive);

void etmemd_free_task_pids(struct task *tk);

int get_pid_from_task_type(const struct task *tk, char *pid);

void etmemd_free_task_struct(struct task **tk);

void free_task_pid_mem(struct task_pid **tk_pid);

void etmemd_print_tasks(int fd, const struct task *tk, char *engine_name, bool started);

struct task *etmemd_add_task(GKeyFile *config);
void etmemd_remove_task(struct task *tk);

#endif
