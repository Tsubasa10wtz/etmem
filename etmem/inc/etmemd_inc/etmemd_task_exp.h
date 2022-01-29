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
 * Create: 2021-4-30
 * Description: This is a header file of the export task symbols.
 ******************************************************************************/

#ifndef ETMEMD_TASK_EXP_H
#define ETMEMD_TASK_EXP_H

#include <stdint.h>
#include <pthread.h>

struct timer_thread_t;
typedef struct timer_thread_t timer_thread;
struct thread_pool_t;
typedef struct thread_pool_t thread_pool;

struct task {
    char *type;
    char *value;
    char *name;
    int max_threads;
    int swap_flag;

    struct task_pid *pids;
    struct engine *eng;
    void *params;
    pthread_t task_pt;
    timer_thread *timer_inst;
    thread_pool *threadpool_inst;

    struct task *next;
};

#endif
