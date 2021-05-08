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
 * Description: Etmemd threadtimer API.
 ******************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include "etmemd_log.h"
#include "etmemd_threadtimer.h"

static void threadtimer_cancel_unlock(void *arg)
{
    pthread_mutex_t *tmp_mutex = arg;
    pthread_mutex_unlock(tmp_mutex);
}

static void *thread_timer_routine(void *arg)
{
    timer_thread *timer = (timer_thread *)arg;
    int return_status;
    int expired_time;
    struct timespec timespec;

    expired_time = timer->expired_time;

    pthread_cleanup_push(threadtimer_cancel_unlock, &timer->cond_mutex);
    pthread_mutex_lock(&timer->cond_mutex);
    while (!timer->down) {
        if (clock_gettime(CLOCK_MONOTONIC, &timespec) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "clock get time fail!\n");
            break;
        }

        if (timespec.tv_sec > timespec.tv_sec + expired_time) {
            etmemd_log(ETMEMD_LOG_ERR, "clock of tv_sec overflows\n");
            timer->down = false;
            break;
        }
        timespec.tv_sec += timer->expired_time;
        timespec.tv_nsec = 0;
        return_status = pthread_cond_timedwait(&timer->cond, &timer->cond_mutex, &timespec);
        if (return_status == ETIMEDOUT) {
            (*timer->functor)(timer->user_param);
        } else {
            etmemd_log(ETMEMD_LOG_WARN, "timer will be exit ! \n");
            break;
        }
    }
    /* unlock th timer->cond_mutex */
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

timer_thread* thread_timer_create(int seconds)
{
    pthread_condattr_t attr;

    timer_thread *timer = (timer_thread *)calloc(1, sizeof(timer_thread));
    if (timer == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "create timer fail\n");
        return NULL;
    }

    if (pthread_mutex_init(&timer->cond_mutex, NULL) != 0) {
        free(timer);
        etmemd_log(ETMEMD_LOG_WARN, "Failed to initialize the mutex of the timer\n");
        return NULL;
    }

    if (pthread_condattr_init(&attr) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "Failed to initialize the attr of condition of the timer\n");
        goto error_exit;
    }
    if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "Failed to set the attr of condition of the timer\n");
        goto error_exit;
    }
    if (pthread_cond_init(&timer->cond, &attr) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "Failed to initialize the condition of the timer\n");
        goto error_exit;
    }
    timer->down = true;
    timer->expired_time = seconds;

    etmemd_log(ETMEMD_LOG_DEBUG, "thread timer creates successfully!\n");
    return timer;

error_exit:
    pthread_mutex_destroy(&(timer->cond_mutex));
    free(timer);
    timer = NULL;
    return NULL;
}

int thread_timer_start(timer_thread* inst, void *(*executor)(void *arg), void *arg)
{
    if (inst == NULL || executor == NULL || arg == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "Timed instance startup parameter exception.\n");
        return -1;
    }

    /* if the functor of timer_thread instance is not NULL, it means that timer thread has been started */
    if (inst->functor != NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "thread timer has been started already\n");
        return 0;
    }

    inst->down = false;
    inst->functor = executor;
    inst->user_param = arg;
    if (pthread_create(&inst->pthread, NULL, thread_timer_routine, inst) != 0) {
        etmemd_log(ETMEMD_LOG_WARN, "start thread for timer instance fail.\n");
        inst->down = true;
        inst->functor = NULL;
        inst->user_param = NULL;
        return -1;
    }

    etmemd_log(ETMEMD_LOG_DEBUG, "thread timer starts successfully!\n");
    return 0;
}

void thread_timer_stop(timer_thread* inst)
{
    if (inst == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "The thread timer instance is null !\n");
        return;
    }
    inst->down = true;

    pthread_cancel(inst->pthread);
    pthread_join(inst->pthread, NULL);
    etmemd_log(ETMEMD_LOG_DEBUG, "Timer instance stops ! \n");
}

void thread_timer_destroy(timer_thread** inst)
{
    timer_thread *timer = NULL;

    if (inst == NULL || *inst == NULL) {
        etmemd_log(ETMEMD_LOG_WARN, "The thread timer instance is null !\n");
        return;
    }

    timer = *inst;
    if (!timer->down) {
        etmemd_log(ETMEMD_LOG_WARN,
                   "The scheduled task is being executed, stop the project now!\n");
        thread_timer_stop(*inst);
    }

    if (pthread_mutex_destroy(&(timer->cond_mutex)) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "The timer instance destroy faild !\n");
    }
    if (pthread_cond_destroy(&(timer->cond)) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "The timer instance destroy faild !\n");
    }
    free(timer);
    *inst = NULL;
    etmemd_log(ETMEMD_LOG_DEBUG, "Timer instance delete ! \n");
}
