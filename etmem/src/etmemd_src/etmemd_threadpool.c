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
 * Description: Etmemd threadpool API.
 ******************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "etmemd_log.h"
#include "etmemd_threadpool.h"

static __thread pthread_mutex_t *g_pool_lock = NULL;

static void cancel_unschedul_tasks(thread_pool *inst)
{
    thread_worker *head = NULL;

    if (inst == NULL || inst->worker_list == NULL) {
        return;
    }

    while (inst->worker_list != NULL) {
        head = inst->worker_list;
        inst->worker_list = inst->worker_list->next_node;
        __atomic_add_fetch(&inst->execution_size, 1, __ATOMIC_SEQ_CST);
        free(head);
    }

    etmemd_log(ETMEMD_LOG_DEBUG, "Unscheduled tasks in the pool have been canceled\n");
    return;
}

static void threadpool_cancel_unlock(void *arg)
{
    if (g_pool_lock == NULL) {
        return;
    }
    etmemd_log(ETMEMD_LOG_DEBUG, "unlock for threadpool once\n");
    pthread_mutex_unlock(g_pool_lock);
}

static void *threadpool_routine(void *arg)
{
    thread_pool *thread_inst = (thread_pool *)arg;
    worker_functor functor = NULL;
    void *func_arg = NULL;

    pthread_cleanup_push(threadpool_cancel_unlock, NULL);
    while (true) {
        pthread_mutex_lock(&thread_inst->lock);
        g_pool_lock = &(thread_inst->lock);
        while (__atomic_load_n(&thread_inst->scheduing_size, __ATOMIC_SEQ_CST) == 0) {
            pthread_cond_wait(&(thread_inst->cond), &(thread_inst->lock));
        }
        thread_worker *worker = thread_inst->worker_list;
        if (worker != NULL) {
            thread_inst->worker_list = worker->next_node;
            functor = worker->functor;
            func_arg = worker->arg;
            free(worker);
            worker = NULL;
            pthread_mutex_unlock(&(thread_inst->lock));
            g_pool_lock = NULL;
            (*functor)(func_arg);
            functor = NULL;
            func_arg = NULL;
            __atomic_add_fetch(&thread_inst->execution_size, 1, __ATOMIC_SEQ_CST);
            continue;
        }
        pthread_mutex_unlock(&(thread_inst->lock));
        g_pool_lock = NULL;
        sleep(1);
    }
    pthread_cleanup_pop(0);

    return NULL;
}

static int pool_pthread_init(thread_pool *pool)
{
    if (pthread_mutex_init(&(pool->lock), NULL) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "init mutex fail\n");
        return -1;
    }

    if (pthread_cond_init(&(pool->cond), NULL) != 0) {
        pthread_mutex_destroy(&(pool->lock));
        etmemd_log(ETMEMD_LOG_ERR, "init condition fail\n");
        return -1;
    }

    return 0;
}

thread_pool *threadpool_create(uint64_t max_thread_num)
{
    bool create = true;
    uint64_t i;

    thread_pool *pool = (thread_pool *)malloc(sizeof(thread_pool));
    if (pool == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for thread_pool fail\n");
        return NULL;
    }
    pool->tid = NULL;
    pool->down = false;
    __atomic_store_n(&pool->scheduing_size, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&pool->execution_size, 0, __ATOMIC_SEQ_CST);
    pool->worker_list = NULL;

    if (max_thread_num < 1) {
        etmemd_log(ETMEMD_LOG_ERR, "max thread num should not be smaller than 1\n");
        goto exit;
    }
    pool->tid = (pthread_t *)malloc(max_thread_num * sizeof(pthread_t));
    if (pool->tid == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for thread_pool fail\n");
        goto exit;
    }

    if (pool_pthread_init(pool) != 0) {
        goto exit;
    }

    for (i = 0; i < max_thread_num; i++) {
        if (pthread_create(&(pool->tid[i]), NULL, threadpool_routine, pool) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "start threadpool fail\n");
            create = false;
            break;
        }
    }
    if (create != true) {
        threadpool_stop_and_destroy(&pool);
        etmemd_log(ETMEMD_LOG_ERR, "start threadpool fail\n");
        return NULL;
    }

    etmemd_log(ETMEMD_LOG_DEBUG, "thread pool creates and starts successfully!\n");
    pool->max_thread_cap = i;
    return pool;
exit:
    if (pool->tid != NULL) {
        free(pool->tid);
        pool->tid = NULL;
    }
    free(pool);
    return NULL;
}

int threadpool_add_worker(thread_pool *inst, void *(*executor)(void *arg), void *arg)
{
    if (executor == NULL || arg == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "threadpool instance startup parameter exception.\n");
        return -1;
    }
    thread_worker *tworker = (thread_worker *)malloc(sizeof(thread_worker));
    if (tworker == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for worker struct fail\n");
        return -1;
    }
    tworker->functor = executor;
    tworker->arg = arg;
    tworker->next_node = NULL;
    pthread_mutex_lock(&(inst->lock));
    thread_worker *worker = inst->worker_list;
    if (worker != NULL) {
        inst->worker_list = tworker;
        tworker->next_node = worker;
    } else {
        inst->worker_list = tworker;
    }
    __atomic_add_fetch(&inst->scheduing_size, 1, __ATOMIC_SEQ_CST);
    pthread_mutex_unlock(&(inst->lock));

    return 0;
}

void threadpool_notify(thread_pool *inst)
{
    if (inst == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "The thread pool instance is null !\n");
        return;
    }
    pthread_mutex_lock(&(inst->lock));
    pthread_cond_broadcast(&(inst->cond));
    pthread_mutex_unlock(&(inst->lock));
}

static void threadpool_cancel_tasks_working(const thread_pool *inst)
{
    int i;

    for (i = 0; i < inst->max_thread_cap; i++) {
        pthread_cancel(inst->tid[i]);
    }
}

void threadpool_reset_status(thread_pool **inst)
{
    if (inst == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "Reset status faild, thread pool instance is null !\n");
        return;
    }
    thread_pool *thread_instance = (thread_pool *)(*inst);
    pthread_mutex_lock(&(thread_instance->lock));
    /* set the count of tasks schedued and executed to 0 for the next loop */
    __atomic_store_n(&thread_instance->scheduing_size, 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&thread_instance->execution_size, 0, __ATOMIC_SEQ_CST);
    pthread_mutex_unlock(&(thread_instance->lock));
}

void threadpool_stop_and_destroy(thread_pool **inst)
{
    int i;
    thread_pool *thread_instance = NULL;

    if (inst == NULL || *inst == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "The thread pool instance is null !\n");
        return;
    }

    /* stop the threadpool first */
    thread_instance = *inst;
    threadpool_cancel_tasks_working(thread_instance);

    for (i = 0; i < thread_instance->max_thread_cap; i++) {
        pthread_join(thread_instance->tid[i], NULL);
    }
    free(thread_instance->tid);

    cancel_unschedul_tasks(thread_instance);
    if (pthread_mutex_destroy(&(thread_instance->lock)) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "The thread pool instance destroy faild !\n");
    }
    if (pthread_cond_destroy(&(thread_instance->cond)) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "The thread pool instance destroy faild !\n");
    }

    free(thread_instance);
    *inst = NULL;

    etmemd_log(ETMEMD_LOG_DEBUG, "Threadpool instance delete !\n");
}
