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
 * Description: Etmemd pool adapter API.
 ******************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "etmemd_pool_adapter.h"
#include "etmemd_engine.h"
#include "etmemd_scan.h"

static void push_ctrl_workflow(struct task_pid **tk_pid, void *(*exector)(void *))
{
    struct task_pid *curr_pid = NULL;
    struct task *tk = (*tk_pid)->tk;
    while (*tk_pid != NULL) {
        if (threadpool_add_worker(tk->threadpool_inst,
                                  exector,
                                  (*tk_pid)) != 0) {
            etmemd_log(ETMEMD_LOG_DEBUG, "Failed to push < pid %u, Task_value %s, project_name %s >\n",
                       (*tk_pid)->pid, tk->value, tk->eng->proj->name);
            curr_pid = *tk_pid;
            *tk_pid = (*tk_pid)->next;
            free_task_pid_mem(&curr_pid);
        }

        tk_pid = &((*tk_pid)->next);
    }
    return;
}

static void *launch_threadtimer_executor(void *arg)
{
    struct task_executor *executor = (struct task_executor*)arg;
    struct task *tk = executor->tk;
    thread_pool *pool_inst = NULL;
    bool done = false;
    int execution_size;
    int scheduing_count;

    if (tk->eng->proj->start) {
        if (etmemd_get_task_pids(tk, true) != 0) {
            return NULL;
        }

        push_ctrl_workflow(&tk->pids, executor->func);

        threadpool_notify(tk->threadpool_inst);

        pool_inst = tk->threadpool_inst;
        scheduing_count = __atomic_load_n(&pool_inst->scheduing_size, __ATOMIC_SEQ_CST);
        while (!done) {
            execution_size = __atomic_load_n(&pool_inst->execution_size, __ATOMIC_SEQ_CST);
            if (scheduing_count != execution_size) {
                sleep(1);
                continue;
            }
            done = true;
        }

        threadpool_reset_status(&tk->threadpool_inst);
    }

    return NULL;
}

int start_threadpool_work(struct task_executor *executor)
{
    struct task *tk = executor->tk;
    struct page_scan *page_scan = (struct page_scan *)tk->eng->proj->scan_param;

    etmemd_log(ETMEMD_LOG_DEBUG, "start etmem  for Task_value %s, project_name %s\n",
               tk->value, tk->eng->proj->name);

    /* create the threadpool first and it will start auto */
    tk->threadpool_inst = threadpool_create(tk->max_threads);
    if (tk->threadpool_inst == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "Thread pool creation failed for project <%s> task <%s>.\n",
                   tk->eng->proj->name, tk->value);
        return -1;
    }

    tk->timer_inst = thread_timer_create(page_scan->interval);
    if (tk->timer_inst == NULL) {
        threadpool_stop_and_destroy(&tk->threadpool_inst);
        etmemd_log(ETMEMD_LOG_ERR, "Timer task creation failed for project <%s> task <%s>.\n",
                   tk->eng->proj->name, tk->value);
        return -1;
    }

    if (thread_timer_start(tk->timer_inst, launch_threadtimer_executor, executor) != 0) {
        threadpool_stop_and_destroy(&tk->threadpool_inst);
        thread_timer_destroy(&tk->timer_inst);
        etmemd_log(ETMEMD_LOG_ERR, "Timer task start failed for project <%s> task <%s>.\n",
                   tk->eng->proj->name, tk->value);
        return -1;
    }

    return 0;
}

void stop_and_delete_threadpool_work(struct task *tk)
{
    etmemd_log(ETMEMD_LOG_DEBUG, "stop and delete task <%s> of project <%s>\n",
               tk->value, tk->eng->proj->name);

    if (tk->timer_inst == NULL) {
        etmemd_log(ETMEMD_LOG_DEBUG, "task <%s> of project <%s> has not been started, return\n",
                   tk->value, tk->eng->proj->name);
        return;
    }

    /* stop the threadtimer first */
    thread_timer_stop(tk->timer_inst);

    /* destroy them then */
    thread_timer_destroy(&tk->timer_inst);
    threadpool_stop_and_destroy(&tk->threadpool_inst);
}

