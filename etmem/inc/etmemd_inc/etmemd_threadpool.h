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
 * Description: This is a header file of the function declaration for threadpool function
 *              and the data structure definition for threadpool.
 ******************************************************************************/

#ifndef ETMEMD_THREADPOOL_H
#define ETMEMD_THREADPOOL_H

#include <stdint.h>

/*
* task worker list
* */
typedef void *(*worker_functor)(void *);
typedef struct thread_worker_t {
    worker_functor functor;
    void *arg;
    struct thread_worker_t *next_node;
} thread_worker;

typedef struct thread_pool_t {
    bool down;
    bool cancel;
    int max_thread_cap;
    int scheduing_size;
    thread_worker *worker_list;
    pthread_t *tid;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int execution_size;
} thread_pool;

/*
 * Thread pools support multiple instances and are thread safe
 * */
thread_pool* threadpool_create(uint64_t max_thread_num);

/*
 * Add tasks to the thread pool, thread safe
 * */
int threadpool_add_worker(thread_pool* inst, void *(*executor)(void *arg), void *arg);

/*
 * Notify consumers
 * */
void threadpool_notify(thread_pool* inst);

/*
 * Query scheduing status in the current thread pool, thread safety
 * */
int threadpool_get_scheduing_size(thread_pool* inst);

/*
 * Query execution status in the current thread pool, thread safety
 * */
int threadpool_get_execution_size(thread_pool* inst);

/*
 * Reset the queue state of the thread pool , Non-thread-safe, used in a specific context .
 * */
void threadpool_reset_status(thread_pool** inst);

/*
 * Stop and destroy the thread pool instance
 * */
void threadpool_stop_and_destroy(thread_pool** inst);

#endif //ETMEMD_THREADPOOL_H
