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
 * Description: This is a header file of the function declaration for threadtimer function
 *              and the data structure definition for threadtimer.
 ******************************************************************************/

#ifndef ETMEMD_THREAD_TIMER_H
#define ETMEMD_THREAD_TIMER_H

typedef void *(*user_functional)(void *);
typedef struct timer_thread_t {
    pthread_mutex_t cond_mutex;
    pthread_cond_t cond;
    bool down;
    user_functional functor;
    void *user_param;
    int expired_time;
    pthread_t pthread;
} timer_thread;

/*
 * Create a timer instance that supports multiple instances
 * */
timer_thread* thread_timer_create(int seconds);

/*
 * Start timer thread instances
 * */
int thread_timer_start(timer_thread* inst, void *(*executor)(void *arg), void *arg);

/*
 * Stop timer thread instances
 * */
void thread_timer_stop(timer_thread* inst);

/*
 * Destroy the timer instance
 * */
void thread_timer_destroy(timer_thread** inst);

#endif //ETMEMD_THREAD_TIMER_H
